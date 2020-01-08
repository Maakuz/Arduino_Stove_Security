#pragma once
#define MAX_BUFFERED_MESSAGES 20

class IO
{
public:
    IO(int id, int transmittorPin = 8, int delayAmount = 0);
    virtual ~IO() {};

    void clearMessages();

    //Reads message if message has been recieved
    int readMessage();

    //Message is in format [RECIPIENT, SENDER, MESSAGE, END]
    void sendMessage(int msg, int recipient);
    int waitForResponse(int timeout = 5000);

    //Saves the message if it is adessed to m_id. To read message call "readMessage()".
    //Returns MESSAGE_STATUS_...
    int recieveMessage();
    void respondOk();
    void respondError();

private:
    int m_id;
    int m_pin;
    int m_delay;
    String m_sentMessage;
    String m_recievedMessage;
    
    int m_messageBufferSize;
    String m_messageBuffer[MAX_BUFFERED_MESSAGES];

    void addToMemoryBuffer(String msg);
};

IO::IO(int id, int transmittorPin, int delayAmount)
{
    m_id = id;
    m_pin = transmittorPin;
    m_sentMessage = "";
    m_recievedMessage = "";
    m_messageBufferSize = 0;
    m_delay = delayAmount;

    pinMode(transmittorPin, OUTPUT);
    digitalWrite(transmittorPin, LOW);
}

void IO::clearMessages()
{
    m_sentMessage = "";
    m_recievedMessage = "";
}

int IO::readMessage()
{
    if (m_recievedMessage.length() == 3)
        return m_recievedMessage[2];

    else if (m_messageBufferSize > 0)
    {
        m_recievedMessage = m_messageBuffer[--m_messageBufferSize];
        return m_recievedMessage[2];
    }

    return MESSAGE_STATUS_NOT_FOUND;
}

void IO::sendMessage(int msg, int recipient)
{
    delay(m_delay);

    digitalWrite(m_pin, HIGH);
    Serial.write(recipient);
    Serial.write(m_id);
    Serial.write(msg);
    Serial.write(MESSAGE_END);

    delay(50);

    digitalWrite(m_pin, LOW);

    m_sentMessage = "";
    m_sentMessage += (char)recipient;
    m_sentMessage += (char)m_id;
    m_sentMessage += (char)msg;
}

int IO::recieveMessage()
{
    String buf = "";
    char c = 0;
    while (c != MESSAGE_END)
    {
        c = Serial.read();

        if (c != MESSAGE_END && c != -1)
            buf += c;
    }

    delay(50);

    if (buf.length() == 3)
    {
        if (buf[0] == m_id)
        {
            m_recievedMessage = buf;
            return MESSAGE_STATUS_ADDRESSED_HERE;
        }

        else
            return MESSAGE_STATUS_ADDRESSED_NOT_HERE;
    }

    else
        return MESSAGE_STATUS_BAD;
}

void IO::respondOk()
{
    sendMessage(RESPONSE_OK, m_recievedMessage[1]);
}

void IO::respondError()
{
    sendMessage(RESPONSE_ERROR, m_recievedMessage[1]);
}

void IO::addToMemoryBuffer(String msg)
{
    if (m_messageBufferSize < MAX_BUFFERED_MESSAGES)
        m_messageBuffer[m_messageBufferSize++] = msg;
}

//if timeout is set to 0 it will never timeout
int IO::waitForResponse(int timeout = 5000)
{
    unsigned long start = millis();

    while (millis() - start < timeout || timeout == 0)
    {
        if (Serial.available())
        {
            if (recieveMessage() == MESSAGE_STATUS_ADDRESSED_HERE)
            {
                if (m_recievedMessage[1] == m_sentMessage[0] &&
                    (m_recievedMessage[2] == RESPONSE_OK || m_recievedMessage[2] == RESPONSE_ERROR))
                    return m_recievedMessage[2];

                else
                    addToMemoryBuffer(m_recievedMessage);
            }

        }
    }

    return RESPONSE_TIMEOUT;
}