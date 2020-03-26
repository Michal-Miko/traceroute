// Michał Mikołajczyk
// 307371
// 2020-03-26

#include "traceroute.h"
#include <arpa/inet.h>
#include <bits/stdint-uintn.h>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

u_int16_t computeICMPChecksum(const void *buff, int length) {
    u_int32_t sum;
    const u_int16_t *ptr = (const u_int16_t *)buff;
    assert(length % 2 == 0);
    for(sum = 0; length > 0; length -= 2)
        sum += *ptr++;
    sum = (sum >> 16) + (sum & 0xffff);
    return (u_int16_t)(~(sum + (sum >> 16)));
}


struct icmp createICMPHeader(u_int8_t type, u_int8_t code, u_int16_t id, u_int16_t seq) {
    struct icmp header;
    header.icmp_type = type;
    header.icmp_code = code;
    header.icmp_hun.ih_idseq.icd_id = id;
    header.icmp_hun.ih_idseq.icd_seq = seq;
    header.icmp_cksum = 0;
    header.icmp_cksum = computeICMPChecksum((u_int16_t *)&header, sizeof(header));

    return header;
}


ssize_t sendICMPPacket(const std::string &ip_addr,
                       const int &socket,
                       const struct icmp &header,
                       const int &ttl) {

    // Set up recipient's address
    struct sockaddr_in recipient;
    bzero(&recipient, sizeof(recipient));
    recipient.sin_family = AF_INET;
    inet_pton(AF_INET, ip_addr.c_str(), &recipient.sin_addr);

    // Set TTL
    setsockopt(socket, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

    // Send packet
    ssize_t bytes_sent =
      sendto(socket, &header, sizeof(header), 0, (struct sockaddr *)&recipient, sizeof(recipient));

    return bytes_sent;
}


trace_response pingRouters(const std::string &ip_addr, unsigned short ttl, const int &socket) {
    // Struct: ttl, ip_count, response_count, response_times[], ip_addr[]
    trace_response responses{ ttl, 0, 0, {}, {} };

    // Get PID for use in ICMP headers as unique id
    // (PID is too large, get the last 16 bits)
    const uint16_t pid = getpid() & 0xffff;

    // Send 3 requests
    for(int i = 0; i < 3; ++i) {
        // Create sequence id:
        // first 14 bits for TTL id,
        // last 2 bits for sequence number in current TTL
        u_int16_t seq = (ttl << 2) + i;
        struct icmp header = createICMPHeader(8, 0, pid, seq);
        sendICMPPacket(ip_addr, socket, header, ttl);
    }

    // Save request send time
    auto send_time = std::chrono::steady_clock::now();
    auto time_now = std::chrono::steady_clock::now();
    auto time_passed =
      std::chrono::duration_cast<std::chrono::microseconds>(time_now - send_time).count();

    // Check for requests until there are 3 responses or more than 1s has passed
    while(responses.response_count < 3 && time_passed < 1000000) {
        // Select setup
        fd_set descriptors;
        FD_ZERO(&descriptors);
        FD_SET(socket, &descriptors);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1000000 - time_passed;
        int ready = select(socket + 1, &descriptors, NULL, NULL, &tv);

        // Check for select error / timeout
        if(ready < 0) {
            std::cerr << "ERROR Select\n";
            return responses;
        } else if(ready == 0) {
            /* std::cerr << "Timeout, not enough responses.\n"; */
            return responses;
        }

        // Look through packets in the socket queue
        struct sockaddr_in sender;
        socklen_t sender_len = sizeof(sender);
        u_int8_t buffer[IP_MAXPACKET];
        ssize_t packet_len = recvfrom(socket,
                                      buffer,
                                      IP_MAXPACKET,
                                      MSG_DONTWAIT,
                                      (struct sockaddr *)&sender,
                                      &sender_len);
        while(packet_len > 0) {
            time_now = std::chrono::steady_clock::now();
            time_passed =
              std::chrono::duration_cast<std::chrono::microseconds>(time_now - send_time).count();

            // Get sender ip addr
            char sender_ip_str[20];
            inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str, sizeof(sender_ip_str));

            // Get icmp header
            struct ip *ip_header = (struct ip *)buffer;
            ssize_t ip_header_len = 4 * ip_header->ip_hl;
            u_int8_t *icmp_packet = buffer + ip_header_len;
            struct icmp *icmp_header = (struct icmp *)icmp_packet;
            ssize_t icmp_header_len = 8;

            // Type 11 - TTL exceeded, extract the original request IP, ICMP data
            if(icmp_header->icmp_type == 11) {
                ssize_t inner_packet = ip_header_len + icmp_header_len;
                ip_header = (struct ip *)(buffer + inner_packet);
                ip_header_len = 4 * ip_header->ip_hl;
                icmp_packet = buffer + inner_packet + ip_header_len;
                icmp_header = (struct icmp *)icmp_packet;
            }

            // Get icmp packet id, seq;
            uint16_t id = icmp_header->icmp_hun.ih_idseq.icd_id;
            uint16_t seq = icmp_header->icmp_hun.ih_idseq.icd_seq;
            uint16_t seq_ttl = seq >> 2;

            // Check if packet is a response to one of the last 3 requests
            if(id == pid && seq_ttl == ttl) {
                std::string newip = sender_ip_str;
                // Check if the current ip addr is a new one
                bool unique = true;
                for(unsigned i = 0; i < responses.ip_count; ++i)
                    if(responses.ip_addr[i].compare(newip) == 0)
                        unique = false;
                if(unique) {
                    responses.ip_addr[responses.ip_count] = newip;
                    responses.ip_count++;
                }
                // Update response time
                responses.response_times[responses.response_count] = time_passed / 1000.0;
                responses.response_count++;
            }

            packet_len = recvfrom(socket,
                                  buffer,
                                  IP_MAXPACKET,
                                  MSG_DONTWAIT,
                                  (struct sockaddr *)&sender,
                                  &sender_len);
        } // End of queue loop

        if(packet_len < 0 && errno != EWOULDBLOCK)
            std::cerr << "recvfrom error: " << std::strerror(errno) << '\n';
    } // End of select loop

    return responses;
}
