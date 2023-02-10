/**
 * Joins over-the-air using app_eui, dev_eui and app_key provided by the user.
 * A message is sent every 5 minutes with a payload containing the temperature
 *
 * If possible the current UNIX time will be requested from network.
 */
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