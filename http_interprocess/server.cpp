//
//  hello.cc
//
//  Copyright (c) 2019 Yuji Hirose. All rights reserved.
//  MIT License
//

#include "httplib.h"
#include <iostream>
#include <thread>
#include <chrono>
using namespace httplib;


int btn_state{ 0 };
int encoder_f{ 0 };

void foo() {
    while(true){
        std::cout << "Button: " << btn_state << " Encoder_F: " << encoder_f << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(void) {
    Server svr;
    int* local_btn_state = &btn_state;
    int* local_encoder_f = &encoder_f;

    std::cout << "Ready to receive data over http. Example: http://localhost:8080/set?btn=1&enc=-123\n\n";

    svr.Get("/set", [&](const Request& req, Response& res) {
        *local_btn_state = atoi(req.get_param_value("btn").c_str());
        *local_encoder_f = atoi(req.get_param_value("enc").c_str());

        res.set_content("Thanks!", "text/plain");
        });

    std::thread first(foo);
    svr.listen("localhost", 8080);
}