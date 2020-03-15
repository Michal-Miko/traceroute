#include "utils.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


int main() {
    // Set up raw socket
    int icmpsocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buffer[IP_MAXPACKET];

    // Get PID for use in ICMP headers as unique id
    /* const int pid = getpid(); */

    bool recieving = true;
    while(recieving) {
        // Recieve a packet from the socket's queue
        ssize_t packet_len = recvfrom(icmpsocket,
                                      buffer,
                                      IP_MAXPACKET,
                                      0,
                                      (struct sockaddr *)&sender,
                                      &sender_len);

        if(packet_len < 0)
            std::cerr << "recvfrom error: " << std::strerror(errno);

        // Get ip addr as string
        char sender_ip_str[20];
        inet_ntop(AF_INET,
                  &(sender.sin_addr),
                  sender_ip_str,
                  sizeof(sender_ip_str));

        // Get icmp header
        struct ip *ip_header = (struct ip *)buffer;
        ssize_t ip_header_len = 4 * ip_header->ip_hl;
        /* u_int8_t *icmp_packet = buffer + 4 * ip_header_len; */
        /* struct icmp *icmp_header = (struct icmp *)icmp_packet; */

        // Print out data
        std::cout << "===========================\n"
                  << "Recieved IP packet with ICMP content from: "
                  << sender_ip_str << "\n"
                  << "----------\n"
                     "IP Header:\n";

        printBytes(buffer, ip_header_len);

        std::cout << "----------\n"
                     "IP Data:\n";

        printBytes(buffer + ip_header_len, packet_len - ip_header_len);

        std::cout << "===========================\n";
    }
    return 0;
}
