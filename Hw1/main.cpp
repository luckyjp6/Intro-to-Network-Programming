#include "functions.h"
#include "print_msg.h"
#include "error_func.h"

int main(int argc, char **argv)
{
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	sockaddr_in	cliaddr, servaddr;
	void				sig_chld(int);
    int	i, nready;

    init();

    clilen = sizeof(cliaddr);
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));
    
    if (argc < 2) 
    {
        printf("Usage: ./a.out [port]\n");
        return -1;
    }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(0);//INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	if (bind(listenfd, (const sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
    {
		printf("failed to bind\n");
		return -1;
	}

	listen(listenfd, 1024);
    
    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;

	for ( ; ; ) 
    {
        nready = poll(client, maxi+1, -1);
        
        // new client
        if (client[0].revents & POLLRDNORM) 
        {
            nready--;
            connfd = accept(listenfd, (sockaddr *) &cliaddr, &clilen);
            num_user ++;

            // save descriptor
            for (i = 1; i < OPEN_MAX; i++)
            {
                if (client[i].fd < 0) 
                {
                    client[i].fd = connfd;
                    client_info[i].addr = cliaddr;
                    break;
                }
            }

            if (i == OPEN_MAX) printf("too many clients\n");
            else
            {
                client[i].events = POLLRDNORM;
                if (i > maxi) maxi = i;
            }            
        }
        
        /* check all clients */
		int n;
        char buf[MSG_SIZE];
        
        for (i = 1; i <= maxi; i++) 
        {
            if (client[i].fd < 0) continue;
            if (client[i].revents & (POLLRDNORM | POLLERR)) 
            {
                /* read input*/
                memset(buf, '\0', MSG_SIZE);

                if ( (n = read(client[i].fd, buf, MSG_SIZE-50)) < 0) 
                { 
                    if (errno == ECONNRESET) close_client(i); /* connection reset by client */
                    // else err_sys("read error");
                } 
                else if (n == 0) close_client(i); /* connection closed by client */
                else /* read command */
                {                 
                    const char *new_line = " \n\r\0";
                    char *command = strtok(buf, new_line);

                    if (command == NULL) goto next;
                    
                    // if (buf[0] == ':') 
                    // {
                    //     command += 1;
                    //     if ( check_nick_name(command) )// name_client.find(command) == name_client.end()) {
                    //         not_registered(sockfd, command);
                    //         goto next;
                    //     }
                    //     command = strtok(NULL, new_line);
                    // }

                    /* set nickname */
                    if (strcmp(command, "NICK") == 0) 
                    {
                        char *new_nick;
                        new_nick = strtok(NULL, new_line);

                        // current client rename
                        if (strlen(client_info[i].user_name) != 0 && strlen(client_info[i].nick_name) != 0)
                        {   
                            if (check_nick_name(i, new_nick, false)) goto next;
                        }
                        // new client set nickname
                        else 
                        {   
                            if (check_nick_name(i, new_nick, true)) goto next;   
                        }

                        strcpy(client_info[i].nick_name, new_nick);
                        goto next;
                    }
                    /* register new client */
                    else if (strcmp(command, "USER") == 0) 
                    {
                        char *args[4]; // <username> <hostname> <servername> <realname>
                        for (int j = 0; j < 4; j++) 
                        {
                            args[j] = strtok(NULL, new_line);
                            if (args[j] == NULL) {
                                not_enough_args(i, command);
                                goto next;
                            }
                        }
                        
                        // register new user and show welcome message
                        if ( strlen(client_info[i].user_name) != 0) reregister_error(i);
                        else 
                        {
                            strcpy(client_info[i].user_name, args[0]);
                            strcpy(client_info[i].real_name, args[3]+1);
                        }

                        welcome_new_client(i);
                        goto next;
                    }
                    
                    if ( strlen(client_info[i].user_name) == 0 ) 
                    {
                        if ( strlen(client_info[i].nick_name) == 0 ) not_registered(i);
                        else not_registered(i, client_info[i].nick_name);
                        goto next;
                    }

                    if (strcmp(command, "PING") == 0) {
                        char *host = strtok(NULL, new_line);
                        if (host == NULL) no_origin(i);
                        print_ping(client[i].fd, host);
                    }
                    /* list all wanted channels and their information */
                    else if (strcmp(command, "LIST") == 0) 
                    {
                        char *channel = strtok(NULL, new_line);
                        // list all channels
                        if (channel == NULL)
                        {
                            print_channel_info(i);
                        }
                        // only list specified channel
                        else
                        {
                            if (channels.find(channel) != channels.end()) {
                                print_channel_info(i, channel);
                            }                            
                        }
                    }
                    /* join a channel */
                    else if (strcmp(command, "JOIN") == 0)
                    {
                        const char *new_line = " ,\n\r\0";
                        char *channel = strtok(NULL, new_line);
                        if (channel == NULL)
                        {
                            not_enough_args(i, command);
                            goto next;
                        }
                        
                        if (channel[0] != '#') {
                            no_such_channel(i, channel);
                            char tmp[100];
                            strcpy(tmp, channel);
                            strcpy(channel, "#");
                            strcat(channel, tmp);
                        }

                        print_join(i, channel);
                    }
                    else if (strcmp(command, "TOPIC") == 0)
                    {
                        char *channel = strtok(NULL, new_line);
                        if (channel == NULL)
                        {
                            not_enough_args(i, command);
                            goto next;
                        }

                        if (!check_user_in_channel(i, channel)) 
                        {
                            not_on_channel(i, channel);
                            goto next;
                        }
                        
                        char *topic = strtok(NULL, "\0");
                        if (topic == NULL) 
                            print_topic(i, "", channel);
                        else 
                        {
                            print_topic(i, topic, channel);
                        }
                    }
                    /* list all clients' nickname in some channels */
                    else if (strcmp(command, "NAMES") == 0)
                    {
                        char *channel = strtok(NULL, new_line);

                        // list every channels' client
                        if (channel == NULL)
                        {
                            print_channel_users(i);
                        }
                        // only list specified channels' clients
                        else
                        {
                            print_channel_users(i, channel);
                        } 
                    }
                    else if (strcmp(command, "PART") == 0)
                    {
                        char *channel = strtok(NULL, new_line);
                        if (channel == NULL) 
                        {
                            not_enough_args(i, command);
                            goto next;
                        }
                        
                        if (!check_user_in_channel(i, channel)) 
                        {
                            not_on_channel(i, channel);
                            goto next;
                        }
                        print_part(i, channel);
                    }
                    /* print all users' and their information */
                    else if (strcmp(command, "USERS") == 0)
                    {
                        print_all_users(i);
                    }
                    else if (strcmp(command, "PRIVMSG") == 0) 
                    {
                        char *channel = strtok(NULL, new_line);
                        if (channel == NULL) {
                            no_recipient(i, command);
                            goto next;
                        }

                        char *msg = strtok(NULL, new_line);
                        
                        if (channels.find(channel) == channels.end())
                        {
                            if (msg == NULL) no_such_channel(i, channel);
                            else no_such_nick_channel(i, channel);
                            goto next;
                        }
                        if (msg == NULL) 
                        {
                            no_text_send(i);
                            goto next;
                        }

                        if (!check_user_in_channel(i, channel)) 
                        {
                            not_on_channel(i, channel);
                            goto next;
                        }
                        msg += 1;
                        print_msg_channel(i, msg, channel);
                    }
                    else if (strcmp(command, "QUIT") == 0) close_client(i);
                    else error_cmd(i, command);
                }
next:                
                if (--nready <= 0) break; /* no more readable descs */
            }
        }	
	}
    /* parent closes connected socket */
    close(connfd);
}