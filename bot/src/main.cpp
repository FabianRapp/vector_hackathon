#include <Arduino.h>
#include <CAN.h>
#include "Hackathon25.h"

// Global variables
const uint32_t hardware_ID = (*(RoReg *)0x008061FCUL);
uint8_t player_ID = 0;
uint8_t game_ID = 0;

// Setup
void setup() {
    Serial.begin(115200);
    while (!Serial);

    
    Serial.println("Initializing CAN bus...");
    if (!setupCan(500000)) {
        Serial.println("Error: CAN initialization failed!");
        while (1);
    }
    Serial.println("CAN bus initialized successfully."); 

    CAN.onReceive(onReceive);

    delay(1000);
    send_Join();
}

// Receive player information
void rcv_Player(){
    MSG_Player msg_player;
    CAN.readBytes((uint8_t*)&msg_player, sizeof(MSG_Player));

    if(msg_player.HardwareID == hardware_ID){
        player_ID = msg_player.PlayerID;
        Serial.printf("Player ID recieved\n");
    }
    //  else {
    //     player_ID = 0;
    // }

    Serial.printf("Received Player packet | Player ID received: %u | Own Player ID: %u | Hardware ID received: %u | Own Hardware ID: %u\n", 
        msg_player.PlayerID, player_ID, msg_player.HardwareID, hardware_ID);
}


