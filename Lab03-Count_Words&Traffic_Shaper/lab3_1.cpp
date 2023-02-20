#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int CC = 0;

int main(int argc, char *argv[]) {
    int sock_fd, connection;
    struct sockaddr_in servaddr;

    char input_addr[] = "140.113.213.213";

    // connect to the server
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(10002);
    if (inet_pton(AF_INET, input_addr, &servaddr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    connection = connect(sock_fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (connection != 0) {
        cout << "connection error\n";
        return -1;
    }
    char tmp[1000];
    read(sock_fd, tmp, 1000);
    // server will start send data after we send "GO"
    write(sock_fd, "GO\n", sizeof("GO\n")-1);

    char start[] = "==== BEGIN DATA ====";
    char end[] = "==== END DATA ====";
    int length_end = sizeof(end);
    int end_offset = 0;
    char buf;
    while (true) {

        read(sock_fd, &buf, 1);
        CC++; // count how many words we get
        
        // check if we reach the end
        // otherwise, keep reading
        if (buf == end[end_offset]) end_offset++;
        else end_offset = 0; // reset the end_offset
        if (end_offset == length_end-1) break;
    }
    CC = CC -  length_end - sizeof(start);
    // read all the remaining data in the buffer if there is any
    read(sock_fd, tmp, 1000);
    
    // prepare the reply
    char number[50] = "";
    memset(number, 0, 50);
    sprintf(number, "%d", CC);

    write(sock_fd, number, strlen(number));

    // read the response from the server and print it
    char rv[100] = "";
    read(sock_fd, rv, 100);
    cout << rv << endl;
    close(connection);
    return 0;
}