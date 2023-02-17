#include "function.h"

using namespace std;

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: ./dns <port-number> <path/to/the/config/file>\n");
        return -1;
    }
    int					srv, connfd;
	sockaddr_in	        cliaddr, servaddr;
	socklen_t			clilen = sizeof(cliaddr);
    
    srv = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0
	servaddr.sin_port        = htons(atoi(argv[1]));

	if (bind(srv, (const sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
    {
		cout << "failed to bind" << endl;
		return -1;
	}
    int conf = open(argv[2], O_RDONLY);
    config my_conf;
    my_conf.read_config(conf);
    // my_conf.print_();

    while (1) {
        DNS_request request;
        DNS_reply reply;
        int         D_len;
        if ((D_len = recvfrom(srv, &request, sizeof(DNS_request), 0, (sockaddr *)&cliaddr, &clilen)) < 0) {
            cout << "can't recvfrom socket srv" << endl;
            return -1;
        }
        request.process_remain();
        request.to_host_endian();

        reply.add_question(request.question);
        int reply_from = strlen(request.question.QNAME) + sizeof(Question_const) + 1; // +1 for 00
        int domain_index = my_conf.get_domain(request.question.QNAME);
        
        if (domain_index < 0) {
            /* resolve unknown domain */ 
            /* send dns request to "forward ip" indicate in config*/
            cout << "in resolve" << endl;
            
            sockaddr_in rin;
            rin.sin_family = AF_INET;
            rin.sin_port = htons(53);
            if(inet_pton(AF_INET, my_conf.forwardIP, &rin.sin_addr) != 1) {
                cout << "** cannot convert IPv4 address for " << my_conf.forwardIP << endl;
                return -1;
            }
            int resolve = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

            connect(resolve, (sockaddr*)(&rin), sizeof(rin));

            request.to_network_endian();
            write(resolve, &request, D_len);
            int reply_len = read(resolve, &reply, sizeof(DNS_reply));
            sendto(srv, &reply, reply_len, 0, (sockaddr*)&cliaddr, clilen);
            continue;
        }

        if (request.check_nip(my_conf.dom[domain_index].domain_name)) {
            /* do nip */
            reply.set_header(request.header, true);
            reply_from = reply.add_nip(reply_from, request);
            reply.header.to_network_endian();
            sendto(srv, &reply, sizeof(Header)+reply_from, 0, (sockaddr *)&cliaddr, clilen);
            continue;
        }

        
        
        reply.set_header(request.header, false);
        vector<int> ans_content, addi_content;
        my_conf.dom[domain_index].get_detail(request.question, ans_content);
        reply_from = reply.add_content(my_conf.dom[domain_index], ans_content, reply_from, request.question);

        if (request.question.qc.QTYPE != 2) 
            reply_from = reply.add_author(my_conf.dom[domain_index], reply_from);

        if (request.question.qc.QTYPE == 2 || request.question.qc.QTYPE == 15) // NX, MX
            my_conf.dom[domain_index].get_addi(ans_content, addi_content);

        reply_from = reply.add_addi(addi_content, my_conf.dom[domain_index], reply_from);

        reply.header.to_network_endian();
        
        sendto(srv, &reply, sizeof(Header)+reply_from, 0, (sockaddr *)&cliaddr, clilen);
    }


    return 0;
}