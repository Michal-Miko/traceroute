// Michał Mikołajczyk
// 307371
// 2020-03-26

#ifndef UTILS_H_A8KC4E9C
#define UTILS_H_A8KC4E9C

#include <string>

typedef struct trace_response {
    unsigned ttl;
    unsigned ip_count;
    unsigned response_count;
    float response_times[3];
    std::string ip_addr[3];
} trace_response;

u_int16_t computeICMPChecksum(const void *buff, int length);
struct icmp createICMPHeader(u_int8_t type, u_int8_t code, u_int16_t id, u_int16_t seq);
ssize_t sendICMPPacket(const std::string &ip_addr,
                       const int &socket,
                       const struct icmp &header,
                       const int &ttl);
trace_response pingRouters(const std::string &ip_addr, unsigned short ttl, const int &socket);


#endif /* end of include guard: UTILS_H_A8KC4E9C */
