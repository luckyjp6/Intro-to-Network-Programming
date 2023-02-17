#ifndef FUNCTION_H
#define FUNCTION_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define MSG_MAX 100000
#define NAME_LENGTH 10000

/* 
1:  A x
2:  NS
5:  CNAME x
6:  SOA x
15: MX
16: TXT x
28: AAAA x
*/

void print_str(char *s) {
        char *p = s;
        int now = 0;
        char out[NAME_LENGTH];
        while(1) {
            int len = (int)*p;
            if (len == 0) break;
            std::cout << len << " ";
            p++;
            memcpy(out + now, p, len);
            p += len; now += len;
        }
        std::cout << out << std::endl;
    }

struct Header_flag {
    uint    QR : 1;     // query(0) or reponse(1)
    uint    Opcode : 4;
    uint    AA : 1;
    uint    TC : 1;
    uint    RD : 1;
    uint    RA : 1;
    uint    Z : 3;
    uint    RCODE : 4;  // error code

    // Header_flag() {}
    // Header_flag(uint16_t in) {

    // }
}__attribute__((packed));
struct Header {
    uint16_t     ID;
    uint16_t     flg; 
    uint16_t     QDCOUNT : 16; // number of questions
    uint16_t     ANCOUNT : 16; // number of answers
    uint16_t     NSCOUNT : 16; // number of authority(s)
    uint16_t     ARCOUNT : 16; // number of additional records

    void to_host_endian() {
        QDCOUNT = ntohs(QDCOUNT);
        ANCOUNT = ntohs(ANCOUNT);
        NSCOUNT = ntohs(NSCOUNT);
        ARCOUNT = ntohs(ARCOUNT);
        flg     = ntohs(flg);
    }
    void to_network_endian() {
        QDCOUNT = htons(QDCOUNT);
        ANCOUNT = htons(ANCOUNT);
        NSCOUNT = htons(NSCOUNT);
        ARCOUNT = htons(ARCOUNT);
        flg     = htons(flg);
    }
    int get_Opcode() {
        uint16_t tmp = flg;
        tmp = tmp << 1;
        tmp = tmp >> 12;
        return tmp;
    }
    void print_() {
        using namespace std;
        cout << "ID: " << ID << endl;
        cout << "flg: " << flg << endl;
        // Header_flag ff = static_cast<Header_flag>(flg);
        // cout << "QR: " << ff.QR << endl;
        // cout << "Opcode: " << ff.Opcode << endl;
        // cout << "AA: " << ff.AA << endl;
        // cout << "TC: " << ff.TC << endl;
        // cout << "RD: " << ff.RD << endl;
        // cout << "RA: " << ff.RA << endl;
        // cout << "Z: " << ff.Z << endl;
        // cout << "RCODE: " << ff.RCODE << endl;
        cout << "QDCOUNT: " << QDCOUNT << endl;
        cout << "ANCOUNT: " << ANCOUNT << endl;
        cout << "NSCOUNT: " << NSCOUNT << endl;
        cout << "ARCOUNT: " << ARCOUNT << endl;
        cout << endl;
    }
}__attribute__((packed));
struct Question_const {
    uint    QTYPE2 : 8 = 0;
    uint    QTYPE : 8;
    uint    QCLASS2 : 8 = 0;
    uint    QCLASS : 8;
};
struct Question {
    char    QNAME[NAME_LENGTH];  
    Question_const qc;
    void print_() {
        using namespace std;
        cout << "question: " << QNAME << endl;
        cout << "QTYPE: " << qc.QTYPE << endl;
        cout << "QCLASS: " << qc.QCLASS << endl;
    }
    // void to_host_endian() {
    //     QTYPE = ntohs(QTYPE);
    //     QCLASS = ntohs(QCLASS);
    // }
    
};
struct  RR_const {
    uint    TYPE1 : 8;
    uint    TYPE : 8;
    uint    CLASS1 : 8;
    uint    CLASS : 8;
    uint    TTL : 32;
    uint    RDLENGTH : 16;
}__attribute__((packed));
struct RR {
    char    name[NAME_LENGTH];
    RR_const rc;
    char    RDATA[MSG_MAX];
};

