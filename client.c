#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 512 //* Definition for Buffer size;

void error_handling(char *message);
void bingo(int sock);
int handle_server_msg(char *state, char *value);

int main(int argc, char *argv[])
{
  int sock;

  struct sockaddr_in serv_adr;

  if (argc != 3) //* Invalid num of arguments
  {
    printf("Usage: %s <IP> <port> \n", argv[0]);
    exit(1);
  }

  sock = socket(PF_INET, SOCK_STREAM, 0); //* Socket init.

  if (sock == -1)
  {
    error_handling("ERROR: Socket Initialization Failed");
  }

  memset(&serv_adr, 0, sizeof(serv_adr)); //* Memory Allocation for address.

  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = inet_addr(argv[1]); //* Server Address (IP) Init.
  serv_adr.sin_port = htons(atoi(argv[2]));      //* Server Port Init.

  if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
  {
    //* If connection failed,
    error_handling("ERROR: Connection Failure");
  }
  else
  {
    printf("Loading...\n");
    bingo(sock); //* Start Bingo Game - Client
  }

  close(sock);
  return 0;
}

//* Function bingo: Start and manage whole process of bingo game.
void bingo(int sock)
{
  int str_len, flag; //* flag: variable to determine to start, continue, or terminate the bingo game.
  char message[BUF_SIZE];
  char *delimiter = " ";

  while (1)
  {
    //* Read Message from the server
    str_len = read(sock, message, strlen(message));
    /*
    > The message will be like following
    * "STATE VALUE" (Value is optional, and both are distinguished by whitespace)
    ? e.g.) The Opponent chose number 24 => SELECTED 24
    ? e.g.) Opponent Found, game starts! (Current Client goes first) => START_FIRST (START FIRST is a message with a single STATE. It doesn't have any value.)
    */

    char *state = strtok(state, delimiter);
    char *value = strtok(NULL, delimiter);

    flag = handle_server_msg(state, value); //* Handle Server-side message
    //: Based on the flag value, the client will determine to start, continue or terminate the bingo game.
    /*
    > Messages
    * 0: PENDING - Waiting for another client join to this game.
    * 1: START_FIRST - Start game, current client first.
    * 2: START_SECOND - Start game, opponent first.
    * 3: SELECTED <number> - Opponent selected number, end its turn.
    * 4: FINISHED WIN | LOSE - Game Finished, show result, and show if current user win or lose based on the value.
    * -1: REJECTED | <Other Error> - Connection Rejected or Error Happened, close the connection.
    */
  }

  return;
}

int handle_server_msg(char *state, char *value)
{
  int ret_val = -1;

  if (strcmp(state, "PENDING") == 0)
  {
    printf("상대방을 기다리고 있습니다...");
    ret_val = 0;
  }
  else if (strcmp(state, "START_FIRST") == 0)
  {
    printf("상대방을 찾았습니다! 게임이 시작됩니다.\n 당신은 선공입니다.\n");
    ret_val = 1;
  }
  else if (strcmp(state, "START_SECOND") == 0)
  {
    printf("상대방을 찾았습니다! 게임이 시작됩니다.\n 당신은 후공입니다.\n");
    ret_val = 2;
  }
  else if (strcmp(state, "SELECTED") == 0)
  {
    const int num = atoi(value);
    printf("상대방이 숫자 %d를 선택했습니다. 반영된 결과는 다음과 같습니다.\n", num);
    ret_val = 3;
  }
  else if (strcmp(state, "FINISHED") == 0)
  {
    ret_val = 4;
  }
  else if (strcmp(state, "REJECTED") == 0)
  {
    printf("게임이 이미 진행되고 있습니다. 연결을 종료합니다.\n");
    ret_val = -1;
  }
  else
  {
    printf("ERROR: Something gone wrong...\n");
    ret_val = -1;
  }

  return ret_val;
}

void error_handling(char *message) //* Error Handling
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}