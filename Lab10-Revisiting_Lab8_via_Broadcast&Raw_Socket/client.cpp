#include "my_define.h"
using namespace std;

char folder_path[200-20];

struct my_file {
	int id, len;
	char buf[buf_size];

	my_file() {};
	my_file(int i) {open_file(i);}

	void open_file(int i)
	{
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
	if(argc < 4) {
		cout << "./client <path-to-read-files> <total-number-of-files> <broadcast-address>" << endl;
		return -1;
	}
	
	static int 		s;
	static struct 	sockaddr_in sin;
	int 			num_file = atoi(argv[2]);
	uint16_t 		ip_id = 1;
	
	strcpy(folder_path, argv[1]);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = 5212;
	if(inet_pton(AF_INET, argv[3], &sin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}

	// use raw socket
	if((s = socket(AF_INET, SOCK_RAW, 161)) < 0) err_quit("socket");
	
	// set socket option -- broadcast and raw socket
	int on = 1;
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) err_quit("set broadcast");
    if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) err_quit("set ID_HDRINCL");

	// connect
	connect(s, (sockaddr*)(&sin), sizeof(sin));

	// open all the files
	for (int i = 0; i < num_file; i++) f[i].open_file(i);
	
	// send the data
	for (int t = 0 ; t < 5; t++) {
		for (int id = 0; id < num_file; id++) {
			char *now = f[id].buf;
			char *the_end = f[id].buf + f[id].len;
			for (int frag = 0; now < the_end; now += payload_size, frag++) {
				my_packet pack;
				pack.my_hdr.set(id, frag, f[id].len, min((int)payload_size, (int)(the_end - now)));
				memcpy(pack.payload, now, min((int)payload_size, (int)(the_end - now)));

				pack.add_ip_header(sin.sin_addr, ip_id);
				pack.ip_header.ip_sum = cksum(&pack, pack.ip_header.ip_len);
				
				// terminate if server closed
				if (send(s, &pack, pack.ip_header.ip_len, MSG_NOSIGNAL) < 0) {
					close(s);
					err_quit("failed to send");
				}
			}
			// assume that the server needs time to process the data
			usleep(2000);
		}
		// assume that the server needs time to process the data
		sleep(1);
	}

	close(s);

	return 0;
}