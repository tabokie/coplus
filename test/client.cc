#include "coplus/channel.h"
#include "coplus/socket.h"
#include "coplus/cotimer.h"

#include <iostream>
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

	std::vector<Client> clients; 
	std::vector<std::thread> connections;
	std::string line;
	cout << menu_str ;
	std::vector<std::string> tokens;
	while(!close.load()) { // schedule
		cout << ">> ";
		mailbox >> line;
		if(StringUtil::starts_with(line, "cin")) {
			tokens.clear();
			int nToken = StringUtil::split(line, tokens);
			int command = 0;
			if(nToken > 1 && StringUtil::starts_with(tokens[1], "exit") ) {
				close.store(true);
			}
			else if(nToken > 1 &&
			 (command = atoi(tokens[1].c_str())) <= 7 && 
			 command > 0) {
				switch(command) {
					case 1: 
					if(nToken > 3) {
						clients.push_back(Client());
						clients.back().Connect(tokens[2], tokens[3]);
						connections.push_back(std::thread([&, &cur = clients.back()]{
							std::string line;
							while(!close.load()) {
								line = cur.Receive();
								if(line.size() == 0 && cur.is_closed()) {
									colog << "client closed";
									break;
								}
								else {
									mailbox << "server " + line;
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
					if(clients.size() > 0) clients.back().Send("time");
					else cout << "need connect to a server\n";
					break;
					case 4:
					if(clients.size() > 0) clients.back().Send("name");
					else cout << "need connect to a server\n";
					break;
					case 5:
					if(clients.size() > 0) clients.back().Send("list");
					else cout << "need connect to a server\n";
					break;
					case 6:
					if(nToken <= 3) cout << "need more arguments\n"; 
					else if(clients.size() == 0) cout << "need connect to a server\n";
					else clients.back().Send("relay" + tokens[2] + ":" + tokens[3]);
					break;
					case 7: // close all
					close.store(true);
					mailbox.close();
					std::for_each(clients.begin(), clients.end(), std::mem_fn(&Client::Close));
					break;
				}
			}
			else {
				cout << menu_str;
			}
		}
		else if( StringUtil::starts_with(line, "server") ){
			cout << (line.c_str() + 7) << endl;
		}
	}

	deamon.join();
	std::for_each(connections.begin(), connections.end(), std::mem_fn(&std::thread::join));
	return 0;
}