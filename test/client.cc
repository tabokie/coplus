#include "coplus/channel.h"
#include "coplus/socket.h"
#include "coplus/cotimer.h"

#include "coplus/protocol.h"

#include <iostream>
#include <ostream>
#include <thread>
#include <vector>
#include <string>

using std::cin;
using std::cout;
using std::endl;

using coplus::Channel;
using coplus::Client;
using coplus::colog;

const char* menu_str = "_____________________________\n"
	"| Operation Menu: \n"\
	"| > 1: connect(ip, port)\n"\
	"| > 2: close()\n"\
	"| > 3: time()\n"\
	"| > 4: name()\n"\
	"| > 5: list()\n"\
	"| > 6: send(n-th, message)\n"\
	"| > 7: exit()\n"\
	"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";

int main(void) {

	Channel<std::string, 2> mailbox;
	std::atomic<bool> close = false;

	std::thread deamon([&]{ // handle input
		std::string line;
		while(!close.load() && (getline(cin, line), true)) {
			if(line.size() > 0) mailbox << "cin " + line;
			if(StringUtil::starts_with(line, "exit") || StringUtil::starts_with(line, "7")) break; // or getline block
		}
	});
	std::cout << ">> 1 10.180.168.248 4007\n" << "connection estabilished" << std::endl << ">> \n";

	std::vector<Client> clients; 
	std::vector<std::thread> connections;
	std::string line;
	std::atomic<int> count = 0;
	// client protocol
	bool pending = false;
	std::string pendingStr;

	cout << menu_str ;
	std::vector<std::string> tokens;
	while(!close.load()) { // schedule
		cout << ">> ";
		mailbox >> line; // mailbox is shared between recv thread and cin thread
		if(StringUtil::starts_with(line, "cin")) {
			tokens.clear();
			int nToken = StringUtil::split(line, tokens, 4);
			int command = 0;
			if(nToken > 1 && StringUtil::starts_with(tokens[1], "exit") ) {
				close.store(true);
				mailbox.close();
				std::for_each(clients.begin(), clients.end(), std::mem_fn(&Client::Close));
			}
			else if(nToken > 1 &&
			 (command = atoi(tokens[1].c_str())) <= 7 && 
			 command > 0) {
			 	// dispatch command
				switch(command) {
					case 1: 
					if(nToken > 3) {
						clients.push_back(Client());
						if(clients.back().Connect(tokens[2], tokens[3])) {
							std::cout << "connection estabilished" << std::endl;
						}
						else {
							std::cout << "connection broke" << std::endl;
							continue;
						}
						connections.push_back(std::thread([&, &cur = clients.back()]{
							std::string package;
							while(!close.load()) {
								package = cur.Receive();
								if(package.size() > 0) {
									mailbox << "server " + package;
								}
								if(cur.is_closed()) {
									colog << "client closed";
									break;
								}
							}
						}));
					} else cout << "need more arguments\n"; 
					break;
					case 2:
					if(clients.size() > 0) clients.back().Close();
					else cout << "need connect to a server\n";
					break;
					case 3:
					if(clients.size() > 0) {
						count = 0; // reset counter
						for(int i = 0; i < 100; i++) {
							clients.back().Send(Protocol::pickle_message(Protocol::kRequest,"time"));
						}
					}
					else cout << "need connect to a server\n";
					break;
					case 4:
					if(clients.size() > 0) clients.back().Send(Protocol::pickle_message(Protocol::kRequest,"name"));
					else cout << "need connect to a server\n";
					break;
					case 5:
					if(clients.size() > 0) clients.back().Send(Protocol::pickle_message(Protocol::kRequest,"list"));
					else cout << "need connect to a server\n";
					break;
					case 6:
					if(nToken <= 3) cout << "need more arguments\n"; 
					else if(clients.size() == 0) cout << "need connect to a server\n";
					else clients.back().Send(Protocol::pickle_message(Protocol::kRequest,"relay" + tokens[2] + ":" + tokens[3]));
					break;
					case 7: // close all
					close.store(true);
					mailbox.close();
					std::for_each(clients.begin(), clients.end(), std::mem_fn(&Client::Close));
					break;
				}
			}
			else { // unknown command
				cout << menu_str;
			}
		}
		else if( StringUtil::starts_with(line, "server") ){ // package from server
			static auto parse = [&](std::string& in)-> int {
				std::string head(Protocol::app_head_len, ' ');
				memcpy((void*)head.c_str(), (void*)&Protocol::app_head, Protocol::app_head_len);
				std::string tail(Protocol::app_terminator_len, ' ');
				memcpy((void*)tail.c_str(), (void*)&Protocol::app_terminator, Protocol::app_terminator_len);
				int h = in.find(head);
				int t = in.find(tail);
				bool has_head = (h != std::string::npos);
				bool has_end = (t != std::string::npos);
				if(has_head && !pending) {
					if(has_end) {
						count++;
						Protocol::format_message(
							cout, 
							in.substr(
								h + Protocol::app_head_len, 
								t - h - Protocol::app_head_len
							)
						);
					} else {
						pending = true;
						pendingStr = in.substr(
							h + Protocol::app_head_len, 
							in.size() - Protocol::app_terminator_len - h - Protocol::app_head_len
						);
					}
				} else if(pending) {
					if(has_end) { // finish pending messaeg
						pendingStr += in.substr(0, t);
						count++;
						Protocol::format_message(
							cout, 
							pendingStr
						);
						pending = false;
						pendingStr.clear();
					} else {
						pendingStr += in;
					}
				}
				return pending ? -1 : t + Protocol::app_terminator_len;
			};
			int cur = 7, delta;
			// parse all package from socket data //
			while( cur < line.size() && (delta = parse(line.substr(cur))) >= 0) cur += delta;
		}
	}
	deamon.join();
	std::for_each(connections.begin(), connections.end(), std::mem_fn(&std::thread::join));
	// if(count != 100)std::cout << "total package after `time`: " << 100 << std::endl;
	std::cout << "total package after `time`: " << count << std::endl;
	return 0;
}