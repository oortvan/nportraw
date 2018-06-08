// WebSocket, v1.00 2012-09-13
//
// Description: WebSocket FRC6544 codec, written in C++.
// Homepage: http://katzarsky.github.com/WebSocket
// Author: katzarsky@gmail.com

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <assert.h>
#include <stdint.h> /* uint8_t */
#include <stdio.h> /* sscanf */
#include <ctype.h> /* isdigit */
#include <stddef.h> /* int */
#include "../nportcdf-turb-v1.0/nport.h"

// std c++
#include <vector>
#include <string>

enum WebSocketFrameType {
	ERROR_FRAME=0xFF00,
	INCOMPLETE_FRAME=0xFE00,

	OPENING_FRAME=0x3300,
	CLOSING_FRAME=0x3400,

	INCOMPLETE_TEXT_FRAME=0x01,
	INCOMPLETE_BINARY_FRAME=0x02,

	TEXT_FRAME=0x81,
	BINARY_FRAME=0x82,

	PING_FRAME=0x19,
	PONG_FRAME=0x1A
};

typedef struct c_socket{
	int client_s;
	//char cmd[500];
	//int r3state; // 0: continuous mode, 1 command mode
	int port;
	char ip[25];
	//int is_web, just_new;
};

class WebSocket{
public:

	std::string resource;
	std::string host;
	std::string origin;
	std::string protocol;
	std::string key;
	std::string logging;
	struct c_socket web_client[max_clients];
	bool hasclients; // boolean to see if any client is connected
	WebSocket();

	/**
	* @param input_frame .in. pointer to input frame
	* @param input_len .in. length of input frame
	* @return [WS_INCOMPLETE_FRAME, WS_ERROR_FRAME, WS_OPENING_FRAME]
	*/
	WebSocketFrameType parseHandshake(char* input_frame, int input_len);
	std::string answerHandshake();

	int makeFrame(WebSocketFrameType frame_type, unsigned char* msg, int msg_len, unsigned char* buffer, int buffer_len);
	WebSocketFrameType getFrame(unsigned char* in_buffer, int in_length, unsigned char* out_buffer, int out_size, int* out_length);

	std::string trim(std::string str);
	std::vector<std::string> explode(std::string theString, std::string theDelimiter, bool theIncludeEmptyStrings = false );

	int websock_handshake(int sock);
	void put_to_websock(const char *message, int index);
	void websock_process_0(struct tcp_server_info *tcp);
};

#endif /* WEBSOCKET_H */
