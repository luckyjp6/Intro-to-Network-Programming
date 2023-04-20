#include "my_define.h"
using namespace std;

char    file_content[1000][buf_size];
int     file_len[1000] = {0};
int     file_total[1000] = {0};
bool    file_get[1000][100] = {false};
bool    finish[1000] = {0};

int main(int argc, char *argv[]) {
	if(argc < 4) {
		return -fprintf(stderr, "./server <path-to-store-files> <total-number-of-files> <broadcast-address>\n");
	}
    
    int             s;
	sockaddr_in     sin, csin;
    socklen_t       csinlen = sizeof(csin);
    int             num_file = atoi(argv[2]);
    int             done = 0, reply_times = 0;
    char            buf[frag_size];

	if((s = socket(AF_INET, SOCK_RAW, 161)) < 0) /* 161 is a protocal number that are not frequently used */
		err_quit("socket");

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = 5212;
    if (inet_pton(AF_INET, argv[3], &sin.sin_addr) < 0) err_quit("inet_pton");
 
    // set socket option --broadcast
    int on = 1;
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) err_quit("set broadcast");

    // bind
	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0) err_quit("bind");

    for (int i = 0; i < 1000; i++) memset(file_content, '\0', buf_size);

    // keep receiving packet until all the files are collected
	while(done < num_file) {
        my_packet pack;
        int rcv_len;
        // directly use self-defined struct "my_packet" to store the packet we get
		recvfrom(s, &pack, frag_size, 0, (struct sockaddr*) &csin, &csinlen);
        
        // store the data if it is needed
        if (!finish[pack.my_hdr.file_num] && !file_get[pack.my_hdr.file_num][pack.my_hdr.frag_num]) {
            if (strlen(pack.payload) != pack.my_hdr.frag_len) continue;

            file_get[pack.my_hdr.file_num][pack.my_hdr.frag_num] = true;
            
            memcpy(file_content[pack.my_hdr.file_num] + pack.my_hdr.frag_num*payload_size, pack.payload, pack.my_hdr.frag_len);
            file_total[pack.my_hdr.file_num] = pack.my_hdr.file_total_len;
            file_len[pack.my_hdr.file_num] += pack.my_hdr.frag_len;

            // if the entire file is collected, save it
            if (file_len[pack.my_hdr.file_num] >= file_total[pack.my_hdr.file_num]) {
                cout << "done " << pack.my_hdr.file_num << endl;
                finish[pack.my_hdr.file_num] = true;
                done++;
                
                char file_name[200];
                sprintf(file_name, "%s/%06d", argv[1], pack.my_hdr.file_num);
                int oo = open(file_name, O_WRONLY | O_CREAT, 0777);
                write(oo, file_content[pack.my_hdr.file_num], file_total[pack.my_hdr.file_num]);
                close(oo);
            }
        }
	}

	close(s);
    return 0;
}