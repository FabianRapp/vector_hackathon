#include <Arduino.h>
#include <CAN.h>
#include "Hackathon25.h"
#include <string.h>
#include <strings.h>
int dirs[4] = {UP, DOWN, RIGHT, LEFT};
int alive_players[4] = {ALIVE, ALIVE, ALIVE, ALIVE};


int current_dir = UP;

// Global variables
const uint32_t hardware_ID = (*(RoReg *)0x008061FCUL);
uint8_t player_ID = 0;
uint8_t game_ID = 0;
uint8_t my_id = 3;
uint8_t my_idx = 0;
bool dead = false;
char error = 0;
uint8_t board[WIDTH][HEIGHT];

void	ft_printf(char *str)
{
	#if DEBUG
	Serial.printf(str);
	#endif
}

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

//Rest func for new game
void reset_game()
{
	player_ID = 0;
	game_ID = 0;
	my_id = 3;
	my_idx = 0;
	dead = false;
	error = 0;
	// board[WIDTH][HEIGHT];
	memset(board, 0, sizeof board);

	current_dir = UP;
	memset(alive_players, ALIVE, sizeof(alive_players));
}

void rcv_game(void) {
	struct game_msg game_msg;

	CAN.readBytes((uint8_t*)&game_msg, sizeof game_msg);
	Serial.printf("Received game msg\n");
	for (uint8_t i = 0; i < 4; i++) {
		if (game_msg.ids[i] == my_id) {
			reset_game();
			my_idx = i;
			Serial.printf("Found my_idx: %u\n", my_idx);
			send_game_ack();
			return ;
		}
	}
	dead = true;
}

void send_move(uint8_t move) {
	CAN.beginPacket(MOVE);

	CAN.write(&my_id, sizeof my_id);
	CAN.write(&move, sizeof move);
	CAN.endPacket();
	Serial.printf("send move %u\n", move);
}

void	update_map(game_state game)
{
	for (int i = 0; i < 4; i++) {
		if (game.players[i].x == 255 || game.players[i].y == 255) {
			Serial.printf("PLAYER %d died\n", i);
			if (i == my_idx) {
				dead = true;
			}
			else
			{
				alive_players[i] = DEAD;
				for (int y = HEIGHT - 1; y >= 0; y--) {
					for (int x = 0; x < WIDTH; x++) {
						if (board[game.players[i].x][game.players[i].y] == i + 1) {
							board[game.players[i].x][game.players[i].y] = 0;
						}
					}
				}
			}
			//todo
		} else {
			board[game.players[i].x][game.players[i].y] = i + 1;
		}
	}
	/*
		1. Check gamestate
			if 255 val --> find player and rm its fields
		2. go through board check the player_Index + 1
			if (board[x][y] == (player_Index + 1))
				board[x][y] = 0;
	*/
}

void print_board(void) {
	#if DEBUG
	for (int y = HEIGHT - 1; y >= 0; y--) {
		for (int x = 0; x < WIDTH; x++) {
			Serial.printf("%d ", board[x][y]);
		}
		Serial.printf("\n");
	}
	Serial.printf("\n");
	#endif
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
			y = HEIGHT - 1;
		} else {
			y--;
		}
	}
	return (board[x][y] != 0);
}

void algo() {

	int me;												   // my data
	int mx;												   // my x-coordinate
	int my;												   // my y-coordinate
	int nx;												   // new/potential x-coordinate
	int ny;
	int dir;
	int nd;
	int tx;
	int ty;
	struct game_state game_state;
	CAN.readBytes((uint8_t*)&game_state, sizeof game_state);

	Serial.printf("PRE update_map: \n");
	print_board();
	update_map(game_state);
	Serial.printf("POST update_map: \n");
	print_board();

	dir = current_dir;
	mx = game_state.players[my_idx].x;
	my = game_state.players[my_idx].y;
	if (used(board, mx, my, dir)) {
		dir++;
		if (dir > 4) {
			dir = 1;
		}
	}
	for (int turn = 0; turn <= 3; turn++)
	{
		nd = (dir + turn) % 4;
		tx = mx + dirs[nd];
		ty = my + dirs[nd];
		if ( board[ty][tx] == 0 )
		{
			dir = nd;
			break;
		}
	}
	Serial.printf("Got game_state:\n");
	for (int i =0; i < 4; i++) {
		Serial.printf("%d: (%u, %u)\n", i, game_state.players[i].x, game_state.players[i].y);
	}

	uint8_t move = dir;
	if (!dead) {
		current_dir = move;
		send_move(move);
	}
}

// CAN receive callback
void onReceive(int packetSize) {
  if (packetSize) {
	long type = CAN.packetId();
	switch(type) {
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
			//Serial.println("CAN: Received unknown packet:%d\n", CAN.packetId());
			Serial.printf("CAN: Received unknown packet: %x\n", type);
			break;
		}
	}
  }
}
