#include <msp430.h>
#include <msp430fr4133.h>

#define LASTTEMP_ADDR   ((unsigned int *)0x1800)   // 單一 FRAM 位址存最後一次溫度
unsigned int *lastTempPtr = LASTTEMP_ADDR;

unsigned int currentRead = 0;   // 目前這次量到的溫度 (ADC raw)
unsigned int recovered  = 0;   // 開機後從 FRAM 讀回的上一筆溫度

#define TEMP_THRESHOLD 685      // 動態調整的門檻值 
unsigned char enableWrite = 0;  // 按下按鍵後才開始寫 FRAM

//-----------------------------
//  簡單的 LED 閃爍：用來表示「上一次量到就已經過熱」
//-----------------------------
void blinkLED(unsigned int times)
{
    unsigned int i;
    for (i = 0; i < times; i++)
    {
        P1OUT ^= BIT0;          // 反轉 LED
        __delay_cycles(50000);  // 大約一點時間
        P1OUT ^= BIT0;
        __delay_cycles(50000);
    }
}

//-----------------------------
//  溫度感測器 / ADC 初始化
//-----------------------------
void initTemperatureSensor(void)
{
    PMMCTL0_H = PMMPW_H;                 // 解鎖 PMM 電源管理
    PMMCTL2 |= INTREFEN | TSENSOREN;     // 開啟內部參考電壓 + 溫度感測器
    __delay_cycles(10000);               // 給一點時間穩定
    PMMCTL0_H = 0;                       // 鎖回 PMM

    ADCCTL0 &= ~ADCENC;                  // 改設定前先關掉 ADC
    ADCCTL0 = ADCSHT_8 | ADCON;          // 長一點 sample time + 開啟 ADC
    ADCCTL1 = ADCSHP;                    // 使用 sampling timer
    ADCCTL2 = ADCRES_1;                  // 10-bit 解析度
    ADCMCTL0 = ADCINCH_12 | ADCSREF_1;   // A12 = 溫度感測器, 參考電壓內部 REF
    ADCCTL0 |= ADCENC;                   // 允許轉換
}

unsigned int readTemperature(void)
{
    ADCCTL0 |= ADCENC | ADCSC;           // 開始轉換
    while (ADCCTL1 & ADCBUSY);           // 等待完成
    return ADCMEM0;                      // 回傳 ADC 值
}

//-----------------------------
//  GPIO / 按鍵
//-----------------------------
void initGPIO(void)     // 紅燈 P1.0
{
    P1DIR |= BIT0;      // P1.0 output
    P1OUT &= ~BIT0;     // 一開始關燈
}

void initButton(void)   // P1.2 當按鍵
{
    P1DIR &= ~BIT2;     // 輸入
    P1REN |= BIT2;      // 啟用 pull
    P1OUT |= BIT2;      // pull-up
}

//-----------------------------
//  直接存取單一 FRAM 位址
//-----------------------------
void writeLastTemp(unsigned int v)
{
    SYSCFG0 &= ~DFWP;   // unlock Data FRAM
    *lastTempPtr = v;
    SYSCFG0 |= DFWP;    // lock Data FRAM
}

unsigned int readLastTemp(void)
{
    return *lastTempPtr;
}

int main(void)
{
    WDTCTL  = WDTPW | WDTHOLD;    // 關掉 watchdog
    PM5CTL0 &= ~LOCKLPM5;         // 解鎖 GPIO

    initGPIO();
    initButton();
    initTemperatureSensor();

    // 開機時從 FRAM 還原上一筆溫度 --------
    recovered = readLastTemp();

    // 如果是剛燒完程式，FRAM 可能是 0xFFFF 或 0x0000，就當成「沒有有效資料」
    if (recovered != 0xFFFF && recovered != 0x0000)
    {
        // 用「復原的溫度」做處理 --------
        // 如果上一次關機前就已經超過門檻，就先閃紅燈 10 下提示
        if (recovered > TEMP_THRESHOLD)
        {
            blinkLED(10);
        }
    }
    else
    {
        recovered = 0;   // 沒有舊資料就清成 0，方便在 WATCH 觀察
    }

    // -------- 進入正常運作迴圈 --------
    while (1)
    {
        // 1) 讀目前溫度
        currentRead = readTemperature();

        // 2) 把目前溫度寫進 FRAM
        if (enableWrite)
        {
            writeLastTemp(currentRead);
        }

        // 3) 用「目前溫度」判斷是否過熱，控制 LED
        if (currentRead > TEMP_THRESHOLD)
        {
            P1OUT |= BIT0;   // 過熱：亮燈
        }
        else
        {
            P1OUT &= ~BIT0;  // 未過熱：關燈
        }

        // 4) 若偵測到按鍵按下（P1.2 被拉低），啟動寫 FRAM 模式
        if (!(P1IN & BIT2))
        {
            enableWrite = 1;
        }

        __delay_cycles(100000); // 約 0.1s
    }
}

