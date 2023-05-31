
#include <iostream>
#include <fstream>

#include <string.h>   		// strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>    		// close
#include <arpa/inet.h>     	// close
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> 		// FD_SET, FD_ISSET, FD_ZERO macros 

#include <chrono>		// sleep for ms
#include <thread>		// sleep for ms

#include "Message.h"

#define TRUE   1 
#define FALSE  0 
#define PORT 8888 

uint64_t timeSinceEpochMillisec() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

int main(int argc, char * argv[]) {
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	int status, valread, client_fd;
	struct sockaddr_in serv_addr;

	char buffer[1025];
	if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		std::cout << "Socket creation error" << std::endl;
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		std::cout << "Invalid address/ Address not supported" << std::endl;
		exit(EXIT_FAILURE);
	}

	if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
		std::cout << "Connection Failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	while (true) {
		small_world::SM_Event sme;
		sme.set_event_type("tick");
		sme.set_event_time(timeSinceEpochMillisec());

		std::string str;
		sme.SerializeToString(&str);

		send(client_fd, str.c_str(), str.length(), 0);
		std::cout << "Tick message sent" << std::endl;

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	// closing the connected socket
	close(client_fd);
				        
	return EXIT_SUCCESS;
}

