# Memorage_Lab
### Linux Kernel Driver & MSP430 Temperature Logger
Developed a Linux kernel module and embedded temperature logging system as part of a systems programming lab.  

- Implemented a Hello World Linux kernel module in C and compiled it using a Makefile.
- Tested module loading/unloading using `insmod`, `rmmod`, and verified kernel messages via `dmesg`.
- Recompiled the Linux kernel and integrated the module as a built-in kernel component. 
- Programmed the MSP430FR4133 to read temperature data using the internal ADC sensor.
- Stored sensor readings in FRAM (non-volatile memory) to preserve data across power cycles.
- Implemented a power recovery mechanism to restore the last recorded temperature after reboot and trigger an LED alert when the temperature exceeds a threshold.
  
Technologies: C, Linux Kernel Module, Embedded Systems, MSP430, ADC12, FRAM
