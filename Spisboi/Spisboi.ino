#include <avr/wdt.h>

#define ALARM_TUTA 11
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
    contactor = 0,
    oven_status = 1,
    alarm_indicator = 2
};

enum Switches
{
    fire = 0,
    heat = 1,
    motion = 2,
    current1 = 3,
    current2 = 4,
    current3 = 5
};

Lamp lamps[3];
Switch switches[6];

unsigned long greenBlinkStart = 0;
unsigned long redBlinkStart = 0;

unsigned long stoveTimerStart = 0;
unsigned long heatTimerStart = 0;
unsigned long currentCheckStart = 0;

bool stoveON = false;
bool alarmTriggered = false;
bool heatWarning = false;

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

    for (int i = 0; i < 6; i++)
        switches[i] = Switch(i + 2);

    for (int i = 0; i < 3; i++)
        lamps[i] = Lamp(i + 8, false);


    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 3; j++)
            lamps[j].turnON();

        delay(100);

        for (int j = 0; j < 3; j++)
            lamps[j].turnOFF();

        delay(50);
    }

    greenBlinkStart = millis();

}

void loop()
{
    wdt_reset();

    //Hangs after 30 days to force a reboot
    while (millis() > 2592000000)
    {

    }

    if (!alarmTriggered)
    {
        bool trigger = false;

        lamps[Lamps::contactor].turnON();

        if (switches[Switches::motion])
        {
            stoveTimerStart = millis();
        }

        if (stoveON && (switches[Switches::fire]))
            trigger = true;

        if (stoveON && (switches[Switches::heat]))
        {
            if (!heatWarning)
            {
                tone(ALARM_TUTA, 400, 200);
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
                noTone(ALARM_TUTA);
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
    tone(ALARM_TUTA, 200);
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
