#include "my_define.h"
using namespace std;

char folder_path[200-20];

struct my_file{
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
	
	// setvbuf(stdin, NULL, _IONBF, 0);
	// setvbuf(stderr, NULL, _IONBF, 0);
	// setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = 5212;
	if(inet_pton(AF_INET, argv[3], &sin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}

	if((s = socket(AF_INET, SOCK_RAW, 161)) < 0)
	// if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");
	
	// int snd_size = 1024*1024;
    // if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &snd_size, sizeof(snd_size)) < 0) err_quit("failed to set send buffer size");
    int on = 1;
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) err_quit("set broadcast");
    if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) err_quit("set ID_HDRINCL");

	connect(s, (sockaddr*)(&sin), sizeof(sin));

	for (int i = 0; i < num_file; i++) f[i].open_file(i);
	
	for (int t = 0 ; t < 5; t++) {
		for (int id = 0; id < num_file; id++)
		{
			char *now = f[id].buf;
			char *the_end = f[id].buf + f[id].len;
			for (int frag = 0; now < the_end; now += payload_size, frag++)
			{
				my_packet pack;
				pack.my_hdr.set(id, frag, f[id].len, min((int)payload_size, (int)(the_end - now)));
				memcpy(pack.payload, now, min((int)payload_size, (int)(the_end - now)));

				pack.add_ip_header(sin.sin_addr, ip_id);
				pack.ip_header.ip_sum = cksum(&pack, pack.ip_header.ip_len);
				
				
					if (send(s, &pack, pack.ip_header.ip_len, MSG_NOSIGNAL) < 0)
					{
						close(s);
						err_quit("failed to send");
					}
				// cout << sizeof(ip) << " " << sizeof(my_header) << endl;
				// cout << id << " " << frag << endl;
				// cout << pack.ip_header.ip_len << " " << pack.my_hdr.frag_len << endl;
			}
			// cout << "send " << id << endl;
			usleep(2000);
		}
		sleep(1);
	}

	close(s);

	return 0;
}