struct details {
    char        NAME[NAME_LENGTH];
    RR_const    r;
    char        RDATA[MSG_MAX];
    uint32_t    RDATA_int_4;
    in6_addr    RDATA_int_6;

    details () {}
    details (char *msg, char* domain_name) {
        r.TYPE1 = 0; r.CLASS1 = 0; r.RDLENGTH = 0;
        char *tmp_name = strtok_r(msg, ",", &msg);
        if (strcmp(tmp_name, "@") == 0) {
            strcpy(NAME, domain_name);
        }else {
            NAME[0] = strlen(tmp_name);
            sprintf(NAME+1, "%s%s", tmp_name, domain_name);
        }
        
        r.TTL = atoi(strtok_r(msg, ",", &msg)); 
        r.CLASS = 1;  strtok_r(msg, ",", &msg); 
        char *TYPE_char = strtok_r(msg, ",", &msg);
        if (strcmp(TYPE_char, "A") == 0) r.TYPE = 1;
        else if (strcmp(TYPE_char, "NS") == 0) r.TYPE = 2;
        else if (strcmp(TYPE_char, "CNAME") == 0) r.TYPE = 5;
        else if (strcmp(TYPE_char, "SOA") == 0) r.TYPE = 6;
        else if (strcmp(TYPE_char, "MX") == 0) r.TYPE = 15;
        else if (strcmp(TYPE_char, "TXT") == 0) r.TYPE = 16;
        else if (strcmp(TYPE_char, "AAAA") == 0) r.TYPE = 28;
     
        memset(RDATA, 0, MSG_MAX);
        if (r.TYPE == 1 || r.TYPE == 28) { // A, AAAA
            char *data = strtok_r(msg, "\r\n", &msg);
            strcpy(RDATA, data);
            if (r.TYPE == 1) {
                inet_pton(AF_INET, RDATA, &RDATA_int_4);
                r.RDLENGTH = sizeof(RDATA_int_4);
            }
            else if (r.TYPE == 28) {
                inet_pton(AF_INET6, RDATA, &RDATA_int_6);
                r.RDLENGTH = sizeof(RDATA_int_6);
            }
            
        }else if (r.TYPE == 6) { // SOA
            for (int i = 0; i < 2; i++) {
                char *data = strtok_r(msg, " ", &msg);
                char *dot;
                while ((dot = strtok_r(data, ".", &data))) {
                    int dot_len = strlen(dot);
                    RDATA[r.RDLENGTH++] = dot_len;
                    memcpy(RDATA + r.RDLENGTH, dot, dot_len);
                    r.RDLENGTH += dot_len;
                    memset(dot, 0, strlen(dot));
                }
                r.RDLENGTH++;
            }
            char *now;
            while (now = strtok_r(msg, " \0", &msg)) {
                uint32_t to_int = atoi(now);
                to_int = htonl(to_int);
                memcpy(RDATA + r.RDLENGTH, &to_int, sizeof(uint32_t));
                r.RDLENGTH += sizeof(uint32_t);
                memset(now, 0, strlen(now));
            }
        }else if (r.TYPE == 15) { // MX
            char *now = strtok_r(msg, " ", &msg);
            uint16_t to_int = atoi(now);
            to_int = htons(to_int);
            memcpy(RDATA, &to_int, sizeof(uint16_t));
            r.RDLENGTH += sizeof(uint16_t);

            char *dot;
            while ((dot = strtok_r(msg, ".", &msg))) {
                int dot_len = strlen(dot);
                RDATA[r.RDLENGTH++] = dot_len;
                memcpy(RDATA + r.RDLENGTH, dot, dot_len);
                r.RDLENGTH += dot_len;
                memset(dot, 0, strlen(dot));
            }
            r.RDLENGTH++;
        }
        else if (r.TYPE == 16) { // TXT
            RDATA[0] = strlen(msg)-2;
            memcpy(RDATA+1, msg+1, strlen(msg)-2);
            r.RDLENGTH = strlen(msg)-1;
            std::cout << r.RDLENGTH << std::endl;
        }
        else {
            char *data = strtok_r(msg, "\r\n", &msg);
            char *dot;
            while ((dot = strtok_r(data, ".", &data))) {
                int dot_len = strlen(dot);
                RDATA[r.RDLENGTH++] = dot_len;
                memcpy(RDATA + r.RDLENGTH, dot, dot_len);
                r.RDLENGTH += dot_len;
                memset(dot, 0, strlen(dot));
            }
            r.RDLENGTH++;
        }
    }
    void to_network_endian() {
        // RDATA_int = htonl(RDATA_int);
        r.TTL = htonl(r.TTL);
        r.RDLENGTH = htons(r.RDLENGTH);
    }
    void print_() {
        using namespace std;
        cout << "subdomain name: " << NAME << endl;
        cout << "TTL:" << ntohl(r.TTL) << endl;
        cout << "CLASS: " << r.CLASS << endl;
        cout << "TYPE: " << r.TYPE << endl;
        cout << "RDLENGTH: " << ntohs(r.RDLENGTH) << endl;
        if (r.TYPE != 1 && r.TYPE != 28) cout << "RDATA: " << RDATA << endl;
    }
};
struct domain {
    char    domain_name[NAME_LENGTH];
    char    name[NAME_LENGTH];
    char    path[NAME_LENGTH];
    std::vector<details> det;

