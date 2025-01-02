#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>
#include <termios.h>

#define BUF_SIZE 1024
#define WORD_SIZE 64
#define NAME_SIZE 20
#define ORANGE "\033[38;2;255;165;0m"
#define RESET "\033[0m"
void error_handling(char *message);
void *send_msg(void *arg);

typedef struct
{
    char word[WORD_SIZE];
    int frequency;
} search_engine;

char name[10] = "[SERVER]: ";

int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE];
    int str_len, bytes;
    struct sockaddr_in serv_adr;
    int index = 0;
    pthread_t snd_thread, rcv_thread;
    void *thread_return;

    FILE *fp;
    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error");
    else
        puts("Connected...........");

    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
    pthread_join(snd_thread, &thread_return);

    close(sock);
    return 0;
}

void *send_msg(void *arg)
{
    int sock = *((int *)arg), index = 0;
    char msg[BUF_SIZE], key, word[WORD_SIZE];
    int str_len;
    struct termios original, newtio;
    search_engine search[10];

    tcgetattr(STDIN_FILENO, &original);
    newtio = original;

    newtio.c_lflag &= ~ICANON;
    newtio.c_lflag &= ~ECHO;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newtio);

    printf("\033[2J");
    printf("\033[H");
    puts("Search word: ");
    while (1)
    {
        str_len = read(STDIN_FILENO, &key, 1);

        if (key == 127) // back space
        {
            if (index > 0)
            {
                index--;
                word[index] = '\0';
            }
        }
        else if (key == '\n')
        {
            index = 0;
            memset(&word, 0, sizeof(word));
            printf("\033[2J");
        }
        else
        {

            word[index] = key;
            index++;
            word[index] = '\0';
        }

        printf("\033[2J");
        printf("\033[H");
        // system("cls");

        if (index == 0)
        {
            printf("Search word: ");
            continue;
        }

        printf("Search word: %s\n", word);

        write(sock, word, strlen(word));

        printf("---------------------\n");

        str_len = read(sock, msg, sizeof(msg));

        write(sock, msg, strlen(msg));

        msg[str_len] = '\0';
        int result_count = atoi(msg);

        memset(&msg, 0, sizeof(msg));

        if (result_count > 10)
            result_count = 10;

        for (int i = 0; i < result_count; i++)
        {
            read(sock, msg, sizeof(msg));

            char *start;
            char *text = msg;

            while ((start = strstr(text, word)) != NULL) // 발견한 위치 = start
            {
                fwrite(text, sizeof(char), start - text, stdout);
                printf("%s", ORANGE);
                fwrite(word, sizeof(char), strlen(word), stdout);
                printf("%s", RESET);
                text = start + strlen(word);
            }
            printf("%s\n", text);
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
    return NULL;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}