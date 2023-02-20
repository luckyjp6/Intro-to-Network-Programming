/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h>

#define Max 81

int Box_index(int i, int j) {return (i/3)*3+j/3;}

int board[Max];
int Row[10][10] = {0}, Col[10][10] = {0}, Box[10][10] = {0};
int fixed[Max];
int Ans = 0;

int check_space(int index, int num) {
    int row = index/9, col = index%9;
    for (int i = 0; i < 9; i++) {
        if (board[i*9+col] == num) return 0;
    }
}

int check_end() {
    for (int i = 0; i < 9; i++) {
        for (int j = 1; j <= 9; j++) {
            if (!Row[i][j]) return 0;
            if (!Col[i][j]) return 0;
            if (!Box[i][j]) return 0;
        }        
    }

    return 1;
}

// recursivly solve the Sudoku
int SolveSudoku(int now) {
    if (now >= Max) return check_end();
    
    int space = now;
    while (board[space] != 0 && space < Max) space++;
    if (space >= Max) return check_end();

    int r = space / 9, c = space % 9;
    int b = Box_index(r, c);
    for (int num = 1; num <= 9; num++) {
        if (Row[r][num]) continue;
        if (Col[c][num]) continue;
        if (Box[b][num]) continue;

        board[space] = num;
        Row[r][num] = 1;
        Col[c][num] = 1;
        Box[b][num] = 1;

        if (SolveSudoku(now+1) == 0) {
            board[space] = 0;
            Row[r][num] = 0;
            Col[c][num] = 0;
            Box[b][num] = 0;
            continue;
        }
        else return 1;
    }

    return 0;
}

int main(int argc, char **argv) {
	int					unix_d;
	struct sockaddr_un	unix_addr;

	unix_d = socket(AF_LOCAL, SOCK_STREAM, 0);
	bzero(&unix_addr, sizeof(unix_addr));
	unix_addr.sun_family = AF_LOCAL;
	strcpy(unix_addr.sun_path, "/sudoku.sock");
	connect(unix_d, (struct sockaddr*) &unix_addr, sizeof(unix_addr));
    
    char b[10000];

    // aquire Sudoku question
    memset(b, '\0', 10000);
    write(unix_d, "S", 1);
    read(unix_d, b, 100);

    char *b_ptr = b+4;

    // store the board
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            int index = i*9+j;
            if (b_ptr[index] == '.') {
                board[index] = 0;
            }
            else {
                int num = (int)(b_ptr[index] - '0');
                board[index] = num;
                fixed[index] = 1;
                Row[i][num] = 1;
                Col[j][num] = 1;
                Box[Box_index(i, j)][num] = 1;
            }
        }
    }
    
    // solve Sudoku
    SolveSudoku(0);
    
    // send the answer
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            int index = i*9+j;
            if (fixed[index]) continue;
            char msg[100];
            sprintf(msg, "V %d %d %d", i, j, board[index]);
            write(unix_d, msg, strlen(msg));
            read(unix_d, b, 100);
        }
    }

    // ask the server to check the answer
    write(unix_d, "C", 1);
    
	exit(0);
}