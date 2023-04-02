#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 100
#define CMD_SIZE 20	// for saving bingo command
void error_handling(char *message);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	char bingo_data[BUF_SIZE];	// Save bingo data from clients
	char *select_cmd;		// Command of "SELECTED <num>"

	char start_first[CMD_SIZE] = "START_FIRST";
	char start_second[CMD_SIZE] = "START_SECOND";
	char selected[CMD_SIZE] = "SELECTED ";
	char finished_win[CMD_SIZE] = "FINISHED 0";
	char finished_lose[CMD_SIZE] = "FINISHED 1";
	char rejected[CMD_SIZE] = "REJECTED";

	int str_len, i, j;

	int clnt_cnt = 0;	// The number of times clients attempted to connect server
	int clnt_socknum[5];	// File descriptor of client 1~5

	struct timeval timeout;
	fd_set reads, cpy_reads;
	int fd_max, fd_num;	// fd : file descriptor, fd_max : maximum of fd

	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t adr_sz;

	if(argc != 2)
	{
		printf("Usage : %s <port> \n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	if(serv_sock == -1)
	{
		error_handling("Error : Failed to create server socket");
	}

	memset(&serv_adr, 0, sizeof(serv_adr));

	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
	{
		error_handling("Error : Failed to address the server socket");
	}

	if(listen(serv_sock, 5) == -1)
	{
		error_handling("Error : Failed to receive connection request");
	}

	FD_ZERO(&reads);
	FD_SET(serv_sock, &reads);
	fd_max = serv_sock;

	while(1)
	{
		cpy_reads = reads;
		timeout.tv_sec = 5;
		timeout.tv_usec = 5000;

		if((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1)
		{
			break;
		}

		if(fd_num == 0)
		{
			continue;
		}

		for(i = 0; i < fd_max + 1; i++)
		{
			if(FD_ISSET(i, &cpy_reads))
			{
				if(i == serv_sock)	// Accept request and start game
				{
					adr_sz = sizeof(clnt_adr);

					clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);

					if(clnt_sock == -1)
					{
						error_handling("Error : Failed to accept connection request");
						exit(1);
					}
					else
					{
						clnt_socknum[clnt_cnt] = clnt_sock;
						clnt_cnt++;
					}

					FD_SET(clnt_sock, &reads);

					if(fd_max < clnt_sock)
					{
						fd_max = clnt_sock;
					}

					printf("Client %d entered room. \n", clnt_cnt);

					if(clnt_cnt == 2)	// 2 clients entered
					{
						write(clnt_socknum[0], start_first, strlen(start_first));	// Client 1
						write(clnt_socknum[1], start_second, strlen(start_second));	// Client 2
					}
					else if(clnt_cnt > 2)	// More clients attempt connecting
					{
						write(clnt_socknum[clnt_cnt - 1], rejected, strlen(rejected));
						close(clnt_socknum[clnt_cnt - 1]);
					}
				}
				else	// Make bingo game works
				{
					str_len = read(i, bingo_data, BUF_SIZE);	// Receive bingo data from clients

					if(str_len == 0)
					{
						FD_CLR(i, &reads);
						close(i);
					}
					else
					{
						if(bingo_data[0] < 3)	// Client completed bingo less than 3
						{
							if(i == clnt_socknum[0]) // Turn of client 1
							{
								printf("Bingo of Client 1 : %c \n", bingo_data[0]);

								select_cmd = strcat(selected, &bingo_data[2]);
								write(clnt_socknum[1], select_cmd, strlen(select_cmd));
								// Deliver selected num of client 1 to client 2
							}
							else if(i == clnt_socknum[1]) // Turn of client 2
							{
								printf("Bingo of Client 2 : %c \n", bingo_data[0]);

								select_cmd = strcat(selected, &bingo_data[2]);
								write(clnt_socknum[0], select_cmd, strlen(select_cmd));
								// Deliver selected num of client 2 to client 1
							}

							printf("%s", select_cmd);

							for(j = 0; j < str_len; j++) // Making bingo_data[] empty
							{
								bingo_data[j] = '\0';
							}
						}
						else	// One of clients completed 3 or more bingo
						{

							if(i == clnt_socknum[0]) // Winner : Client 1
							{
								write(clnt_socknum[0], finished_win, strlen(finished_win));
								write(clnt_socknum[1], finished_lose, strlen(finished_lose));

								close(clnt_socknum[0]);
								close(clnt_socknum[1]);
							}
							else if(i == clnt_socknum[1]) // Winner : client 2
							{
								write(clnt_socknum[0], finished_lose, strlen(finished_lose));
								write(clnt_socknum[1], finished_win, strlen(finished_win));

								close(clnt_socknum[0]);
								close(clnt_socknum[1]);
							}
						}
					}
				}
			}
		}
	}
	close(serv_sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
