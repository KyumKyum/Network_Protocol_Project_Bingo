#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 512 //* Definition for Buffer size;
#define REQ_SIZE 10
#define NUM_SIZE 5
#define NUM_LIMIT 25 //* Definition for number limit in bingo game.

void error_handling(char *);
void bingo(int);
void bingo_init(void);    //* Intialize bingo
void bingo_shuffle(void); //* Shuffle the bingo
void bingo_print(void);   //* Print bingo plates
void bingo_select(int);
int bingo_calculate(void);
void bingo_calculate_row(int *);
void bingo_calculate_col(int *);
void bingo_calculate_diag(int *);
void bingo_send_msg(char *, char *, int, int);
int handle_server_msg(char *, char *, int);

int bingo_value[25] = {0}; //* bingo_value: show bingo number, became -1 if the number checked.
// int bingo_num = 0;         //* bingo_num: show number of bingo current player get.

int main(int argc, char *argv[])
{
  int sock;

  struct sockaddr_in serv_adr;

  if (argc != 3) //* Invalid num of arguments
  {
    printf("Usage: %s <IP> <port> \n", argv[0]);
    exit(1);
  }

  sock = socket(AF_INET, SOCK_STREAM, 0); //* Socket init.

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
    printf("상대방을 기다리고 있습니다...\n");
    bingo(sock); //* Start Bingo Game - Client
  }

  close(sock);
  return 0;
}

//* Function bingo: Start and manage whole process of bingo game.
void bingo(int sock)
{

  int str_len, flag, bingo_num = 0; //* flag: variable to determine to start, continue, or terminate the bingo game.
  char message[BUF_SIZE];
  char request[REQ_SIZE];
  char number[NUM_SIZE];

  bingo_init();
  printf("\n\n\n");

  while (1)
  {
    //* Reset value.
    flag = 0;
    bingo_num = 0;

    //* Read Message from the server
    str_len = read(sock, message, BUF_SIZE);
    /*
    > The message will be like following
    * "STATE VALUE" (Value is optional, and both are distinguished by whitespace)
    ? e.g.) The Opponent chose number 24 => SELECTED 24
    ? e.g.) Opponent Found, game starts! (Current Client goes first) => START_FIRST (START FIRST is a message with a single STATE. It doesn't have any value.)
    */
    // printf("%s\n", message); //! Debug

    if (str_len > 0) //* Works if sth arrives
    {
      printf("\n%s\n", message);
      char *state = strtok(message, " ");
      char *value = strtok(NULL, " ");

      // printf("\n%s\n%s\n", state, value);
      flag = handle_server_msg(state, value, str_len); //* Handle Server-side message
      //: Based on the flag value, the client will determine to start, continue or terminate the bingo game.
      /*
      > Messages
      * 0: init value, used for pending
      * 1: START_FIRST - Start game, current client first.
      * 2: START_SECOND - Start game, opponent first.
      * 3: SELECTED <number> - Opponent selected number, end its turn.
      * 4: FINISHED WIN | LOSE - Game Finished, show result, and show if current user win or lose based on the value.
      * -1: REJECTED | <Other Error> - Connection Rejected or Error Happened, close the connection.
      */

      if (flag == 1)
      {
        //* START_FIRST - Start game, you first
        bingo_print(); //* Print Bingo.
        fputs("\n숫자를 입력해주세요: ", stdout);
        fgets(number, BUF_SIZE, stdin);

        bingo_select(atoi(number));
        bingo_num = bingo_calculate();

        // printf("%d %d\n", bingo_num, atoi(number));
        bingo_send_msg(request, number, bingo_num, sock);

        //* show message
        bingo_print(); //* Print Bingo.
        printf("\n 상대방의 응답을 기다리는 중입니다...\n\n");
      }
      else if (flag == 2)
      {
        //* START_SECOND - Start game, the opponent first
        bingo_print(); //* Print Bingo.
        printf("\n 상대방의 응답을 기다리는 중입니다...\n\n");
      }
      else if (flag == 3)
      {
        bingo_select(atoi(value)); //* Apply Opponent
        bingo_print();
        //* Check if bingo had finished
        bingo_num = bingo_calculate();

        if (bingo_num >= 3)
        {
          bingo_send_msg(request, number, bingo_num, sock); //* Immediately send message, tells bingo had been completed.
        }
        else
        {
          fputs("\n숫자를 입력해주세요: ", stdout);
          fgets(number, BUF_SIZE, stdin);

          bingo_select(atoi(number));
          bingo_num = bingo_calculate();

          // printf("%d %d\n", bingo_num, atoi(number));
          bingo_send_msg(request, number, bingo_num, sock);
          //* show message
          bingo_print(); //* Print Bingo.
          printf("\n 상대방의 응답을 기다리는 중입니다...\n\n");
        }
      }
      else if (flag == 4 || flag == -1)
      {
        //*Game finished or error occured
        break;
      }
    }
  }
  return;
}

