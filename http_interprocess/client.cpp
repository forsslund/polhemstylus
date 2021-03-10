#include "httplib.h"
#include <iostream>
using namespace httplib;

int main(void)
{
    Client cli("localhost", 8080);

    if (auto res = cli.Get("/set?btn=1&enc=4711")) {
        if (res->status == 200) {
            std::cout << res->body << std::endl;
        }
    }
    else {
        auto err = res.error();
    }
}