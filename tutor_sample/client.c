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
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <dirent.h>

/* ノンブロッキング:'n' */
// char g_mode='b';
char g_mode='n';

#define PORT 22222
#define SERVER_ADDR "127.0.0.1"

#define BUFSIZE 1024
#define FILESIZE 102400
#define FILENUM 999
#define FILENUMDIGIT 3  // dataファイルの数字部分、最大3桁（0-999）

// char* data_dir = "../sampleData/";
char* data_dir = "../data/";
char* file_name_prefix = "data"; // data0, data1, data2, ...
char g_buf[BUFSIZE]; // ファイルの読み込みバッファ

// flow: socket-connect

/* ブロッキングモードのセット */
int set_block(int fd, int flag) {
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

// check file
int checkIfFileExists(const char* filename){
    struct stat buf;
    int exist = stat(filename,&buf);
    if(exist == 0)
        return 1;
    else
        return 0;
}

// 分割して送信
// ssize_t send_all(int soc, char *buf, size_t size, int flag)
// {
//     ssize_t len, lest;
//     char *ptr;
//     for(ptr = buf, lest = size; lest > 0; lest -= len, ptr += len) {
//         if ((len = send(soc, ptr, lest, flag)) == -1) {
//             /* エラー:EAGAINでもリトライしない */
//             perror("send");
//             return (-1);
//         } else {
//           fprintf(stderr, "send:%d\n", (int) len);
//         }
//     }
//     return (size);
// }

// ファイルを1つ送信
void send_one(char *file_path, int soc) {
  fpos_t ft;
  ssize_t total = 0, len;
  memset(g_buf, 0, sizeof(g_buf)); // init
  // read binary file to g_buf
  FILE *fp = fopen(file_path, "rb");
  if(fp == NULL) {
      perror("fopen");
      exit(1);
  }
  // sizeof(g_buf)ごとに分割送信
  while(1) {
    len = fread(g_buf, sizeof(g_buf), 1, fp);
    fprintf(stderr, "fread:%d\n", (int) len); // debug
    if(len <= 0) {
      // EOF or error
      break;
    }
    len = send(soc, g_buf, sizeof(g_buf), 0);
    if(len == -1) {
        /* エラー:EAGAINでもリトライしない */
        perror("send");
    }
    total += len;
    fseek(fp,total, SEEK_SET); // 読み書き位置を先頭からtotalバイトずらす

    /// debug
    //fgetpos(fp,&ft);
    //printf("current fp location: %ld\n",ft);
    ///

    // free buf
    bzero(g_buf, BUFSIZE);
    usleep(1000);
  }
  fprintf(stderr, "send:%d\n", (int) len);
  fclose(fp);
}

void read_dir_file(char *dir_name, int sockfd) {
  int file_count = 0;
  for(file_count = 0; file_count <= FILENUM; file_count++) {
    char file_path[strlen(dir_name) + strlen(file_name_prefix) + FILENUMDIGIT + 1];
    // 例：../data/data0
    snprintf(file_path, sizeof(file_path), "%s%s%d", dir_name, file_name_prefix, file_count);
    printf("file_path: %s\n", file_path);

    // 該当するファイルが存在するかチェック
    if(checkIfFileExists(file_path)) {
      send_one(file_path, sockfd);
      usleep(1000); // ファイル間の送る間隔
    } else {
      // ない場合は、送り終えたとみなす
      printf("%s not found\n", file_path);
      break;
    }
  }
}

int main() {
  struct sockaddr_in server;
  int sockfd;

  // ソケットの作成
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) {
    perror("socket");
    exit(1);
  }

  // サーバのアドレスとポートを設定
  memset(&server, 0, sizeof(server)); // init
  server.sin_family = AF_INET; // IPv4
  server.sin_port = htons(PORT);
  server.sin_addr.s_addr = inet_addr(SERVER_ADDR);

  /* サーバに接続 */
  int client_handle = connect(sockfd, (struct sockaddr *)&server, sizeof(server));
  if(client_handle < 0) {
    perror("connect");
    exit(1);
  }
  // non-blocking mode
  if(g_mode == 'n') {
    set_block(sockfd, 0);
  }

  puts("server connected");
  // ディレクトリを読み込み、ファイルを1つずつサーバに送信
  read_dir_file(data_dir, sockfd);
  puts("finish read_dir_file");

  // socketの終了
  close(sockfd);

  return 0;
}