void bingo_init(void)
{

  for (int idx = 0; idx < NUM_LIMIT; idx++)
  {
    bingo_value[idx] = idx + 1;
  }

  bingo_shuffle();
}

void bingo_shuffle(void)
{
  srand((unsigned int)time(NULL));
  int t1, t2, temp;
  for (int idx = 0; idx < NUM_LIMIT; idx++)
  {
    int t1 = rand() % NUM_LIMIT;
    int t2 = rand() % NUM_LIMIT;

    temp = bingo_value[t1];
    bingo_value[t1] = bingo_value[t2];
    bingo_value[t2] = temp;
  }
}

void bingo_print(void)
{
  for (int idx = 0; idx < NUM_LIMIT; idx++)
  {
    if (bingo_value[idx] == -1)
    { //* If this value had been checked, prints blank.
      printf("  X ");
    }
    else
    {
      printf("%3d ", bingo_value[idx]);
    }
    if ((idx + 1) % 5 == 0)
    {
      //* for every 5 values
      printf("\n"); //*line breaks
    }
  }
}

void bingo_select(int selected)
{
  for (int idx = 0; idx < 25; idx++)
  {
    if (bingo_value[idx] == -1 || bingo_value[idx] != selected)
    {
      //* If current value is already selected, or not the selected
      continue;
    }
    //* Founded.
    bingo_value[idx] = -1;
    break;
  }
}

int bingo_calculate(void)
{
  int num_of_bingo = 0;

  bingo_calculate_row(&num_of_bingo);
  bingo_calculate_col(&num_of_bingo);
  bingo_calculate_diag(&num_of_bingo);

  return num_of_bingo;
}

void bingo_calculate_row(int *num_of_bingo) //* Calculate number of bingo in row.
{
  //*calculate the row
  for (int row = 0; row < 5; row++)
  {
    for (int element = 0; element < 5; element++)
    {
      if (bingo_value[(row * 5) + element] != -1)
      { //*Current row is not a bingo
        break;
      }
      if (element == 4)
      { //* Reached end, current row is a bingo.
        *num_of_bingo += 1;
      }
    }
  }
}

void bingo_calculate_col(int *num_of_bingo) //* Calculate number of bingo in column.
{
  //*calculate the column
  for (int col = 0; col < 5; col++)
  {
    for (int element = 0; element < 5; element++)
    {
      if (bingo_value[col + (element * 5)] != -1)
      { //*Current column is not a bingo
        break;
      }
      if (element == 4)
      { //* Reached end, current column is a bingo.
        *num_of_bingo += 1;
      }
    }
  }
}

void bingo_calculate_diag(int *num_of_bingo) //* Calculate number of bingo in diagonal direction.
{
  //* calculate right diagonal
  for (int rdiag = 0; rdiag < 5; rdiag++)
  {
    if (bingo_value[rdiag * 6] != -1)
    {
      //* Current Diagonal Line is not a bingo
      break;
    }

    if (rdiag == 4)
    {
      *num_of_bingo += 1;
    }
  }

  //* calculate left diagonal
  for (int ldiag = 0; ldiag < 5; ldiag++)
  {
    if (bingo_value[(ldiag + 1) * 4] != -1)
    {
      //* Current Diagonal Line is not a bingo.
      break;
    }

    if (ldiag == 4)
    {
      *num_of_bingo += 1;
    }
  }
}

void bingo_send_msg(char *request, char *number, int bingo_num, int sock)
{
  char blank[2] = " ";
  char firstNum[5];

  sprintf(firstNum, "%d", bingo_num);
  request = strcat(strcat(firstNum, blank), number);

  //* Send message to server
  printf("\n%s\n", request);
  write(sock, request, REQ_SIZE);

  return;
}

int handle_server_msg(char *msg, char *value, int str_len)
{
  int ret_val = -1;
  char message[20] = {'\0'};
  strncpy(message, msg, str_len);

  if (strcmp(message, "START_FIRST") == 0)
  {
    printf("상대방을 찾았습니다! 게임이 시작됩니다.\n 당신은 선공입니다.\n");
    ret_val = 1;
  }
  else if (strcmp(message, "START_SECOND") == 0)
  {
    printf("상대방을 찾았습니다! 게임이 시작됩니다.\n 당신은 후공입니다.\n");
    ret_val = 2;
  }
  else if (strcmp(message, "SELECTED") == 0)
  {
    const int num = atoi(value);
    printf("상대방이 숫자 %d를 선택했습니다. 반영된 결과는 다음과 같습니다.\n", num);
    ret_val = 3;
  }
  else if (strcmp(message, "FINISHED") == 0)
  {
    ret_val = 4;

    printf("GAME SET! 게임이 종료됩니다!\n");
    const int flag = atoi(value);

    if (flag == 0)
    {
      printf("당신이 이겼습니다! 축하드립니다! :) \n");
    }
    else
    {
      printf("당신은 졌습니다... 다음에 좋은 기회가 있겠죠! \n");
    }
  }
  else if (strcmp(message, "REJECTED") == 0)
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