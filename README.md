# lmic-wrapper

Provides the setup and handling of LoRaWAN communications when using 
the [arduino-lmic](https://github.com/mcci-catena/arduino-lmic) library.

## Features
- [x] Connect to gateway using TTN keys
- [x] Send message and communicate with gateway to schedule future messages
- [x] Get network time (unix time) over-the-air
- [x] Payload handling
  - [x] Easily define new values
  - [x] Easily add these values to the LoRa payload
- [ ] Intergrated support for node when sleeping

## Example
```c++
#include "lmic-wrapper.h"

// Keys from TTN
static const uint8_t PROGMEM APPEUI[8] = { APP_EUI };
static const uint8_t PROGMEM DEVEUI[8] = { DEV_EUI };
static const uint8_t PROGMEM APPKEY[16] = { APP_KEY };

// A payload is made up of several `Packets`
LoRa::Packet<double> temperature;

void setup() {
    Serial.begin(115200);
    while(!Serial) yield();
    LoRa::init(&Serial, APPEUI, DEVEUI, APPKEY);
}

void loop() {
    temperature.value = 20.3; // Dummy value
    LoRa::append_to_payload(temperature);
    LoRa::send_payload();
    LoRa::reset_payload();
    delay(300000);
}
```

## Licence 
This project is under the GNU LESSER GENERAL PUBLIC LICENSE as found in the LICENCE file.
