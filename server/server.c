#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
#define MAX_CLNT 256
#define FILE_SIZE 64
#define WORD_SIZE 32

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
char database_file_name[FILE_SIZE];
pthread_mutex_t mutx;
FILE *fp;

typedef struct
{
    char word[WORD_SIZE];
    int count;
} data_struct;

typedef struct
{
    char word[WORD_SIZE];
    int count;
} result_struct;

void error_handling(char *message);
void *handle_clnt(void *arg);
void change_structure(result_struct *result, int *result_count, char word[WORD_SIZE]);
data_struct data[FILE_SIZE];
int data_count = 0;

int main(int argc, char *argv[])
{
    int serv_sock;
    int clnt_sock;
    pthread_t t_id;
    char buf[BUF_SIZE], message[BUF_SIZE], line[BUF_SIZE];
    int str_len;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;

    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    if (serv_sock == -1)
        error_handling("UDP socket creation error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (argc != 3)
    {
        printf("Usage : %s <Port> <File name>\n", argv[0]);
        exit(1);
    }

    strcpy(database_file_name, argv[2]);

    fp = fopen(database_file_name, "rb");

    if (!fp)
    {
        error_handling("No file exist");
    }
    else
    {
        printf("Data base: %s\n", database_file_name);
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        char *ptr = strtok(line, "/");

        strcpy(data[data_count].word, ptr);

        ptr = strtok(NULL, " ");

        data[data_count].count = atoi(ptr);

        data_count++;
    }

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }

    close(serv_sock);
    fclose(fp);
    return 0;
}

void *handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);
    int str_len = 0, i = 0;
    char msg[WORD_SIZE];
    result_struct result[10];

    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
    {
        printf("\033[2J");
        printf("\033[H");

        char word[WORD_SIZE];
        strcpy(word, msg);

        puts(word);
        memset(&msg, 0, sizeof(msg));

        int result_count = 0;

        change_structure(result, &result_count, word); // 구조 재배치

        char number[WORD_SIZE];
        snprintf(number, sizeof(number), "%d", result_count);

        write(clnt_sock, number, sizeof(number)); // count send

        read(clnt_sock, msg, sizeof(msg));
        memset(&msg, 0, sizeof(msg));

        for (int j = 0; j < result_count; j++)
        {
            char message[BUF_SIZE];
            int len = snprintf(message, sizeof(message), "%s %d", result[j].word, result[j].count);

            message[len] = '\0';
            printf("%s\n", message);
            write(clnt_sock, message, sizeof(message));
        }
        memset(result, 0, sizeof(result));
    }
    puts("입력 종료");

    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++) // remove disconnected client
    {
        if (clnt_sock == clnt_socks[i])
        {
            while (i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }

    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}

void change_structure(result_struct *result, int *result_count, char word[WORD_SIZE])
{
    *result_count = 0;

    for (int j = 0; j < data_count; j++)
    {
        if (strncmp(data[j].word, word, strlen(word)) == 0)
        {
            strcpy(result[*result_count].word, data[j].word);
            result[*result_count].count = data[j].count;

            (*result_count)++;
        }
    } 

    for(int i=0; i<*result_count; i++) // 내림차순 result 재배치
    {
        for(int j=0; j<*result_count; j++)
        {
            if(result[i].count > result[j].count)
            {
                int temp = result[j].count;
                result[j].count = result[i].count;
                result[i].count = temp;

                char tmp[WORD_SIZE];
                strcpy(tmp, result[j].word);
                strcpy(result[j].word,result[i].word);
                strcpy(result[i].word,tmp);
            }
        }
    }
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}