#include "Builds.h"

#if CONTACTOR
#include "Codes.h"
#include "Watchdog.h"
#include "IO.h"


//#define ALARM_TUTA 11
#define CIRCUIT_LAMP 13
#define TRANSMITTOR_PIN 8

#define BLINK_INTERVAL 700
#define STOVE_TIMER 300000
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

IO io(ID_CONTACTOR, TRANSMITTOR_PIN, 0);

unsigned long greenBlinkStart = 0;
unsigned long redBlinkStart = 0;

unsigned long stoveTimerStart = 0;
unsigned long heatTimerStart = 0;
unsigned long currentCheckStart = 0;

bool stoveON = false;
bool alarmTriggered = false;
bool heatWarningRecieved = false;
bool heatWarning = false;
bool hasConnection = true;

const int SWITCH_START = 5;
const int SWITCH_AMOUNT = 3;

const int LAMP_START = 2;
const int LAMP_AMOUNT = 3;

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

    io.sendMessage(MESSAGE_CONNECTION_CHECK, ID_HEAT_MONITOR);
    int response = io.waitForResponse();

    if (response == RESPONSE_ERROR || response == RESPONSE_TIMEOUT)
        hasConnection = false;
}

void loop()
{
    wdt_reset();

    //Hangs after 30 days to force a reboot by watchdog
    while (millis() > 2592000000)
    {

    }

    io.clearMessages();

    long codeRecieved = 0;

    
    if (!alarmTriggered && hasConnection)
    {
        bool trigger = false;

        lamps[Lamps::contactor].turnON();
        
        if (codeRecieved == MESSAGE_MOTION_DETECTED)
        {
            stoveTimerStart = millis();
        }

        if (stoveON && codeRecieved == MESSAGE_FIRE_DETECTED)
            trigger = true;

        if (codeRecieved == MESSAGE_HEAT_WARNING)
            heatWarningRecieved = true;

        if (codeRecieved == MESSAGE_HEAT_RESOLVED)
            heatWarningRecieved = false;
            
        if (stoveON && heatWarningRecieved)
        {
            if (!heatWarning)
            {
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
            static bool messageConfirmed = false;

            if (!stoveON)
            {
                stoveTimerStart = millis();
                stoveON = true;
                messageConfirmed = false;
            }

            if (!messageConfirmed)
            {
                io.sendMessage(MESSAGE_OVEN_ON, ID_HEAT_MONITOR);

                if (io.waitForResponse(2000) == RESPONSE_OK)
                    messageConfirmed = true;
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

    else if (alarmTriggered)
    {
        if (millis() - currentCheckStart > CURRENT_CHECK_INTERVAL)
        {
            lamps[Lamps::contactor].turnON();

            if (!isCurrentON())
            {
                lamps[Lamps::alarm_indicator].turnOFF();
                alarmTriggered = false;
            }

            if (millis() - (currentCheckStart + CURRENT_CHECK_DURATION) > CURRENT_CHECK_INTERVAL)
            {
                lamps[Lamps::contactor].turnOFF();
                currentCheckStart += CURRENT_CHECK_INTERVAL + CURRENT_CHECK_DURATION;
            }
        }
    }

    //if no connection
    else
    {
        //Todo: reconnect
    }


}

void triggerAlarm()
{
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
#endif