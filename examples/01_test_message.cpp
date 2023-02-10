/**
 * Joins over-the-air using app_eui, dev_eui and app_key provided by the user.
 * A message is sent every 5 minutes with a test payload ([1,2,3,4]).
 *
 * If possible the current UNIX time will be requested from network.
 */
#include <Arduino.h>
#include "lmic-wrapper.h"

const int32_t sleep_time = 5 * (1000 * 60);

static const uint8_t PROGMEM APPEUI[8] = { YOUR_APP_EUI };
static const uint8_t PROGMEM DEVEUI[8] = { YOUR_DEVICE_EUI };
static const uint8_t PROGMEM APPKEY[16] = { YOUR_APP_KEY };

void setup() {
    Serial.begin(115200);
    while(!Serial) yield();
    LoRa::init(&Serial, APPEUI, DEVEUI, APPKEY, sleep_time);
}

void loop() {
    Serial.println("Sending test message");
    LoRa::test_connection();
    Serial.println("Completed.");
    delay(sleep_time);
}
