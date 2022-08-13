#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/types.h>
#include <dirent.h>

/* ノンブロッキング:'n' */
char g_mode='b';

#define PORT 22222
#define QUEUELIMIT 10

#define FILESIZE 102400
#define BUFSIZE 1024
#define FILENUMDIGIT 3   // dataファイルの数字部分、最大3桁（0-999）

char recv_buf[BUFSIZE]; // 受信バッファ
char* data_dir = "../dataReceive/";
char* file_name_prefix = "data";
int receive_file_count = 0;

// flow: socket-bind-listen-accept

/* ブロッキングモードのセット */
int set_block(int fd, int flag){
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
        perror("fcntl");
        return (-1);
    }
    if (flag == 0) {
        /* ノンブロッキング */
        (void) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    } else if (flag == 1) {
        /* ブロッキング */
        (void) fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
    }
    return (0);
}


// receive binary data from client and write to file
/* 受信ループ */
int recv_loop(int acc)
{
    ssize_t total = 0, len;
    if (g_mode == 'n') {
        /* ノンブロッキングモード */
        (void) set_block(acc, 0);
    }
    char file_path[strlen(data_dir) + strlen(file_name_prefix) + FILENUMDIGIT + 1];
    // printf("strlen file_path: %lu\n", strlen(file_path));　// debug
    // concat ../dataReceive/ + dataXX
    snprintf(file_path, sizeof(file_path), "%s%s%d", data_dir, file_name_prefix, receive_file_count);
    printf("file_path: %s\n", file_path); // debug

    FILE *fp = fopen(file_path, "ab+"); // append mode
    if(fp == NULL) {
      perror("fopen");
      exit(1);
    }

    do {
        /* 受信 */
        len = recv(acc, recv_buf, sizeof(recv_buf), 0);
        if(len == -1) {
            /* エラー */
            if (errno == EAGAIN) {
                fprintf(stderr, ".");
                continue;
            } else {
                perror("recv");
                break;
            }
        }
        if (len == 0) { // EOF
            fprintf(stderr, "recv:EOF\n");
            break;
        }
        fprintf(stderr, "recv:%d\n", (int) len);

        // write binary data to file
        fwrite(recv_buf, sizeof(char), sizeof(recv_buf), fp);
        fseek(fp, 0, SEEK_END); // seek to end of file
        total += len;
        usleep(1000); // ファイルを受信する間隔
    } while(total < FILESIZE && total > 0);
    fprintf(stderr, "total:%d\n", (int) total);
    fclose(fp);

    //　異常状態を検知：読み込みができなかったときや、オーバーフローを起こした時
    if(total <= 0) {
      return -1;
    }
    // 正常状態：次のファイルの名前をつけるために、インクリメント
    receive_file_count++;
    return 0;
}

int main(void) {
  int sockfd;
  struct sockaddr_in addr;
  struct sockaddr_in client;
  unsigned int client_len;
  int new_sockfd;

  /* ソケットの作成 */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) {
    perror("socket");
    exit(1);
  }

  /* ソケットの設定 */
  // IPv4
  memset(&addr, 0, sizeof(addr)); // init
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  // 任意のアドレスを受け付ける
  addr.sin_addr.s_addr = INADDR_ANY;

  int bind_handle = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
  if(bind_handle < 0) {
    perror("bind");
    exit(1);
  }

  /* TCPクライアントからの接続要求を待てる状態にする */
  int listen_handle = listen(sockfd, QUEUELIMIT);
  if(listen_handle < 0) {
    perror("listen");
    exit(1);
  }

  /* TCPクライアントからの接続要求を受け付ける */
  client_len = sizeof(client);
  new_sockfd = accept(sockfd, (struct sockaddr *)&client, &client_len);
  if(new_sockfd < 0) {
    perror("accept");
    exit(1);
  }
  while(1) {
    int result = recv_loop(new_sockfd);
    if(result == -1) {
      break;
    }
  }
  printf("Data written in the file successfully.\n");

  /* TCPセッションの終了 */
  close(new_sockfd);

  /* listenするsocketの終了 */
  close(sockfd);

  return 0;
}