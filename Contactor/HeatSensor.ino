#include "Builds.h"

#if HEAT_SENSOR
#include "Codes.h"
#include "Watchdog.h"
#include "IO.h"


#define ALARM_TUTA 6
#define CIRCUIT_LAMP 13
#define TRANSMITTOR_PIN 8

#define BLINK_INTERVAL 700

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
    status = 0,
    alarm_indicator = 1
};

enum Switches
{
    High = 0,
    Low = 1
};

Lamp lamps[2];
Switch switches[2];

IO io(ID_HEAT_MONITOR, TRANSMITTOR_PIN, 50);

bool stoveON = false;
bool responseRecievedHighHeat = false;
bool responseRecievedLowHeat = false;
bool responseRecievedNoHeat = false;

bool high = false;
bool low = false;

//0 is OK, 1 is lowAlarm and 2 is highAlarm
int heatLevel = 0;

const int SWITCH_START = 2;
const int SWITCH_AMOUNT = 2;

const int LAMP_START = 4;
const int LAMP_AMOUNT = 2;

unsigned long greenBlinkStart = 0;
unsigned long redBlinkStart = 0;


void setup()
{
    watchdogOn();

    Serial.read();

    for (int i = SWITCH_START; i < SWITCH_START + SWITCH_AMOUNT; i++)
        switches[i - SWITCH_START] = Switch(i);

    for (int i = LAMP_START; i < LAMP_START + LAMP_AMOUNT; i++)
        lamps[i - LAMP_START] = Lamp(i, false);

    pinMode(TRANSMITTOR_PIN, OUTPUT);
    digitalWrite(TRANSMITTOR_PIN, LOW);

    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < LAMP_AMOUNT; j++)
            lamps[j].turnON();

        delay(50);

        for (int j = 0; j < LAMP_AMOUNT; j++)
            lamps[j].turnOFF();

        delay(50);
    }

    Serial.begin(9600);

    lamps[Lamps::status].turnON();
}

void loop()
{
    wdt_reset();

    //Hangs after 30 days to force a reboot
    while (millis() > 2592000000)
    {

    }

    io.clearMessages();

    if (Serial.available())
        io.recieveMessage();

    if (io.readMessage() == MESSAGE_CONNECTION_CHECK)
        io.respondOk();

    else if (io.readMessage() == MESSAGE_OVEN_ON)
    {
        stoveON = true;
        greenBlinkStart = millis();
        io.respondOk();
    }

    else if (io.readMessage() == MESSAGE_OVEN_OFF)
    {
        stoveON = false;
        lamps[Lamps::status].turnON();
        lamps[Lamps::alarm_indicator].turnOFF();
        io.respondOk();
    }

    else if (io.readMessage() == MESSAGE_CONTACTOR_ON)
    {
        noTone(ALARM_TUTA);
        lamps[Lamps::status].turnON();
        lamps[Lamps::alarm_indicator].turnOFF();
        io.respondOk();
    }

    else if (io.readMessage() == MESSAGE_CONTACTOR_OFF)
    {
        stoveON = false;
        lamps[Lamps::status].turnOFF();
        lamps[Lamps::alarm_indicator].turnON();
        io.respondOk();
    }


    if (stoveON)
    {
        if (millis() - greenBlinkStart > BLINK_INTERVAL)
        {
            lamps[Lamps::status].toggle();
            greenBlinkStart += BLINK_INTERVAL;
        }

        if (switches[Switches::High] && !high)
        {
            heatLevel = 2;
            high = true;
            low = false;
            responseRecievedHighHeat = false;
        }

        else if (switches[Switches::Low] && !switches[Switches::High] && !low)
        {
            tone(ALARM_TUTA, 33, 1000);

            heatLevel = 1;
            low = true;
            high = false;
            responseRecievedLowHeat = false;

            redBlinkStart = millis();
        }

        else if (!switches[Switches::Low] && !switches[Switches::High])
        {
            heatLevel = 0;
            low = false;
            high = false;
            responseRecievedNoHeat = false;

            lamps[Lamps::alarm_indicator].turnOFF();
        }

        if (heatLevel == 2)
        {
            tone(ALARM_TUTA, 41);
            if (!responseRecievedHighHeat)
            {
                io.sendMessage(MESSAGE_HEAT_WARNING_HIGH, ID_CONTACTOR);

                if (io.waitForResponse() == RESPONSE_OK)
                    responseRecievedHighHeat = true;
            }
        }

        else if (heatLevel == 1)
        {
            if (!responseRecievedLowHeat)
            {
                io.sendMessage(MESSAGE_HEAT_WARNING_LOW, ID_CONTACTOR);

                if (io.waitForResponse() == RESPONSE_OK)
                    responseRecievedLowHeat = true;
            }

            if (millis() - redBlinkStart > BLINK_INTERVAL)
            {
                lamps[Lamps::alarm_indicator].toggle();
                redBlinkStart += BLINK_INTERVAL;
            }
        }

        else
        {
            noTone(ALARM_TUTA);
            if (!responseRecievedNoHeat)
            {
                io.sendMessage(MESSAGE_HEAT_RESOLVED, ID_CONTACTOR);

                if (io.waitForResponse() == RESPONSE_OK)
                    responseRecievedNoHeat = true;
            }
        }
    }

    else
    {
        high = false;
        low = false;
    }


    
}
#endif