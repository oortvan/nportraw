// WebSocket, v1.00 2012-09-13
//
// Description: WebSocket FRC6544 codec, written in C++.
// Homepage: http://katzarsky.github.com/WebSocket
// Author: katzarsky@gmail.com

#include "WebSocket.h"
#include "base64.h"
#include "sha1.h"
#include <iostream>
#include <string>
#include <vector>

//using namespace std;

WebSocket::WebSocket(){  // constructor
	hasclients = FALSE;
}

WebSocketFrameType WebSocket::parseHandshake(char *input_frame, int input_len){
	// 1. copy char*/len into string
	// 2. try to parse headers until \r\n occurs
	std::string headers((char*)input_frame, input_len);
	int header_end = headers.find("\r\n\r\n");

	if(header_end == std::string::npos) { // end-of-headers not found - do not parse
		return INCOMPLETE_FRAME;
	}

	headers.resize(header_end); // trim off any data we don't need after the headers
	std::vector<std::string> headers_rows = explode(headers, std::string("\r\n"));
	for(int i=0; i<headers_rows.size(); i++) {
		std::string& header = headers_rows[i];
		if(header.find("GET") == 0) {
			std::vector<std::string> get_tokens = explode(header, std::string(" "));
			if(get_tokens.size() >= 2) {
				this->resource = get_tokens[1];
			}
		}
		else {
			int pos = header.find(":");
			if(pos != std::string::npos) {
				std::string header_key(header, 0, pos);
				std::string header_value(header, pos+1);
				header_value = trim(header_value);
				if(header_key == "Host") this->host = header_value;
				else if(header_key == "Origin") this->origin = header_value;
				else if(header_key == "Sec-WebSocket-Key") this->key = header_value;
				else if(header_key == "Sec-WebSocket-Protocol") this->protocol = header_value;
			}
		}
	}

	//this->key = "dGhlIHNhbXBsZSBub25jZQ==";
	//printf("PARSED_KEY:%s \n", this->key.data());
	//return FrameType::OPENING_FRAME;
	//printf("HANDSHAKE-PARSED\n");
	return OPENING_FRAME;
}

std::string WebSocket::trim(std::string str){
	//printf("TRIM\n");
	const char* whitespace = " \t\r\n";
	std::string::size_type pos = str.find_last_not_of(whitespace);
	if(pos != std::string::npos) {
		str.erase(pos + 1);
		pos = str.find_first_not_of(whitespace);
		if(pos != std::string::npos) str.erase(0, pos);
	}
	else return std::string();
	return str;
}

std::vector<std::string> WebSocket::explode(std::string theString, std::string theDelimiter, bool theIncludeEmptyStrings){
	//printf("EXPLODE\n");
	//UASSERT( theDelimiter.size(), >, 0 );
	std::vector<std::string> theStringVector;
	int start = 0, end = 0, length = 0;
	while ( end != std::string::npos ){
		end = theString.find( theDelimiter, start );
		// If at end, use length=maxLength. Else use length=end-start.
		length = (end == std::string::npos) ? std::string::npos : end - start;
		if (theIncludeEmptyStrings	|| ( ( length > 0 ) /* At end, end == length == std::string::npos */
				    && ( start < theString.size() ) ) )
			theStringVector.push_back( theString.substr( start, length ) );
		// If at end, use start=maxSize. Else use start=end+delimiter.
		start = ( ( end > (std::string::npos - theDelimiter.size()) )
				      ? std::string::npos : end + theDelimiter.size() );
	}
	return theStringVector;
}

std::string WebSocket::answerHandshake(){
    unsigned char digest[20]; // 160 bit sha1 digest
	std::string answer;
	answer += "HTTP/1.1 101 Switching Protocols\r\n";
	answer += "Upgrade: WebSocket\r\n";
	answer += "Connection: Upgrade\r\n";
	if(this->key.length() > 0) {
		std::string accept_key;
		accept_key += this->key;
		accept_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; //RFC6544_MAGIC_KEY

		//printf("INTERMEDIATE_KEY:(%s)\n", accept_key.data());

		SHA1 sha;
		sha.Input(accept_key.data(), accept_key.size());
		sha.Result((unsigned*)digest);

		//printf("DIGEST:"); for(int i=0; i<20; i++) printf("%02x ",digest[i]); printf("\n");

		//little endian to big endian
		for(int i=0; i<20; i+=4) {
			unsigned char c;

			c = digest[i];
			digest[i] = digest[i+3];
			digest[i+3] = c;

			c = digest[i+1];
			digest[i+1] = digest[i+2];
			digest[i+2] = c;
		}

		//printf("DIGEST:"); for(int i=0; i<20; i++) printf("%02x ",digest[i]); printf("\n");

		accept_key = base64_encode((const unsigned char *)digest, 20); //160bit = 20 bytes/chars
		answer += "Sec-WebSocket-Accept: "+(accept_key)+"\r\n";
	}
	if(this->protocol.length() > 0) {
		answer += "Sec-WebSocket-Protocol: "+(this->protocol)+"\r\n";
	}
	answer += "\r\n";
	return answer;
	//return WS_OPENING_FRAME;
}


