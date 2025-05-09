#ifndef HACKATHON25_H
#define HACKATHON25_H

#define WIDTH 64
#define HEIGHT 64
#include <stdint.h>

#define UP  1 //0, 1
#define RIGHT 2 // 1, 0
#define DOWN 3 // 0, -1
#define LEFT 4  // -1, 0

extern const uint32_t hardware_ID;
extern uint8_t player_ID;
extern uint8_t game_ID;
extern uint8_t my_id;
extern uint8_t my_idx;
extern uint8_t board[WIDTH][HEIGHT];
extern int dirs[4];
extern int current_dir;

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

struct point {
	uint8_t x;
	uint8_t y;
	point(int x, int y): x(x), y(y){}
	point(void): x(0), y(0){}
	bool operator<(const point& other) const {
		if (x != other.x) return x < other.x;
		return y < other.y;
	}
};

struct game_msg {
	uint8_t ids[4];
};

struct player_state {
	uint8_t x;
	uint8_t y;
};

struct game_state {
	struct point players[4];
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

void print_board(void);

void print_board(uint8_t board[WIDTH][HEIGHT]);
bool used(uint8_t board[WIDTH][HEIGHT], uint8_t x, uint8_t y, uint8_t move);
#endif

