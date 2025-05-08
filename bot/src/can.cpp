#include <Arduino.h>
#include <CAN.h>
#include "Hackathon25.h"

// CAN setup
bool setupCan(long baudRate) {
    pinMode(PIN_CAN_STANDBY, OUTPUT);
    digitalWrite(PIN_CAN_STANDBY, false);
    pinMode(PIN_CAN_BOOSTEN, OUTPUT);
    digitalWrite(PIN_CAN_BOOSTEN, true);

    if (!CAN.begin(baudRate)) {
        return false;
    }
    return true;
}

// Send JOIN packet via CAN
void send_Join(){
    MSG_Join msg_join;
    msg_join.HardwareID = hardware_ID;

    CAN.beginPacket(JOIN);
    CAN.write((uint8_t*)&msg_join, sizeof(MSG_Join));
    CAN.endPacket();

    Serial.printf("JOIN packet sent (Hardware ID: %u)\n", hardware_ID);
}



// Loop remains empty, logic is event-driven via CAN callback
void loop() {}
