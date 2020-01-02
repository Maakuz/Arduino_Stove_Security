#include <avr/wdt.h>
#include "Codes.h"

//#define ALARM_TUTA 11
#define CIRCUIT_LAMP 13

#define BLINK_INTERVAL 700
#define STOVE_TIMER 5000
#define CURRENT_CHECK_INTERVAL 1000
#define CURRENT_CHECK_DURATION 50

#define HEAT_TIMER 7000

void triggerAlarm();
bool isCurrentON();

struct Lamp
{
    int pin;
    bool status;

    Lamp(int pin = 0, bool status = false)
    {
        this->pin = pin;
        this->status = status;
        pinMode(pin, OUTPUT);

        if (status)
            digitalWrite(pin, LOW);

        else
            digitalWrite(pin, HIGH);
    }

    void turnON()
    {
        if (!this->status)
        {
            this->status = true;
            digitalWrite(pin, LOW);
        }
    }

    void turnOFF()
    {
        if (this->status)
        {
            this->status = false;
            digitalWrite(pin, HIGH);
        }
    }

    void toggle()
    {
        if (this->status)
            turnOFF();

        else
            turnON();
    }
};

struct Switch
{
    int pin;

    Switch(int pin = 0)
    {
        this->pin = pin;
        pinMode(pin, INPUT_PULLUP);
    }

    operator bool()
    {
        return digitalRead(pin) == LOW ? true : false;
    }
};

enum Lamps 
{
    oven_status = 0,
    alarm_indicator = 1,
    contactor = 2
};

enum Switches
{
    current1 = 0,
    current2 = 1,
    current3 = 2
};

Lamp lamps[3];
Switch switches[3];

unsigned long greenBlinkStart = 0;
unsigned long redBlinkStart = 0;

unsigned long stoveTimerStart = 0;
unsigned long heatTimerStart = 0;
unsigned long currentCheckStart = 0;

bool stoveON = false;
bool alarmTriggered = false;
bool heatWarningRecieved = false;
bool heatWarning = false;

const int SWITCH_START = 5;
const int SWITCH_AMOUNT = 3;

const int LAMP_START = 2;
const int LAMP_AMOUNT = 3;

void sendMessage(int msg)
{
    Serial.write(4 + '0');
    Serial.write("e");
}

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

void setup()
{
    watchdogOn();

    for (int i = SWITCH_START; i < SWITCH_START + SWITCH_AMOUNT; i++)
        switches[i - SWITCH_START] = Switch(i);

    for (int i = LAMP_START; i < LAMP_START + LAMP_AMOUNT; i++)
        lamps[i - LAMP_START] = Lamp(i, false);


    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < LAMP_AMOUNT; j++)
            lamps[j].turnON();

        delay(50);

        for (int j = 0; j < LAMP_AMOUNT; j++)
            lamps[j].turnOFF();

        delay(50);
    }

    greenBlinkStart = millis();

    Serial.begin(9600);

    Serial.println("Setup done");
    sendMessage(CONTACTOR_ON);
}

void loop()
{
    wdt_reset();

    //Hangs after 30 days to force a reboot
    while (millis() > 2592000000)
    {

    }

    long codeRecieved = 0;
    if (Serial.available())
    {
        String buf= "";
        char c = ' ';
        while (c != END_OF_MESSAGE)
        {
            c = Serial.read();

            if (c >= '0' && c <= '9')
                buf += c;
        }

        codeRecieved = buf.toInt();

        //for debugging
        char ibuf[12];
        itoa(codeRecieved, ibuf, 10);
        Serial.write(ibuf);
        
        Serial.write('\n');
    }

    if (!alarmTriggered)
    {
        bool trigger = false;

        lamps[Lamps::contactor].turnON();

        if (codeRecieved == MOTION_DETECTED)
        {
            stoveTimerStart = millis();
        }

        if (stoveON && codeRecieved == FIRE_DETECTED)
            trigger = true;

        if (codeRecieved == HEAT_WARNING)
            heatWarningRecieved = true;

        if (codeRecieved == HEAT_RESOLVED)
            heatWarningRecieved = false;

        if (stoveON && heatWarningRecieved)
        {
            if (!heatWarning)
            {
#ifdef ALARM_TUTA
                tone(ALARM_TUTA, 400, 200);
#endif // ALARM_TUTA

                heatWarning = true;
                heatTimerStart = millis();
                redBlinkStart = millis();
            }

            if (millis() - heatTimerStart > HEAT_TIMER)
                trigger = true;


            if (millis() - redBlinkStart > BLINK_INTERVAL)
            {
                lamps[Lamps::alarm_indicator].toggle();
                redBlinkStart += BLINK_INTERVAL;
            }
        }

        else
        {
            lamps[Lamps::alarm_indicator].turnOFF();
            heatWarning = false;
        }

        if (isCurrentON())
        {
            if (!stoveON)
            {
                stoveTimerStart = millis();
                stoveON = true;
            }

            if (millis() - greenBlinkStart > BLINK_INTERVAL)
            {
                lamps[Lamps::oven_status].toggle();
                greenBlinkStart += BLINK_INTERVAL;
            }

            if (millis() - stoveTimerStart > STOVE_TIMER)
                trigger = true;
        }

        else
        {
            stoveON = false;
            lamps[Lamps::oven_status].turnON();
        }


        if (trigger)
            triggerAlarm();
    }

    //if alarmTriggered
    else
    {
        if (millis() - currentCheckStart > CURRENT_CHECK_INTERVAL)
        {
            lamps[Lamps::contactor].turnON();

            if (!isCurrentON())
            {
                lamps[Lamps::alarm_indicator].turnOFF();
                alarmTriggered = false;
#ifdef ALARM_TUTA
                noTone(ALARM_TUTA);
#endif // ALARM_TUTA
            }

            if (millis() - (currentCheckStart + CURRENT_CHECK_DURATION) > CURRENT_CHECK_INTERVAL)
            {
                lamps[Lamps::contactor].turnOFF();
                currentCheckStart += CURRENT_CHECK_INTERVAL + CURRENT_CHECK_DURATION;
            }
        }
    }


}

void triggerAlarm()
{
#ifdef ALARM_TUTA
    tone(ALARM_TUTA, 200);
#endif // ALARM_TUTA

    alarmTriggered = true;
    lamps[Lamps::contactor].turnOFF();
    lamps[Lamps::oven_status].turnOFF();
    lamps[Lamps::alarm_indicator].turnON();
    stoveON = false;
    currentCheckStart = millis();
}

bool isCurrentON()
{
    return switches[Switches::current1] || switches[Switches::current2] || switches[Switches::current3];
}
