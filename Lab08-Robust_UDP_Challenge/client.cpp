#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>

#define header_size 21
#define frag_size 1000
#define buf_size 34000
#define payload_size (frag_size-header_size)
#define err_quit(m) { perror(m); exit(-1); }

using namespace std;

char folder_path[200-20];

struct my_file {
	int id, len;
	char buf[buf_size];

	my_file() {};
	my_file(int i) { open_file(i); }

	// open file with id i
	void open_file(int i) {
		id = i;
		char file_name[200];
		
		memset(buf, '\0', buf_size);
		memset(file_name, '\0', sizeof(file_name));
		sprintf(file_name, "%s/%06d", folder_path, id);

		int oo = open(file_name, O_RDONLY);		
		len = read(oo, buf, buf_size);
		close(oo);
	}
};

my_file f[1000];

int main(int argc, char *argv[]) {
	if(argc < 5) {
		return -fprintf(stderr, "usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>\n", argv[0]);
	}
	static int 		s;
	static struct 	sockaddr_in sin;
	int 			num_file = atoi(argv[2]);
	
	// set non-blocking
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	// set basic information of socket
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(atoi(argv[3]));
	if(inet_pton(AF_INET, argv[4], &sin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}
	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) err_quit("socket");
 
	// set socket option
	int snd_size = 1024*1024;
    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &snd_size, sizeof(snd_size)) < 0) err_quit("failed to set send buffer size");
    
	// connect
	connect(s, (sockaddr*)(&sin), sizeof(sin));

	// open all the files
	strcpy(folder_path, argv[1]);
	for (int i = 0; i < num_file; i++) f[i].open_file(i);
	
	// send all the files once
	for (int id = 0; id < num_file; id++) {
		char *now = f[id].buf;
		char *the_end = f[id].buf + f[id].len;
		for (int frag = 0; now < the_end; now += payload_size, frag++) {
			// prepare sending packet
			char snd_msg[frag_size], cpy_msg[payload_size];
			memset(snd_msg, '\0', frag_size);
			memset(cpy_msg, '\0', payload_size);
			// fill the self-defined header
			sprintf(snd_msg, "%6d %6d %6d ", id+ frag*1000, f[id].len, min(payload_size, (int)(the_end - now)));
			// get data
			memcpy(cpy_msg, now, min(payload_size, (int)(the_end - now)));
			// assemble self-defined header and data
			strcat(snd_msg, cpy_msg);
			
			if (send(s, snd_msg, strlen(snd_msg), MSG_NOSIGNAL) < 0) {
				close(s);
				exit(0);
			}
		}
	}

	// send data
	while(1) {
		char reply[frag_size];
		memset(reply, '\0', frag_size);
		// receive request
		if (recv(s, reply, frag_size, 0) == 1)  {
			close(s);
			exit(0);
		}
		char *r = reply, *get_num;
		int id = atoi(strtok_r(r, " ", &r)); /* get wanted file id */
		while ((get_num = strtok_r(r, " ", &r)) != NULL) { /* get wanted fragment id */
			int frag = atoi(get_num);

			// calculate exact position of wanted data and its length
			int index = frag * payload_size;
			int frag_len = f[id].len - index;
			if (frag_len <= 0) break;

			// prepare sending packet
			char snd_msg[frag_size], cpy_msg[payload_size];
			memset(snd_msg, '\0', frag_size);
			memset(cpy_msg, '\0', payload_size);
			// fill the self-defined header
			sprintf(snd_msg, "%6d %6d %6d ", id+ frag*1000, f[id].len, min(payload_size, f[id].len-index));
			// get the wanted data
			memcpy(cpy_msg, f[id].buf+index, min(payload_size, frag_len));
			// assemble self-defined header and the wanted data
			strcat(snd_msg, cpy_msg);

			if (send(s, snd_msg, strlen(snd_msg), MSG_NOSIGNAL) < 0) {
				close(s);
				exit(0);
			}			
		}
	}

	close(s);

	return 0;
}