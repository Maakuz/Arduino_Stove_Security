#pragma once

//0 is reserved for errors
#define ID_CONTACTOR 1
#define ID_HEAT_MONITOR 2

#define MESSAGE_CONTACTOR_OFF 10
#define MESSAGE_CONTACTOR_ON 11

#define MESSAGE_OVEN_OFF 20
#define MESSAGE_OVEN_ON 21


#define MESSAGE_HEAT_RESOLVED 30
#define MESSAGE_HEAT_WARNING_LOW 31
#define MESSAGE_HEAT_WARNING_HIGH 32
#define MESSAGE_FIRE_DETECTED 33

#define MESSAGE_MOTION_DETECTED 40

#define MESSAGE_CONNECTION_CHECK 50

#define RESPONSE_OK 100
#define RESPONSE_ERROR 101
#define RESPONSE_TIMEOUT 102

#define MESSAGE_STATUS_ADDRESSED_HERE 120
#define MESSAGE_STATUS_ADDRESSED_NOT_HERE 121
#define MESSAGE_STATUS_NOT_FOUND 122
#define MESSAGE_STATUS_BAD 123

#define MESSAGE_END 127


//#define toChar(integear) integear + '0' 
//#define toInt(character) character - '0'

//Connection check every minute or so
//Test timeout response