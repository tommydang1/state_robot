
#include <iostream>
#include <string>  

#include <chrono>
#include <cstdint>

#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>    

#include <arpa/inet.h>     
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h>

#include "Message.h"

#define PORT 8888 

class Tickable {
	uint64_t last_tick_time = 0;

	public:
		virtual void tick(const small_world::SM_Event & event) {
			last_tick_time = event.event_time();
		}
};

class StateMachine;

class RobotState {
	uint64_t initial_time = 0;
	uint64_t current_time = 0;
	std::map<std::string, std::shared_ptr<RobotState>> next_states;

	/*
	uint64_t get_elapsed() {
		return current_time - initial_time;
	}
	*/

	public:
		std::shared_ptr<StateMachine> owner;
		void set_owner(std::shared_ptr<StateMachine> new_owner) {
			owner = new_owner;
		}

		void set_next_state(const std::string & state_name, std::shared_ptr<RobotState> state) {
			next_states[state_name] = state;
		}

		std::shared_ptr<RobotState> get_next_state(const std::string & transition_name) {
			std::map<std::string, std::shared_ptr<RobotState>>::iterator it = next_states.find(transition_name);
			if (it == next_states.end()) return nullptr;
			return it->second;
		}

		virtual void tick(const small_world::SM_Event &e) {
			if (initial_time == 0) initial_time = e.event_time();
			current_time = e.event_time();
			decide_action(initial_time, current_time);
		}

		virtual std::string get_state_name() {
			return "";
		}

		virtual void decide_action(uint64_t & initial_time, uint64_t current_time) = 0;
};

class StateMachine : public Tickable {
	std::shared_ptr<RobotState> current_state;

	public:
		virtual void tick(const small_world::SM_Event & event) {
			Tickable::tick(event);
			if (current_state != nullptr) current_state->tick(event);
		}

		virtual void set_current_state(std::shared_ptr<RobotState> cs) {
			current_state = cs;
		}
};

class TimedState : public RobotState {
	std::string state_name;
	std::string verb_name;
	std::string state_finished_name;
	uint64_t time_to_wait = 2000;

	public:
		void set_time_to_wait (uint64_t new_time_to_wait) {
			time_to_wait = new_time_to_wait;
		}

		void set_state_name(const std::string & new_state_name) {
			state_name = new_state_name;
		}

		void set_verb_name(const std::string & new_verb_name) {
			verb_name = new_verb_name;
		}

		void set_state_finished_name(const std::string & new_state_finished_name) {
			state_finished_name = new_state_finished_name;
		}

		virtual std::string get_state_name() {
			return state_name;
		}
		
		virtual void decide_action(uint64_t & initial_time, uint64_t current_time) {
			if (current_time - initial_time < time_to_wait) {
				std::cout << verb_name << std::endl;
				return;
			}
			initial_time = 0;
			std::shared_ptr<RobotState> next = get_next_state("done");
			if (next == nullptr) {
				std::cout << "Can't get a next state to go to" << std::endl;
				return;
			}
			std::cout << state_finished_name << std::endl;
			std::cout << next->get_state_name() << std::endl;
			owner->set_current_state(next);
		}
};

int main(int argc, char * argv[]) {

	// use these strings to indicate the state transitions
	// the robot progresses through.  Do not modify these strings

	std::string robot_waiting = "The robot is waiting";
	std::string robot_moving = "The robot is moving";

	std::string robot_finished_waiting = "The robot finished waiting";
	std::string robot_finished_moving = "The robot finished moving";

	std::string robot_began_waiting = "The robot began waiting";
	std::string robot_began_moving = "The robot began moving";

	// State Machine Code
	std::shared_ptr<StateMachine> sm = std::make_shared<StateMachine>();

	std::shared_ptr<TimedState> s0 = std::make_shared<TimedState>();
	s0->set_state_name(robot_began_waiting);
	s0->set_verb_name(robot_waiting);
	s0->set_state_finished_name(robot_finished_waiting);
	s0->set_owner(sm);

	std::shared_ptr<TimedState> s1 = std::make_shared<TimedState>();
	s1->set_state_name(robot_began_moving);
	s1->set_verb_name(robot_moving);
	s1->set_state_finished_name(robot_finished_moving);
	s1->set_owner(sm);

	s0->set_next_state("done", s1);
	s1->set_next_state("done", s0);
	sm->set_current_state(s0);

	// Server code
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[1025];

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		std::cout << "socket failed" << std::endl;  
		exit(EXIT_FAILURE);
	}
	
	// Forcefully attaching socket to the port 8888
	if (setsockopt(server_fd, SOL_SOCKET,
			SO_REUSEADDR | SO_REUSEPORT, &opt,
			sizeof(opt))) {
		std::cout << "setsockopt failed" << std::endl;  
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// Forcefully attaching socket to the port 8888
	if (bind(server_fd, (struct sockaddr*)&address,
				sizeof(address))
			< 0) {
		std::cout << "bind failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		std::cout << "error listening" << std::endl;
		exit(EXIT_FAILURE);
	}
	if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
		std::cout << "error accepting" << std::endl;
		exit(EXIT_FAILURE);
	}
	while(true) {
		if (valread = read(new_socket, buffer, 1024)) {
			small_world::SM_Event sme;
			sme.ParseFromString(buffer);
			// std::cout << sme.event_type() << " Time: " << sme.event_time() << std::endl;
			sm->tick(sme);
		}
	}

	// close the connected socket
	close(new_socket);
	// closing the listening socket
	shutdown(server_fd, SHUT_RDWR);

	return EXIT_SUCCESS;
}