    domain () {}
    domain (char *msg) {
        char *pos = strtok_r(msg, ",", &msg);
        strcpy(name, pos);
        strcpy(path, msg);
    }
    void read_domain () {
        int file = open(path, O_RDONLY);
        char msg[MSG_MAX];
        read(file, msg, MSG_MAX);
        
        char    *m = msg, *pos;
        int     now_at = 0;
        char    *for_dot = strtok_r(m, "\r\n", &m), *dot;
        while(dot = strtok_r(for_dot, ".", &for_dot)) {
            int dot_len = strlen(dot);
            domain_name[now_at++] = dot_len;
            memcpy(domain_name + now_at, dot, dot_len);
            now_at += dot_len;
            memset(dot, 0, strlen(dot));
        }
        while(pos = strtok_r(m, "\r\n", &m)) {
            details d(pos, domain_name);
            d.to_network_endian();
            det.push_back(d);
            memset(pos, 0, strlen(pos));
        }
    }
    void get_detail (Question q, std::vector<int> &ans_content) {
        for (int i = 0; i < det.size(); i++) {
            if (strcmp(det[i].NAME, q.QNAME) == 0
                && q.qc.QTYPE == det[i].r.TYPE) ans_content.push_back(i);
        }
    }
    void get_NS (std::vector<int> &ns) {
        for (int i = 0; i < det.size(); i++) {
            if (det[i].r.TYPE == 2) ns.push_back(i);
        }
    }
    void get_SOA (std::vector<int> &soa) {
        for (int i = 0; i < det.size(); i++) {
            if (det[i].r.TYPE == 6) soa.push_back(i);
        }
    }
    void get_addi (std::vector<int> ans_constent, std::vector<int> &addi_content) {
        for (auto ans:ans_constent) {
            char *wanted = det[ans].RDATA;
            if (det[ans].r.TYPE == 15) wanted += sizeof(uint16_t);
            for (int now = 0; now < det.size(); now++) {
                if (det[now].r.TYPE != 1 && det[now].r.TYPE != 28 && det[now].r.TYPE != 5) continue;
                if (det[ans].r.RDLENGTH != strlen(det[now].NAME))
                if (memcmp(det[now].NAME, wanted, det[ans].r.RDLENGTH) == 0) {
                    addi_content.push_back(now);
                }
            }
        }
        sort(addi_content.begin(), addi_content.end());
        int last_one = -1;
        for (int i = 0; i < addi_content.size(); ) {
            if (addi_content[i] == last_one) addi_content.erase(addi_content.begin()+i);
            else {
                last_one = addi_content[i];
                i++;
            }
        }
    }
    void print_ () {
        using namespace std;
        cout << "domain name: " << domain_name << endl;
        cout << "path: " << path << endl;
        for (auto d:det) d.print_();
        cout << endl;
    }
};
struct config {
    char forwardIP[100];
    std::vector<domain> dom;

