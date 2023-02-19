/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>


#define header_size 21
#define frag_size 1000
#define buf_size 34000
#define payload_size (frag_size-header_size)
#define err_quit(m) { perror(m); exit(-1); }

char    file_content[1000][buf_size];
int     file_len[1000] = {0};
int     file_total[1000] = {0};
bool    file_get[1000][100] = {false};

int main(int argc, char *argv[]) {
	if(argc < 4) {
		return -fprintf(stderr, "/server <path-to-store-files> <total-number-of-files> <port>\n");
	}
    
    int             s;
	sockaddr_in     sin, csin;
    socklen_t       csinlen = sizeof(csin);
    int             num_file = atoi(argv[2]);
    int             done = 0, reply_times = 0;
    char            buf[frag_size];
    bool            finish[1000];

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(atoi(argv[3]));

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

    struct timeval read_timeout;
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 10;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
	int rcv_size = 1024*1024;
    if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &rcv_size, sizeof(rcv_size)) < 0) err_quit("failed to set send buffer size");
    // int snd_size = 1024*1024;
    // if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &snd_size, sizeof(snd_size)) < 0) err_quit("failed to set send buffer size");
	
	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		err_quit("bind");

    for (int i = 0; i < 1000; i++) memset(file_content, '\0', buf_size);

    int     now_get, rcv_len, total_len, buf_len;
	while(done < num_file) {

        memset(buf, '\0', frag_size);
		while ((rcv_len = recvfrom(s, buf, frag_size, 0, (struct sockaddr*) &csin, &csinlen)) > 0)
        {
            sscanf(buf, "%6d %6d %6d ", &now_get, &total_len, &buf_len); 
         
            int index = now_get/1000;
            now_get %= 1000;
            if (!finish[now_get] && !file_get[now_get][index]) 
            {
                rcv_len -= header_size;
                if (rcv_len != buf_len) continue;

                file_get[now_get][index] = true;
                
                memcpy(file_content[now_get]+index*payload_size, buf+header_size, rcv_len);
                file_total[now_get] = total_len;
                file_len[now_get] += rcv_len;
   
                if (file_len[now_get] >= total_len)
                {
                    finish[now_get] = true;
                    done++;
                    
                    // printf("get %d niceeee, size: %d\n", now_get, file_total[now_get]);
                    char file_name[200];
                    sprintf(file_name, "%s/%06d", argv[1], now_get);
                    int oo = open(file_name, O_WRONLY | O_CREAT, 0777);
                    write(oo, file_content[now_get], total_len);
                    close(oo);
                }
            }
        }

        // send response   
        char num[8] = {0};
        for (int i = 0; i < num_file; i++)
        {
            char reply[frag_size];
            memset(reply, '\0', frag_size);
            if (finish[i]) continue;
            
            sprintf(num, "%d ", i);
            strcat(reply, num);

            int num_frag = (file_total[i] > 0) ? (file_total[i]/payload_size+1):100;
            for (int j = 0; j < num_frag; j++)
            {
                if (file_get[i][j]) continue;
                memset(num, '\0', 8);
                sprintf(num, "%d ", j);
                strcat(reply, num);
            }
            
            sendto(s, reply, strlen(reply), 0, (struct sockaddr*) &csin, csinlen);
        }
	}

    while (sendto(s, "f", 1, 0, (struct sockaddr*) &csin, csinlen) > 0);
	close(s);

    return 0;
}