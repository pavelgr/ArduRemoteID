
#pragma once

#ifdef DEBUG
#define DEBUG_BAUDRATE 115200

#define DBEGIN() Serial.begin(DEBUG_BAUDRATE)
//#define DBEGIN() Serial2.begin(g.baudrate, SERIAL_8N1, GPIO_NUM_18, GPIO_NUM_17);

#define DPRINT(x) Serial.print(x)
#define DPRINTLN(x) Serial.println(x)

#define DPRINTF(...) Serial.printf(__VA_ARGS__)

#else
#define DBEGIN()

#define DPRINT(x)
#define DPRINTLN(x)

#define DPRINTF(...)

#endif
