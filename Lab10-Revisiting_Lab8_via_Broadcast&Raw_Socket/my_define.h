#include <iostream>
#include <string.h> // for memxxx
#include <unistd.h> // for read, write, close
#include <fcntl.h> // for open
#include <sys/socket.h>
#include <netinet/ip.h> // for ip header structure
#include <arpa/inet.h> // for inet_pton

#define frag_size 1300
#define buf_size 34000
#define payload_size (frag_size-sizeof(my_header)-sizeof(ip))
#define err_quit(m) { perror(m); exit(-1); }

struct my_header {
    int file_total_len;
    int file_num;
    int frag_num;
    int frag_len;
    my_header() {}
    my_header(int file_n, int frag_n, int total_l, int frag_l) {
        file_num = file_n;
        frag_num = frag_n; 
        file_total_len = total_l;
        frag_len = frag_l;
    }
    void set (int file_n, int frag_n, int total_l, int frag_l) {
        file_num = file_n;
        frag_num = frag_n; 
        file_total_len = total_l;
        frag_len = frag_l;
    }
}__attribute__((packed));

struct my_packet {
    ip ip_header;
    my_header my_hdr;
    char payload[payload_size];
    my_packet () { memset(payload, 0, payload_size); }

    void add_ip_header (in_addr broadcast_addr, uint16_t &id) {
        ip_header.ip_hl = sizeof(ip)>>2;
        ip_header.ip_v = 4;
        ip_header.ip_tos = 0;
        ip_header.ip_len = (sizeof(ip) + sizeof(my_header) + my_hdr.frag_len);
        ip_header.ip_id = id++;
        ip_header.ip_off = 0;
        ip_header.ip_ttl = 255;
        ip_header.ip_p = 161;
        ip_header.ip_sum = 0;
        ip_header.ip_src = broadcast_addr;
        ip_header.ip_dst = broadcast_addr;

        return;
    }
}__attribute__((packed));

// calculate checksum
unsigned short cksum(void *in, int sz) {
    long sum = 0; 
    unsigned short *ptr = (unsigned short*)in;
    for (; sz > 1; sz -= 2) sum += *ptr++;
    while (sum >> 16) sum = (sum & 0xffff) + (sum>>16);
    return ~sum;
}