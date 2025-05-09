#include <Arduino.h>
#include <CAN.h>
#include "Hackathon25.h"
#include <string.h>
#include <strings.h>
#include <vector>
#include <map>
#include <assert.h>
//#include <varargs.h>

#if DEBUG
#define ft_printf(args) \
	Serial.printf(args)
#else
#define ft_printf(args) \
	do {}while(0)
#endif

using namespace std;

int dirs[4] = {UP, DOWN, RIGHT, LEFT};
int alive_players[4] = {ALIVE, ALIVE, ALIVE, ALIVE};


int current_dir = UP;
int frame = 0;

// Global variables
const uint32_t hardware_ID = (*(RoReg *)0x008061FCUL);
uint8_t player_ID = 0;
uint8_t game_ID = 0;
uint8_t my_id = 3;
uint8_t my_idx = 0;
bool dead = false;
char error = 0;
uint8_t board[WIDTH][HEIGHT];

//void	ft_printf(const char *str)
//{
//	#if DEBUG
//
//	Serial.printf(str);
//	#endif
//}
//void	ft_printf(const char *str, va_list args)
//{
//	#if DEBUG
//
//	Serial.printf(str, args);
//	#endif
//}
enum algos {
	MODULO,
	MINMAX,
	//FLOODFILL,
};

enum algos algo_type = MINMAX;

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
	#if DEBUB
	Serial.printf("nenamed to %s\n", "TR-OFF");
	#endif
}

