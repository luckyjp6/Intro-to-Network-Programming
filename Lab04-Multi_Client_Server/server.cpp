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
#include <sys/wait.h>
#include <fcntl.h>

using namespace std;

void sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
        printf("child %d terminated\n", pid);
    }

	return;
}

void err_sys(const char* x) 
{ 
    perror(x); 
    exit(1); 
}

int main(int argc, char **argv)
{
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
	void				sig_chld(int);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	int port_num = 0;
	for (int i = 0; i < sizeof(argv[1]); i++) {
        if (argv[1][i] == '\0') break;
        port_num *= 10;
        port_num += argv[1][i] - '0';
    }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(0);
	servaddr.sin_port        = htons(port_num);

	if (bind(listenfd, (const sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		cout << "failed to bind\n";
		return -1;
	}

	listen(listenfd, 20);

	signal(SIGCHLD, sig_chld);

	int stdout_cpy = dup(STDOUT_FILENO);

	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (sockaddr *) &cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				err_sys("accept error");
		}
        printf("received connection!\n");

		if ( (childpid = fork()) == 0) {	/* child process */
			dup2(connfd, STDOUT_FILENO);
			dup2(connfd, STDERR_FILENO);
			dup2(connfd, STDIN_FILENO);
			close(listenfd);	/* close listening socket */
            if (execvp(argv[2], argv+2) <0) 	/* process the request */
			{
				write(connfd, "Bad command!\n", sizeof("Bad command!\n"));
			}
            exit(0);
		}

		close(connfd);			/* parent closes connected socket */
	}
}