#include <Arduino.h>
#include <CAN.h>
#include "Hackathon25.h"
#include <string.h>
#include <strings.h>


// Global variables
const uint32_t hardware_ID = (*(RoReg *)0x008061FCUL);
uint8_t player_ID = 0;
uint8_t game_ID = 0;
uint8_t my_id = 3;
uint8_t my_idx = 0;

uint8_t board[WIDTH][HEIGHT];


// Setup
void setup() {
	memset(board, 0, sizeof board);

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


struct rename {
	uint8_t my_id;
	uint8_t size;
	char buf[2];
}__attribute__((packed));

void rename() {
	CAN.beginPacket(RENAME);
	struct rename buf;
	memset(&buf, 0, sizeof buf);
	buf.my_id = my_id;
	buf.size = 2;
	buf.buf[0] = 'A';
	buf.buf[1] = 0;

	CAN.write((uint8_t*)&buf, sizeof buf);

	CAN.endPacket();
	Serial.printf("nenamed to %s\n", "TR-OFF");
}

void recv_id(void) {
	MSG_Player msg_player;
	CAN.readBytes((uint8_t*)&msg_player, sizeof(MSG_Player));

	if(msg_player.HardwareID == hardware_ID){
		my_id = msg_player.PlayerID;
		Serial.printf("Player ID recieved\n");
		rename();
	}
	//  else {
	//	 player_ID = 0;
	// }

	Serial.printf("Received Player packet | Player ID received: %u | Own Player ID: %u | Hardware ID received: %u | Own Hardware ID: %u\n", 
		msg_player.PlayerID, player_ID, msg_player.HardwareID, hardware_ID);
}

void send_game_ack(void) {
	CAN.beginPacket(GAMEACK);
	CAN.write((uint8_t*)&my_id, sizeof my_id);
	CAN.endPacket();
	Serial.printf("send ack\n");
}

void rcv_game(void) {
	struct game_msg game_msg;

	CAN.readBytes((uint8_t*)&game_msg, sizeof game_msg);
	Serial.printf("Received game msg\n");
	for (uint8_t i = 0; i < 4; i++) {
		if (game_msg.ids[i] == my_id) {
			my_idx = i;
			Serial.printf("Found my_idx: %u\n", my_idx);
			send_game_ack();
			return ;
		}
	}
}

void send_move(uint8_t move) {
	CAN.beginPacket(MOVE);

	CAN.write(&my_id, sizeof my_id);
	CAN.write(&move, sizeof move);
	CAN.endPacket();
	Serial.printf("send move %u\n", move);
}

void print_board(void) {
	for (int y = HEIGHT - 1; y>= 0; y--) {
		for (int x = 0; x < WIDTH; x++) {
			Serial.printf("%d ", board[x][y]);
		}
	}
}

bool used(uint8_t board[WIDTH][HEIGHT], uint8_t x, uint8_t y, uint8_t move) {
	if (move == LEFT) {
		if (x == 0) {
			x = WIDTH - 1;
		} else {
			x--;
		}
	} else if (move == RIGHT) {
		if (x == WIDTH - 1) {
			x = 0;
		} else {
			x++;
		}
	} else if (move == UP) {
		if (y == HEIGHT - 1) {
			y = 0;
		} else {
			y++;
		}
	} else if (move == DOWN) {
		if (y == 0) {
			y = HEIGHT -1;
		} else {
			y++;
		}
	}
}

void algo() {
	struct game_state game_state;
	CAN.readBytes((uint8_t*)&game_state, sizeof game_state);

	for (int i = 0; i < 4; i++) {
		if (game_state.players[i].x == 255 || game_state.players[i].y) {
			//todo
		} else {
			board[game_state.players[i].x][game_state.players[i].y] = i + 1;
		}
	}

	Serial.printf("Got game_state:\n");
	for (int i =0; i < 4; i++) {
		Serial.printf("%d: (%u, %u)\n", i, game_state.players[i].x, game_state.players[i].y);
	}
	
	uint8_t move = LEFT;
	send_move(move);
}

// CAN receive callback
void onReceive(int packetSize) {
  if (packetSize) {
	switch(CAN.packetId()) {
		case (PLAYER): {
			recv_id();
			break ;
		}
		case (GAME): {
			rcv_game();
			break ;
		}
		case (GAME_STATE): {
			algo();
			print_board();
			break ;
		}
		case (DIE): {
			break ;
		}
		default: {
			Serial.println("CAN: Received unknown packet:%x\n", CAN.packetId());
			break;
		}
	}
  }
}
