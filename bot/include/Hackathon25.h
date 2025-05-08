#ifndef HACKATHON25_H
#define HACKATHON25_H

#define WIDTH 64
#define HEIGHT 64
#include <stdint.h>

//enum rcv_type {
//	ID,
//	GAME,
//	GAME_STATE,
//	DIE,
//};

extern const uint32_t hardware_ID;
extern uint8_t player_ID;
extern uint8_t game_ID;
extern uint8_t my_id;
extern uint8_t my_idx;
extern uint8_t board[WIDTH][HEIGHT];

enum CAN_MSGs {
	GAME = 0x040,
	GAME_STATE = 0x050,
	MOVE = 0x090,
	DIE = 0x080,
    JOIN = 0x100,
    LEAVE = 0x101,
    PLAYER = 0x110,
	GAMEACK = 0x120,
	RENAME = 0x500,
};

#define UP  1 //0, 1
#define RIGHT 2 // 1, 0
#define DOWN 3 // 0, -1
#define LEFT 4  // -1, 0

struct game_msg {
	uint8_t ids[4];
};

struct player_state {
	uint8_t x;
	uint8_t y;
};

struct game_state {
	struct player_state players[4];
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