int WebSocket::makeFrame(WebSocketFrameType frame_type, unsigned char* msg, int msg_length, unsigned char* buffer, int buffer_size){
	int pos = 0;
	int size = msg_length;
	buffer[pos++] = (unsigned char)frame_type; // text frame

	if(size<=125) {
		buffer[pos++] = size;
	}
	else if(size<=65535) {
		buffer[pos++] = 126; //16 bit length
		buffer[pos++] = (size >> 8) & 0xFF; // rightmost first
		buffer[pos++] = size & 0xFF;
	}
	else { // >2^16-1
		buffer[pos++] = 127; //64 bit length

		//TODO: write 8 bytes length
		pos+=8;
	}
	memcpy((void*)(buffer+pos), msg, size);
	return (size+pos);
}

WebSocketFrameType WebSocket::getFrame(unsigned char* in_buffer, int in_length, unsigned char* out_buffer, int out_size, int* out_length){
	//printf("getTextFrame()\n");
	if(in_length < 3) return INCOMPLETE_FRAME;

	unsigned char msg_opcode = in_buffer[0] & 0x0F;
	unsigned char msg_fin = (in_buffer[0] >> 7) & 0x01;
	unsigned char msg_masked = (in_buffer[1] >> 7) & 0x01;

	// *** message decoding

	int payload_length = 0;
	int pos = 2;
	int length_field = in_buffer[1] & (~0x80);
	unsigned int mask = 0;

	//printf("IN:"); for(int i=0; i<20; i++) printf("%02x ",buffer[i]); printf("\n");

	if(length_field <= 125) {
	payload_length = length_field;
	}
	else if(length_field == 126) { //msglen is 16bit!
		payload_length = in_buffer[2] + (in_buffer[3]<<8);
		pos += 2;
	}
	else if(length_field == 127) { //msglen is 64bit!
		payload_length = in_buffer[2] + (in_buffer[3]<<8);
		pos += 8;
	}
	//printf("PAYLOAD_LEN: %08x\n", payload_length);
	if(in_length < payload_length+pos) {
		return INCOMPLETE_FRAME;
	}

	if(msg_masked) {
		mask = *((unsigned int*)(in_buffer+pos));
		//printf("MASK: %08x\n", mask);
		pos += 4;
		// unmask data:
		unsigned char* c = in_buffer+pos;
		for(int i=0; i<payload_length; i++) {
			c[i] = c[i] ^ ((unsigned char*)(&mask))[i%4];
		}
	}

	if(payload_length > out_size) {
	//TODO: if output buffer is too small -- ERROR or resize(free and allocate bigger one) the buffer ?
	}

	memcpy((void*)out_buffer, (void*)(in_buffer+pos), payload_length);
	out_buffer[payload_length] = 0;
	*out_length = payload_length+1;

	//printf("TEXT: %s\n", out_buffer);

	if(msg_opcode == 0x0) return (msg_fin)?TEXT_FRAME:INCOMPLETE_TEXT_FRAME; // continuation frame ?
	if(msg_opcode == 0x1) return (msg_fin)?TEXT_FRAME:INCOMPLETE_TEXT_FRAME;
	if(msg_opcode == 0x2) return (msg_fin)?BINARY_FRAME:INCOMPLETE_BINARY_FRAME;
	if(msg_opcode == 0x9) return PING_FRAME;
	if(msg_opcode == 0xA) return PONG_FRAME;

	return ERROR_FRAME;
}

int WebSocket::websock_handshake(int sock){
	WebSocketFrameType wframe;
	int n, building=1, slen;
	char buffer[256], cmd[600];
	cmd[0] = 0;
	while(building){  // try building a websocket connection 
		bzero(buffer,256);
		n = read(sock,buffer,256);
		strncat(cmd, buffer, n);
		if (n < 0 || n == 0){ 
			return 0;
		}	
		if (strstr(cmd, "\r") != NULL)
		if (strstr(cmd,"GET") != NULL)  // websocket handling
		if (strstr(cmd,"\r\n\r\n") != NULL) {
			//ws_printf("%s", cmd);  // show the get message from browser
			wframe = parseHandshake(cmd, strlen(cmd));
			std::string wanswer = answerHandshake();
			slen = write(sock, wanswer.c_str(), wanswer.length());
			building = 0;
			return slen;
		}
	}
}	

