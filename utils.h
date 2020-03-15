#ifndef UTILS_H_A8KC4E9C
#define UTILS_H_A8KC4E9C

#include <iomanip>
#include <iostream>
#include <sys/types.h>


void printBytes(unsigned char *buff, ssize_t length);
u_int16_t computeICMPChecksum(const void *buff, int length);
struct icmp createICMPHeader(short packet_type);
ssize_t sendICMPPacket(const std::string &ip_addr,
                       const int &socket,
                       const struct icmp &header,
                       const int &ttl);


#endif /* end of include guard: UTILS_H_A8KC4E9C */