    void read_config(int fd) {
        char msg[MSG_MAX];
        read(fd, msg, MSG_MAX);

        char *m = msg;
        char *pos = strtok_r(m, "\r\n", &m);
        strcpy(forwardIP, pos);
        while (pos = strtok_r(m, "\r\n", &m)) {
            domain d(pos);
            d.read_domain();
            dom.push_back(d);
            memset(pos, 0, strlen(pos));
        }
    }
    int get_domain(char *wanted) {
        std::string s(wanted);
        for (int i = 0; i < dom.size(); i++) {
            if (s.find(dom[i].domain_name) != std::string::npos) return i;
        }
        return -1;
    }
    void print_() {
        using namespace std;
        cout << "forwardIP: " << forwardIP << endl;
        for (auto d: dom) {
            d.read_domain();
            cout << d.domain_name << endl;
            d.print_();
        }
    }
};

struct DNS_request {
    Header      header;
    char        remain[MSG_MAX];
    Question    question;
    uint32_t    nip;

    void process_remain() { 
        // question
        char *r = remain, empty[]="0"; empty[0] = 0;
        char *pos = strtok_r(r, empty, &r);
        strcpy(question.QNAME, remain);
        
        r++;
        question.qc.QTYPE = (int)r[1]; //question.QTYPE2 = (int)r[1];
        question.qc.QCLASS = (int)r[3]; //question.QCLASS2 = (int)r[3];

    }
    bool check_nip(char *domain_name) {
        // char    *num, *others;
        // sscanf(question.QNAME, "%[1-9]%s)", num, others);
        // print_str(num);
        // print_str(others);
        // std::cout << "num: " << num << std::endl;
        // std::cout << "others: " << others << std::endl;
        char *now = question.QNAME;
        char tmp_ip[NAME_LENGTH] = {0};
        int tmp_len = 0;
        for (int i = 0; i < 4; i++) {
            int len = *now;
            if (*now > 3) return false;
            now++;
            for (int j = 0; j < len; j++) {
                if (*now < '0' || *now > '9') return false;
                tmp_ip[tmp_len++] = *now;
                now++;
            }
            if (i != 4-1) tmp_ip[tmp_len++] = '.';
        }
        
        inet_pton(AF_INET, tmp_ip, &nip);

        // check domain name
        int len = *now;
        now++;
        now += len;
        if (strcmp(now, domain_name) != 0) return false;
        
        return true;
    }
    void to_host_endian() {
        header.to_host_endian();
        // question.to_host_endian();
    }
    void to_network_endian() {
        header.to_network_endian();
        // question.to_host_endian();
    }
    void print_() {
        header.print_();
        question.print_();
    }
};
struct  DNS_reply {
    Header      header;
    char        content[MSG_MAX];

    DNS_reply () { memset(content, 0, MSG_MAX); }

