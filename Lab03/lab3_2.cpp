#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>

using namespace std;

char garbage[2000];

static struct timeval _t0;
static unsigned long long bytesent = 0;

double tv2s(struct timeval *ptv) {
	return 1.0*(ptv->tv_sec) + 0.000001*(ptv->tv_usec);
}

void handler(int s) {
	struct timeval _t1;
	double t0, t1;
	gettimeofday(&_t1, NULL);
	t0 = tv2s(&_t0);
	t1 = tv2s(&_t1);
	fprintf(stderr, "\n%lu.%06lu %llu bytes sent in %.6fs (%.6f Mbps; %.6f MBps)\n",
		_t1.tv_sec, _t1.tv_usec, bytesent, t1-t0, 8.0*(bytesent/1000000.0)/(t1-t0), (bytesent/1000000.0)/(t1-t0));
	exit(0);
}

int main(int argc, char *argv[]) {
    int sock_fd, connection;
    struct sockaddr_in servaddr;

    char input_addr[] = "127.0.0.1";
    
    int total = 0;
    if (argc == 1) {
        cout << "usage: .out <sending speed>\n";
        return -1;
    }

    bool under = false;
    int uu = 0;
    // get sending speed (float number)
    for (int i; i < strlen(argv[1]); i++) {
        if (argv[1][i] == '\0') break; 
        if (argv[1][i] == '.') {
            under = true;
            continue;
        }
        if (under) uu++;
        total *= 10;
        total += argv[1][i] - '0';
        if (uu == 2) break;
    }

    if (uu == 0) total *= 100;
    else if (uu == 1) total *= 10;
    total *= 10000;

    // build connection
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(10003);
    if (inet_pton(AF_INET, input_addr, &servaddr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    connection = connect(sock_fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (connection != 0) {
        cout << "connection error\n";
        return -1;
    }
    memset(garbage,'$',2000);
    
    char tmp[100];
    read(sock_fd, tmp, 100);
    
    // this program will keep sending useless data to the server until being terminated
    signal(SIGINT,  handler);
	signal(SIGTERM, handler);

	gettimeofday(&_t0, NULL);
    
    // to precisely controll the traffic speed, we must take the size of header(s) into account
    int header = 66;
    int packet_size = 500-header;
    int added = header + packet_size;

    struct timeval start, now;
    int loop_sent;
    while (true) {
        // per sec.
        gettimeofday(&start, NULL);
        loop_sent = 0;
        // send all data that should be send in one second
        while (true) {
            write(sock_fd, garbage, packet_size);
            usleep(5);
            loop_sent += added;
            if (loop_sent > total) {
                break;
            }
        }
        bytesent += loop_sent;
        // count how many time left after we send the data
        // make sure each period is one second
        gettimeofday(&now, NULL);
        if (start.tv_sec == now.tv_sec) usleep(start.tv_usec - now.tv_usec + 999990);
        else if (start.tv_sec+1 == now.tv_sec) usleep(start.tv_usec - now.tv_usec - 10);
        else{
            cout << "out of time!\n";
            return -1;
        }
    }
    
    close(connection);
    return 0;
}
// test case (.sh)
/*
g++ lab3_2.cpp
timeout 20 ./a.out 5; sleep 2 
timeout 20 ./a.out 4.5; sleep 2
timeout 20 ./a.out 4; sleep 2
timeout 20 ./a.out 3.5; sleep 2
timeout 20 ./a.out 3; sleep 2
timeout 20 ./a.out 2.5; sleep 2
timeout 20 ./a.out 2; sleep 2
timeout 20 ./a.out 1.5; sleep 2
timeout 20 ./a.out 1; sleep 2
*/