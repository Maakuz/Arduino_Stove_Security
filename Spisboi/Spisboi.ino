#define ALARM_TUTA 11
#define CIRCUIT_LAMP 13

#define GREEN_BLINK_INTERVAL 2000
#define STOVE_TIMER 5000

void triggerAlarm();

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
unsigned long stoveTimerStart = 0;

bool stoveON = false;
bool alarmTriggered = false;

void setup()
{
    for (int i = 0; i < 6; i++)
        switches[i] = Switch(i + 2);

    for (int i = 0; i < 3; i++)
        lamps[i] = Lamp(i + 8, false);

    greenBlinkStart = millis();
    lamps[Lamps::alarm_indicator].turnOFF();
}

void loop()
{
    if (stoveON)
    {
        if (switches[Switches::motion])
        {
            stoveTimerStart = millis();
        }

        if (switches[Switches::fire] || switches[Switches::heat])
            triggerAlarm();

    }


    if (switches[Switches::current1] || switches[Switches::current2] || switches[Switches::current3])
    {
        if (!stoveON && !alarmTriggered)
        {
            stoveTimerStart = millis();
            lamps[Lamps::contactor].turnON();
            stoveON = true;
        }

        if (millis() - stoveTimerStart > STOVE_TIMER)
        {
            if (!alarmTriggered)
                triggerAlarm();
        }

        if (millis() - greenBlinkStart > GREEN_BLINK_INTERVAL)
        {
            greenBlinkStart += GREEN_BLINK_INTERVAL;
            lamps[Lamps::oven_status].toggle();
        }
    }

    else
    {
        lamps[Lamps::contactor].turnOFF();
        lamps[Lamps::oven_status].turnON();
        stoveON = false;
        alarmTriggered = false;
    }

    if (alarmTriggered)
        lamps[Lamps::alarm_indicator].turnON();

    else
        lamps[Lamps::alarm_indicator].turnOFF();

}

void triggerAlarm()
{
    tone(ALARM_TUTA, 200, 100);
    alarmTriggered = true;
    lamps[Lamps::contactor].turnOFF();
    stoveON = false;
}
