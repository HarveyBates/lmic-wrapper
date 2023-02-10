#include "lmic-wrapper.h"

// Setup LoRa class static variables
Stream* LoRa::stream = nullptr;
osjob_t LoRa::sendjob{};
ev_t LoRa::last_event = EV_JOINING;
bool LoRa::joined = false;
bool LoRa::sent = false;
bool LoRa::time_set = false;
uint8_t LoRa::payload_index = 0;
int32_t LoRa::sleep_time_ms = 0;
uint32_t LoRa::unix_time = 0;
uint8_t LoRa::APP_EUI[8]{};
uint8_t LoRa::DEV_EUI[8]{};
uint8_t LoRa::APP_KEY[16]{};

// Arduino LMIC network time setup, used to get UNIX time over-the-air
static lmic_time_reference_t lmicNetworkTime{};
void request_network_time_cb(void* pUTCTime, int flagSuccess);

/** LMIC inbuilt functions **/
void os_getArtEui (u1_t* buf) { memcpy_P(buf, LoRa::APP_EUI, 8); }
void os_getDevEui (u1_t* buf) { memcpy_P(buf, LoRa::DEV_EUI, 8); }
void os_getDevKey (u1_t* buf) { memcpy_P(buf, LoRa::APP_KEY, 16); }
void onEvent (ev_t ev) { LoRa::on_event(ev); }

// Main LoRaWAN send function, will request network time if not already set
void send(osjob_t* j, uint8_t* _payload, uint8_t payload_size, uint8_t port) {
    if(!LoRa::is_time_set()){
        LMIC_requestNetworkTime(request_network_time_cb, &lmicNetworkTime);
    }
    LMIC_setTxData2(port, _payload, payload_size, 0);
}

void LoRa::init(Stream* _stream,
                const uint8_t* app_eui,
                const uint8_t* dev_eui,
                const uint8_t* app_key,
                int32_t _sleep_time_ms){
    LoRa::stream = _stream;
    LoRa::sleep_time_ms = _sleep_time_ms;
    LoRa::set_app_eui(app_eui);
    LoRa::set_device_eui(dev_eui);
    LoRa::set_app_key(app_key);

    // Startup arduino-lmic
    os_init();
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

    // Attempt to join
    LoRa::join();
}


void LoRa::join(){
    LMIC_startJoining();
    while((!check_joined() &&
           LoRa::last_event != EV_SCAN_TIMEOUT) ||
          !LMIC_queryTxReady()) {
        os_runloop_once();
    }
}

void LoRa::runloop_send(){
    while(!check_sent()) {
        os_runloop_once();
    }
}

void LoRa::test_connection() {
    uint8_t connect[4] = {1, 2, 3, 4};
    send_custom_payload(connect, sizeof(connect), 1);
    runloop_send();
}

template <typename T>
bool LoRa::append_to_payload(T packet){
    if((payload_index + sizeof(packet)) > sizeof(payload)){
        return false;
    }

    uint8_t byte_index = 0;
    for(uint8_t i = 0; i < (uint8_t)sizeof(packet.value); i++){
        payload[payload_index++] = packet.bytes[byte_index++];
    }

    return true;
}

bool LoRa::send_payload(){
    set_msg_sent(false);

    // Check if there is not a current TX/RX job running
    if(LMIC.opmode & OP_TXRXPEND || !LMIC_queryTxReady()){
        return false;
    }

    send(&sendjob, payload, LoRa::payload_index, 1);
    runloop_send();

    return check_sent();

}

bool LoRa::send_custom_payload(uint8_t* _payload, uint8_t _payload_size,
                               uint8_t port) {
    set_msg_sent(false);

    // Check if there is not a current TX/RX job running
    if(LMIC.opmode & OP_TXRXPEND || !LMIC_queryTxReady()){
        return false;
    }

    send(&sendjob, _payload, _payload_size, port);
    runloop_send();

    return check_sent();
}

void request_network_time_cb(void* pUTCTime, int flagSuccess){
    LoRa::set_unix_time(0);

    if(flagSuccess != 1 ||
       LMIC_getNetworkTimeReference(&lmicNetworkTime) != 1){
        LoRa::debugln("Unable to get network time.");
        return;
    }

    // Current utc time
    uint32_t unix_time = lmicNetworkTime.tNetwork + (315964800 - 18);

    // Offset between UTC time and time at which request was sent
    ostime_t ticks_now = os_getTime();
    ostime_t ticks_when_sent = lmicNetworkTime.tLocal;
    unix_time += osticks2ms(ticks_now - ticks_when_sent) / 1000;

    LoRa::set_unix_time(unix_time);

    // Set time_set flag to ensure the network time isn't requested again
    LoRa::set_time(true);

    LoRa::debug("EV_NETWORK_TIME (UNIX): ");
    LoRa::debugln(LoRa::get_unix_time());
}

void LoRa::on_event(ev_t ev) {
    set_last_event(ev);
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            stream->println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_JOINING:
            stream->println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            stream->println(F("EV_JOINED"));
            LMIC_setLinkCheckMode(0);
            LoRa::set_joined(true);
            break;
        case EV_JOIN_FAILED:
            stream->println(F("EV_JOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            // Check if all arduino-lmic is finished sending pending messages
            // If not keep in a tight loop with runloop_once()
            // Main query here is to see that the desired sleep time can be
            // safely slept for without missing an arduino-lmic event
            if(LMIC_queryTxReady() &&
               os_queryTimeCriticalJobs(sleep_time_ms) == 0){
                stream->println(F("EV_TXCOMPLETE: Scheduled jobs complete."));
                set_msg_sent(true);
            } else {
                stream->println(F("EV_TXCOMPLETE: Scheduled jobs pending."));
            }
            break;
        case EV_TXSTART:
            stream->println(F("EV_TXSTART"));
            break;
        case EV_JOIN_TXCOMPLETE:
            stream->println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        default:
            stream->print(F("Unknown event: "));
            stream->println((unsigned) ev);
            break;
    }
}
