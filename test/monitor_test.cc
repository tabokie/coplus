#include <iostream>
#include <cassert>
#include <typeinfo>
#include <thread>
#include <cassert>

#include "coplus/corand.h"
#include "coplus/colog.h"
#include "coplus/coroutine.h"
#include "coplus/async_timer.h"
#include "coplus/socket.h"

using namespace coplus;


// proper example with no blocking function
int coplus::main(int argc, char** argv){
	colog << "this is page monitor";
	Client client;
	colog << client.Connect(Client::ResolveHost("cspo.zju.edu.cn"), "80");
	colog << client.Send(
		"GET /redir.php?catalog_id=719823 HTTP/1.1\r\n"\
		"Host: cspo.zju.edu.cn\r\n"\
		"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/71.0.3578.98 Safari/537.36\r\n\r\n"
		// "Cookie: gr_user_id=14ddb499-0a95-41f6-bfdd-d84aa4b47e68; _ga=GA1.3.1911907513.1537533332; Hm_lvt_fe30bbc1ee45421ec1679d1b8d8f8453=1538295138; grwng_uid=ba59ea5a-7faf-4b88-9bf7-e5e9bf5da232; 8a762667df5cb9d5_gr_last_sent_cs1=388798; 8a762667df5cb9d5_gr_cs1=388798; UM_distinctid=167989aa975af-0f2d3cfd48912d-6313363-232800-167989aa977290; PHPSESSID=guni7t4kb3vhf9s04dboeogpg7; arp_scroll_position=522\r\n"
	);
	while(true) {
		colog << client.Receive(3000);
	}
	// computing here to wait for main scheduler ready
	// await(10); // 10 seconds
	colog << "exit main()...";
	return 0;
}