void recv_id(void) {
	MSG_Player msg_player;
	CAN.readBytes((uint8_t*)&msg_player, sizeof(MSG_Player));

	if(msg_player.HardwareID == hardware_ID){
		my_id = msg_player.PlayerID;
		ft_printf("Player ID recieved\n");
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
	ft_printf("send ack\n");
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
	ft_printf("Received game msg\n");
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
						if (board[x][y] == i + 1) {
							board[x][y] = 0;
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

void print_board(uint8_t board[WIDTH][HEIGHT]) {
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

bool used(uint8_t board[WIDTH][HEIGHT], struct point p, uint8_t move) {
	return (used(board, p.x, p.y, move));
}



uint8_t modulo_algo(struct game_state game_state) {
	uint8_t dir = current_dir;
	uint8_t mx = game_state.players[my_idx].x;
	uint8_t my = game_state.players[my_idx].y;
	if (used(board, mx, my, dir)) {
		dir++;
		if (dir > 4) {
			dir = 1;
		}
	}
	return (dir);
}

void add_point_to_map(uint8_t board[WIDTH][HEIGHT], struct point p, uint8_t val) {
	board[p.x][p.y] = val;
}

//todo
struct point apply_move_to_point(struct point p, uint8_t move) {
    struct point new_p = p;
    switch(move) {
        case LEFT:
            new_p.x = (p.x == 0) ? WIDTH - 1 : p.x - 1;
            break;
        case RIGHT:
            new_p.x = (p.x == WIDTH - 1) ? 0 : p.x + 1;
            break;
        case UP:
            new_p.y = (p.y == HEIGHT - 1) ? 0 : p.y + 1;
            break;
        case DOWN:
            new_p.y = (p.y == 0) ? HEIGHT - 1 : p.y - 1;
            break;
    }
    return (new_p);
}

void push_back_possible_moves(vector<struct point> &start_points, uint8_t board[WIDTH][HEIGHT], struct point bot_pos) {
	for (uint8_t i = 1; i < 5; i++) {
		if (!used(board, bot_pos, i)) {
			start_points.push_back(apply_move_to_point(bot_pos, i));
		}
	}
}

int get_score(uint8_t occupied[WIDTH][HEIGHT], vector<struct point> starts_in[4]) {
	int extra_score = 0;

	if (used(occupied, starts_in[my_idx][0], 1)) {
		extra_score = -1000000;
	}
	if (used(occupied, starts_in[my_idx][0], 2)) {
		extra_score = -1000000;
	}
	if (used(occupied, starts_in[my_idx][0], 3)) {
		extra_score = -1000000;
	}
	if (used(occupied, starts_in[my_idx][0], 4)) {
		extra_score = -1000000;
	}

	Serial.printf("get score\n");
	//std::map<struct point, int> graphs[4];
	vector<struct point> starts[4];
	for (int i = 0; i < 4; i++) {
		starts[i] = starts_in[i];
	}
	//memcpy(starts, starts_in, sizeof starts);

	uint8_t graphset[WIDTH][HEIGHT];
	memcpy(graphset, occupied, sizeof graphset);

	int order[4];
	int j = 0;
	for (int i = my_idx; i < 4; i++) {
		order[j++] = i;
	}
	for (int i = 0; i < my_idx; i++) {
		order[j++] = i;
	}
	int it = 1;
	while (true) {
		bool full = true;
		std::map<struct point, int> moves;
		for (int i = 0; i < 4; i++) {
			int cur_player = order[i];
			if (!(alive_players[cur_player] == ALIVE)) {
				continue ;
			}
			for (struct point start_point : starts[cur_player]) {
				for (uint8_t i = 1; i < 5; i++) {
					if (used(graphset, start_point, i)) {
						continue ;
					}
					struct point neighbour = apply_move_to_point(start_point, i);
					// or conditon lets enemies access contested areas
					if (!graphset[neighbour.x][neighbour.y] || (it == 1 && moves.find(neighbour) != moves.end())) {
						full = false;
						graphset[neighbour.x][neighbour.y] = cur_player + 4;
						moves[neighbour] = cur_player;
					}
				}
			}
		}
		//for (const pair<struct point, int> move : moves) {
		//	graphs[move.second][move.first] = it;
		//}
		//if (full || it > 40) {
		if (full) {
			break ;
		}
		//starts.clear();
		for (int i = 0; i < 4; i++) {
			vector<struct point> player_starts;
			for (pair<struct point, int> move : moves) {
				if (move.second == i) {
					player_starts.push_back(move.first);
				}
			}
			starts[i] = player_starts;
			//starts.push_back(player_starts);
		}
		it++;
	}
	int num_my_tiles = 0;
	int num_enemy_tiles = 0;
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			if (graphset[x][y] == my_idx + 4) {
				num_my_tiles++;
			} else if (graphset[x][y] >= 4) {
				num_enemy_tiles++;
			}
		}
	}

	int score = num_my_tiles * 10000000 + num_enemy_tiles * -100000 + extra_score;
	//return (num_my_tiles);
	printf("score: %d\n", score);
	return (score);
}

uint8_t minmax_algo(struct game_state game_state) {
	Serial.print("minmax entry\n");
	uint8_t x = game_state.players[my_idx].x;
	uint8_t y = game_state.players[my_idx].y;
	uint8_t best_move = current_dir;
	int best_score = INT_MIN;
	int current_score;

	vector<struct point> start_points[4];
	for (int i = 0; i < 4; i++) {
		if (alive_players[i] == ALIVE && i != my_idx) {
			start_points[i].push_back(game_state.players[i]);
		}
	}

#if DEBUG
	uint8_t possible_count = 0;;
#endif
	for (uint8_t i = 1; i < 5; i++) {
		if (used(board, x, y, i)) {
			Serial.printf("used (%u, %u): %u\n", x, y, i);
			continue ;
		}

#if DEBUG
		possible_count++;
#endif

		Serial.printf("unused (%u, %u): %u\n", x, y, i);
		start_points[my_idx].push_back(apply_move_to_point({x, y}, i));
		//current_score = get_score(game_state, i);

		//current_score = get_score(game_state, i);
		current_score = get_score(board, start_points);
		Serial.printf("score: %d\n", current_score);
		if (current_score > best_score) {
			best_score = current_score;
			best_move = i;
		}
	}
#if DEBUG
	if (possible_count) {
		assert(!used(board, {x, y}, best_move));
	}
#endif
	Serial.print("minmax return\n");
	return (best_move);
}

void algo() {
	struct game_state game_state;
	CAN.readBytes((uint8_t*)&game_state, sizeof game_state);
	ft_printf("Got game_state:\n");

	Serial.printf("PRE update_map: \n");
	//print_board();
	update_map(game_state);
	Serial.printf("POST update_map: \n");
	//print_board();


	uint8_t move = current_dir;
	switch (algo_type) {
		case (MODULO):
			move = modulo_algo(game_state);
			break;
		case (MINMAX):
			move = minmax_algo(game_state);
			break ;
		default:
			break ;
	}
	for (int i =0; i < 4; i++) {
		Serial.printf("%d: (%u, %u)\n", i, game_state.players[i].x, game_state.players[i].y);
	}

	if (!dead) {
		current_dir = move;
		send_move(move);
	}

	Serial.printf("frame: %d\n", frame++);
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
			if (dead) {
				return ;
			}
			algo();
			print_board();
			break ;
		}
		case (DIE): {
			break ;
		}
		case (MOVE):
		case (JOIN):
		case (RENAME):
		case (GAMEACK):
		{
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

//int get_score(struct game_state game_state, uint8_t move) {
//	Serial.printf("entry get_score\n");
//
//	struct point cur_pos = game_state.players[my_idx];
//
//	// check for possible player head collision
//    struct point my_new_pos = apply_move_to_point(cur_pos, move);
//    for(int i = 0; i < 4; i++) {
//        if(i == my_idx || game_state.players[i].x == 255)
//			continue;
//        struct point their_pos = {
//            game_state.players[i].x,
//            game_state.players[i].y
//        };
//        for(uint8_t their_move = 1; their_move <= 4; their_move++) {
//            struct point their_new_pos = apply_move_to_point(their_pos, their_move);
//            if(my_new_pos.x == their_new_pos.x && my_new_pos.y == their_new_pos.y) {
//                return (INT_MIN + 1);  // should only be choosen as a last resort, could potentially be smarter
//            }
//        }
//    }
//
//
//	uint8_t board_cpy[WIDTH][HEIGHT];
//
//	memcpy(board_cpy, board, sizeof board_cpy);
//	cur_pos = apply_move_to_point(game_state.players[my_idx], move);
//	add_point_to_map(board_cpy, cur_pos, my_idx + 1 + 10);
//	vector<struct point> start_points[4];
//	for (int i = 0; i < 4; i++) {
//		if (game_state.players[i].x == 255) {
//			continue ;
//		}
//		if (i != my_idx) {
//			start_points[i].push_back({game_state.players[i].x, game_state.players[i].y});
//		} else {
//			start_points[my_idx].push_back(cur_pos);
//		}
//	}
//
//	// us first for now
//	uint8_t order[4]; //todo: get array of which player is dead/alive
//	uint8_t j = 0;
//	for (uint8_t i = my_idx; i < 4; i++) {
//		order[j++] = i;
//	}
//	for (uint8_t i = 0; i < my_idx; i++) {
//		order[j++] = i;
//	}
//
//	int it = 0;
//	const int max_iter = 2;
//	while (it < max_iter) {
//		bool full = true;
//
//		std::map<struct point, uint8_t> moves;
//		for (uint8_t i = 0; i < 4; i++) {
//			uint8_t cur_bot = order[i];
//			if (i == 0 && it == 0) {
//				continue ; // we allready took this turn
//			}
//			for (const struct point &start_point : start_points[cur_bot]) {
//				if (start_point.x == 255 || start_point.y == 255) {
//					//todo: check if tile was captured this turn
//					continue ;
//				}
//				for (uint8_t cur_move = 1; cur_move < 5; cur_move++) {
//					if (used(board_cpy, start_point, cur_move)) {
//						continue ;
//					}
//					cur_pos = apply_move_to_point(start_point, cur_move);
//					full = false;
//					board_cpy[cur_pos.x][cur_pos.y] = cur_bot + 1 + 10;
//					start_points[cur_bot].push_back(cur_pos);
//					moves[cur_pos] = cur_bot;
//				}
//			}
//		}
//		if (full) {
//			break ;
//		}
//		for (int i = 0; i < 4; i++) {
//			vector<struct point> player_starts;
//			for (pair<struct point, int> move : moves) {
//				if (move.second == i) {
//					player_starts.push_back(move.first);
//				}
//			}
//			start_points[i] = player_starts;
//		}
//		it++;
//	}
//	print_board(board_cpy);
//	int num_my_tiles = 0;
//	int num_enemy_tiles = 0;
//	for (int x = 0; x < WIDTH; x++) {
//		for (int y = 0; y < HEIGHT; y++) {
//			if (board_cpy[x][y] == my_idx + 1 + 10) {
//				num_my_tiles++;
//			} else if (board_cpy[x][y] > 4) {
//				num_enemy_tiles++;
//			}
//		}
//	}
//	int score = num_my_tiles * 1000 + num_enemy_tiles * -10;
//	//return (num_my_tiles);
//	return (score);
//}


