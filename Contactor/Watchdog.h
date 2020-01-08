#pragma once
#include <avr/wdt.h>

void watchdogOn() {

    // Clear the reset flag, the WDRF bit (bit 3) of MCUSR.
    MCUSR = MCUSR & B11110111;

    // Set the WDCE bit (bit 4) and the WDE bit (bit 3) 
    // of WDTCSR. The WDCE bit must be set in order to 
    // change WDE or the watchdog prescalers. Setting the 
    // WDCE bit will allow updtaes to the prescalers and 
    // WDE for 4 clock cycles then it will be reset by 
    // hardware.
    WDTCSR = WDTCSR | B00011000;

    // Set the watchdog timeout prescaler value to 1024 K 
    // which will yeild a time-out interval of about 8.0 s.
    WDTCSR = B00100001;

    // Enable the watchdog timer interupt.
    WDTCSR = WDTCSR | B01000000;
    MCUSR = MCUSR & B11110111;
}