void WebSocket::put_to_websock(const char *message, int index){
	int slen, wl=0, i0, i1;
	char buffer[20000];
	/*
	 * when index < 0 all possible web clients must be scanned
	 * when	0 <= index < max_clients message only to web_client[index]
	 */
	if (index < 0)					{i0 = 0; i1 = max_clients;}
	else if (index < max_clients)	{i0 = index; i1 = index+1;}
	else return;
    for ( int i = i0 ; i < i1 ; i++)
     if (web_client[i].client_s > 0){
		if ( wl == 0 ){ // webframe not yet created
			slen = strlen(message);
			wl = makeFrame(TEXT_FRAME, (unsigned char *)message, slen, (unsigned char *)buffer, 256);
		}	
		slen = send(web_client[i].client_s,buffer,wl,MSG_NOSIGNAL);
		if ( slen == -1){  // client has broken connection
			close(web_client[i].client_s);
		    ws_printf("Closing webclient %d:%d, sockets as %s\n", i, web_client[i].client_s, web_client[i].ip);
			logging = "Closing WEBSOCKET "+itoa(i)+':'+itoa(web_client[i].client_s)+' '+web_client[i].ip;
			tolog(logging.c_str(), LOG_INFO);
			web_client[i].client_s = -1;
		}
	}
	
	for ( int i = 0 ; i < max_clients ; i++) 
		if (web_client[i].client_s > 0) { hasclients = TRUE; return; }
	hasclients = FALSE;
	tolog("WEBSOCKET has no clients", LOG_INFO);
}

void WebSocket::websock_process_0(struct tcp_server_info *tcp){
	struct sockaddr_in	des;
	int n, i, s, newsockfd, clilen = sizeof(des);
	fd_set readfds;
	//char buffer[50];
	std::string webformat;//, afie;

    for ( i = 0 ; i < max_clients ; i++)
        web_client[i].client_s = -1;
	ws_printf("STARTING WEBSOCKET FOR %d CLIENTS MAX\n",max_clients);
	while(1){
		// select triggered by master socket
		FD_ZERO(&readfds);
		FD_SET(tcp->tcpfd, &readfds);
		// one way ticket here, only in the handshake is writing to client
		// is possible to scan the clients for active one and add it to the select list
		//if (select( max_clients+12, &readfds , NULL , NULL , NULL) > 0){
		if (select( proj.si+5, &readfds , NULL , NULL , NULL) > 0){
			if (FD_ISSET(tcp->tcpfd, &readfds)){ 
				ws_printf("WS CLIENTCONNECT\n");
				if ((newsockfd = accept(tcp->tcpfd, (struct sockaddr *)&des, (socklen_t*) &clilen))<0){
					exit(EXIT_FAILURE);
				}
				//try to add new socket to array of sockets
				for ( i = 0; i < max_clients; i++){
				    s = web_client[i].client_s;
				    if (s == -1){ // check for unused socket item
						int alen = websock_handshake(newsockfd);
						if (alen > 0){
							sprintf(web_client[i].ip,"%s:%d",inet_ntoa(des.sin_addr),ntohs(des.sin_port));
							web_client[i].port = ntohs(des.sin_port);
						    web_client[i].client_s = newsockfd;
							//put_to_websock(tcp->webformat.c_str(),i);  // tell the browser what to expect
						    ws_printf("Adding webclient %d:%d, sockets as %s\n", i, newsockfd, web_client[i].ip);
							logging = "Adding WEBSOCKET "+itoa(i)+':'+itoa(newsockfd)+' '+web_client[i].ip;
							tolog(logging.c_str(), LOG_INFO);
							hasclients = TRUE;
							webformat.clear();
							nport_json(&webformat);  // create json info for client
							put_to_websock(webformat.c_str(),i);  // send json info to client
							ws_printf("%s", webformat.c_str());

						    i = max_clients;  // leaves the loop
						}	
				    }
					else if (i == (max_clients -1)){  // all spaces are used, skip it
						close(newsockfd);
					}	
				}
			}
			//  eventueel een for loop that scans the clients for a possible command
		}
	}	
}	

