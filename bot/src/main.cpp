#include <Arduino.h>
#include <CAN.h>
#include "Hackathon25.h"
#include <string.h>

// Global variables
const uint32_t hardware_ID = (*(RoReg *)0x008061FCUL);
uint8_t player_ID = 0;
uint8_t game_ID = 0;
uint8_t my_id = 3;
uint8_t my_idx = 0;


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

void rename() {
	//char buf1[17];
	//memset(buf1, 0, sizeof buf1);
	//strcpy(buf1, );
	CAN.beginPacket(RENAME);
	//CAN.write((uint8_t*)&buf1, sizeof buf1);

	CAN.write(&my_id, sizeof my_id);
	CAN.write((uint8_t *)"TR-OFF", strlen("TR-OFF"));
	CAN.endPacket();
	Serial.printf("nenamed to %s\n", "TR-OFF");
}

void recv_id(void) {
	MSG_Player msg_player;
	CAN.readBytes((uint8_t*)&msg_player, sizeof(MSG_Player));

	if(msg_player.HardwareID == hardware_ID){
		my_id = msg_player.PlayerID;
		Serial.printf("Player ID recieved\n");
	}
	//  else {
	//	 player_ID = 0;
	// }

	Serial.printf("Received Player packet | Player ID received: %u | Own Player ID: %u | Hardware ID received: %u | Own Hardware ID: %u\n", 
		msg_player.PlayerID, player_ID, msg_player.HardwareID, hardware_ID);
	rename();
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

//void send_move() {
//	//char buf1[17];
//	//memset(buf1, 0, sizeof buf1);
//	//strcpy(buf1, );
//	CAN.beginPacket(MOVE);
//	//CAN.write((uint8_t*)&buf1, sizeof buf1);
//	CAN.write((uint8_t *)"TR-OFF", strlen("TR-OFF"));
//	CAN.endPacket();
//	Serial.printf("nenamed to %s\n", "TR-OFF");
//}

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
			struct game_state game_state;
			CAN.readBytes((uint8_t*)&game_state, sizeof game_state);
			Serial.printf("Got game_state:\n");
			for (int i =0; i < 4; i++) {
				Serial.printf("%d: (%u, %u)\n", i, game_state.players[i].x, game_state.players[i].y);
			}

			break ;
		}
		case (DIE): {
			break ;
		}
		default: {
			Serial.println("CAN: Received unknown packet");
			break;
		}
	}
  }
}
