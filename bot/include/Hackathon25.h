#ifndef HACKATHON25_H
#define HACKATHON25_H

#include <stdint.h>
extern const uint32_t hardware_ID;
extern uint8_t player_ID;
extern uint8_t game_ID;


enum CAN_MSGs {
    Join = 0x100,
    Leave = 0x101,
    Player = 0x110
};

struct __attribute__((packed)) MSG_Join {
    uint32_t HardwareID;
};

struct __attribute__((packed)) MSG_Player {
    uint32_t HardwareID;
    uint8_t PlayerID;
};

// can.cpp
void onReceive(int packetSize);
bool setupCan(long baudRate);
void send_Join();

// main.cpp
void rcv_Player();
#endif

