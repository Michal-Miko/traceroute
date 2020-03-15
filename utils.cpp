#include "utils.h"
#include <arpa/inet.h>
#include <cassert>
#include <ios>
#include <netinet/ip_icmp.h>
#include <strings.h>
#include <sys/socket.h>

void printBytes(unsigned char *buff, ssize_t length) {
    // Store current flag state
    std::ios_base::fmtflags f(std::cout.flags());

    // Print bytes in rows of 10
    for(ssize_t i = 0; i < length; i++, buff++) {
        if(i % 10 == 0 && i != 0)
            std::cout << '\n';
        std::cout << std::hex << std::setfill('0') << std::setw(2)
                  << (int)*buff << ' ';
    }
    std::cout << '\n';

    // Restore previous flags
    std::cout.flags(f);
}


u_int16_t computeICMPChecksum(const void *buff, int length) {
    u_int32_t sum;
    const u_int16_t *ptr = (const u_int16_t *)buff;
    assert(length % 2 == 0);
    for(sum = 0; length > 0; length -= 2)
        sum += *ptr++;
    sum = (sum >> 16) + (sum & 0xffff);
    return (u_int16_t)(~(sum + (sum >> 16)));
}


struct icmp createICMPHeader(short type, short code, int id, int seq) {
    struct icmp header;
    header.icmp_type = type;
    header.icmp_code = code;
    header.icmp_hun.ih_idseq.icd_id = id;
    header.icmp_hun.ih_idseq.icd_seq = seq;
    header.icmp_cksum = 0;
    header.icmp_cksum =
      computeICMPChecksum((u_int16_t *)&header, sizeof(header));

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
    ssize_t bytes_sent = sendto(socket,
                                &header,
                                sizeof(header),
                                0,
                                (struct sockaddr *)&recipient,
                                sizeof(recipient));

    return bytes_sent;
}
