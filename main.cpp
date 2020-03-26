// Michał Mikołajczyk
// 307371
// 2020-03-26

#include "traceroute.h"
#include <arpa/inet.h>
#include <iomanip>
#include <iostream>
#include <netinet/ip_icmp.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Buffer for ip addr validation
    unsigned char buf[sizeof(struct in6_addr)];
    if(argc != 2) {
        std::cerr << "Error: wrong number of arguments.\nExample "
                     "usage (required root privileges):\n\ttraceroute 127.0.0.1\n";
        return -1;
    }
    if(inet_pton(AF_INET, argv[1], buf) == 0) {
        std::cerr << "Error: invalid ip address: " << argv[1] << ". Pass an ipv4 address.\n";
        return -1;
    }
    if(getuid()) {
        std::cerr << "Error: you cannot perform this operation unless you are root.\n";
        return -1;
    }

    // Set up raw socket
    int icmpsocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    bool reached_target = false;
    for(int i = 1; i <= 30 && !reached_target; ++i) {
        auto responses = pingRouters(argv[1], i, icmpsocket);
        std::cout << i << ". ";

        // No responses
        if(responses.response_count == 0)
            std::cout << "*\n";
        else {
            // List ip addresses
            for(unsigned j = 0; j < responses.ip_count; ++j) {
                std::cout << responses.ip_addr[j] << " ";
                // Check if the target ip_addr has been reached
                if(responses.ip_addr[j].compare(argv[1]) == 0)
                    reached_target = true;
            }

            // Not all requests got a response
            if(responses.response_count < 3)
                std::cout << "???\n";
            else {
                auto avg = (responses.response_times[0] + responses.response_times[1] +
                            responses.response_times[2]) /
                           3.0;
                std::cout << std::fixed << std::setprecision(2) << avg << "ms\n";
            }
        }
    }
    return 0;
}