    void set_header(Header rq, int is_author) {
        header.ID = rq.ID;
        header.flg = rq.flg + (1<<15) + (is_author << 10); // set QR and AA
        // header.flg.QR = 1;
        // header.flg.Opcode = rq.flg.Opcode;
        // header.flg.AA = is_author;
        // header.flg.TC = 0;
        // header.flg.RD = rq.flg.RD;
        // header.flg.RA = rq.flg.RA;
        // header.flg.Z = 0;
        // header.flg.RCODE = rq.flg.RCODE;
        header.QDCOUNT = 1;
        header.ANCOUNT = 0;
        header.NSCOUNT = 0;
        header.ARCOUNT = 0;
    }
    void add_question(Question q) {
        memcpy(content, q.QNAME, strlen(q.QNAME));        
        memcpy(content + strlen(q.QNAME)+1, &q.qc, sizeof(Question_const));
    }
    int add_content(domain dom, std::vector<int> ans_content, int from, Question q) {
        for (auto index:ans_content) {
            int name_len = strlen(dom.det[index].NAME);
            memcpy(content + from, dom.det[index].NAME, name_len); from += name_len+1;
            memcpy(content + from, &dom.det[index].r, sizeof(RR_const)); from += sizeof(RR_const);

            if (dom.det[index].r.TYPE == 1) 
                memcpy(content + from, &dom.det[index].RDATA_int_4, ntohs(dom.det[index].r.RDLENGTH)); 
            else if (dom.det[index].r.TYPE == 28)
                memcpy(content + from, &dom.det[index].RDATA_int_6, ntohs(dom.det[index].r.RDLENGTH));
            else 
                memcpy(content + from, dom.det[index].RDATA, ntohs(dom.det[index].r.RDLENGTH)); 

            from += ntohs(dom.det[index].r.RDLENGTH);
            header.ANCOUNT++;
        }
        return from;
    }
    int add_author(domain d, int from) {
        std::vector<int> author_content;
        if (header.ANCOUNT > 0) d.get_NS(author_content); // add NS
        else d.get_SOA(author_content);// add SOA
// for (auto i:author_content) std::cout << i << " "; std::cout << std::endl;
        for (auto index:author_content) {
            header.NSCOUNT++;
            int name_len = strlen(d.det[index].NAME);
            memcpy(content + from, d.det[index].NAME, name_len); from += name_len+1;
            memcpy(content + from, &d.det[index].r, sizeof(RR_const)); from += sizeof(RR_const);
            
            memcpy(content + from, d.det[index].RDATA, ntohs(d.det[index].r.RDLENGTH)); 
            from += ntohs(d.det[index].r.RDLENGTH);
        }
        
        return from;
    }
    int add_addi(std::vector<int> addi_content, domain dom, int from) {
        for (auto index:addi_content) {
            int name_len = strlen(dom.det[index].NAME);
            memcpy(content + from, dom.det[index].NAME, name_len); from += name_len+1;
            memcpy(content + from, &dom.det[index].r, sizeof(RR_const)); from += sizeof(RR_const);
            if (dom.det[index].r.TYPE == 1) 
                memcpy(content + from, &dom.det[index].RDATA_int_4, ntohs(dom.det[index].r.RDLENGTH)); 
            else if (dom.det[index].r.TYPE == 28)
                memcpy(content + from, &dom.det[index].RDATA_int_6, ntohs(dom.det[index].r.RDLENGTH));
            else if (dom.det[index].r.TYPE == 5)
                memcpy(content + from, dom.det[index].RDATA, ntohs(dom.det[index].r.RDLENGTH));
            from += ntohs(dom.det[index].r.RDLENGTH);
            header.ARCOUNT++;
        }
        return from;
    }
    int add_nip(int from, DNS_request rq) {
        int name_len = strlen(rq.question.QNAME);
        memcpy(content + from, rq.question.QNAME, name_len); from += name_len+1;
        RR_const rc;
        rc.TTL = 1; rc.TTL = ntohl(rc.TTL);
        rc.TYPE = 1; rc.TYPE1 = 0;
        rc.CLASS = 1; rc.CLASS1 = 0;
        rc.RDLENGTH = sizeof(uint32_t); rc.RDLENGTH = ntohs(rc.RDLENGTH);
        memcpy(content + from, &rc, sizeof(RR_const)); from += sizeof(RR_const);
        memcpy(content + from, &rq.nip, sizeof(uint32_t)); from += sizeof(uint32_t);
        header.ANCOUNT++;
        return from;
    }
}__attribute__((packed));

#endif