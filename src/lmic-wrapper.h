#ifndef LMIC_WRAPPER_H
#define LMIC_WRAPPER_H

#include <Arduino.h>

// arduino-lmic imports
#include "lmic.h"
#include <hal/hal.h>

// SPI for custom interface to LoRaWAN module (RFM95W)
#include <SPI.h>

class LoRa {
    static Stream* stream;
    static ev_t last_event;

    // Payload setup
    static const uint8_t MAX_PAYLOAD_SIZE = 52;
    static uint8_t payload[MAX_PAYLOAD_SIZE];
    static uint8_t payload_index;

    // LoRa join state
    static bool joined;
    // LoRa message state (sent or pending)
    static bool sent;

    // arduino-lmic job
    static osjob_t sendjob;

    // User defined sleep time (checked against deadlines to ensure
    // arduino-lmic can complete all jobs before sleeping)
    static int32_t sleep_time_ms;

    // Network time
    static uint32_t unix_time;
    static bool time_set;

public:
    static uint8_t APP_EUI[8];
    static uint8_t DEV_EUI[8];
    static uint8_t APP_KEY[16];
private:
    static void set_app_eui(const uint8_t* app_eui){
        memcpy(APP_EUI, app_eui, 8);
    }
    static void set_device_eui(const uint8_t* dev_eui){
        memcpy(DEV_EUI, dev_eui, 8);
    }
    static void set_app_key(const uint8_t* app_key){
        memcpy(APP_KEY, app_key, 16);
    }

public:
	static void init(Stream* _stream,
                     const uint8_t* app_eui,
                     const uint8_t* dev_eui,
                     const uint8_t* app_key,
                     int32_t _sleep_time_ms = 0);
    static void on_event(ev_t ev);
	static void test_connection();

    // Payload related functions and templates
	template <typename T>
	union Packet {
		T raw;
		uint8_t b[sizeof(T)]{};
	};
	template <typename T>
	static bool append_to_payload(T value);
    static bool send_payload(uint8_t* _payload,
                             uint8_t _payload_size,
                             uint8_t port = 0);
    static void clear_payload(){
        memset(payload, 0, sizeof(payload));
        payload_index = 0;
    }
    static uint8_t remaining_bytes() {
        return MAX_PAYLOAD_SIZE - payload_index;
    }

    // Get current unix time if network request was successful
    static uint32_t get_unix_time() {
        if(time_set) { return unix_time; }
        return 0;
    }

    /** Below here while public are not intended for user access **/
    // Set unix time
    static void set_unix_time(uint32_t ut) { unix_time = ut; }
    // Set whether the unix time has been set
    static void set_time(bool set) { time_set = set; }
    static bool is_time_set() { return time_set; }

    template<typename T>
    static void debug(T msg) { stream->print(msg); }
    template<typename T>
    static void debugln(T msg) { stream->println(msg); }

private:
    static void join();
    static void runloop_send();

    // Check if a message has been sent
    static bool check_sent() { return sent; }
    // Set if a message has been sent
    static void set_msg_sent(bool s) { sent = s; }

    // Check if a message has been sent
    static bool check_joined() { return joined; }
    // Set if a message has been sent
    static void set_joined(bool j) { joined = j; }

    // Keep track of lmic events
    static void set_last_event(ev_t ev){ last_event = ev; }
};

#endif // LMIC_WRAPPER_H
