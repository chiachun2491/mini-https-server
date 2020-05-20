#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <dirent.h>

#include "mySSLTool.h"
#include "myFileTool.h"

#define DEFAULT_PORT 4433

char httpHeader_200[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n";

char textHeader_200[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/plain; charset=UTF-8\r\n\r\n";

char httpHeader_404[] = 
"HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n";

/* server socket setting */
int create_socket(int port)
{
    int s;
    struct sockaddr_in addr;

    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }
    if (listen(s, 1) < 0) {
        perror("Unable to listen");
        exit(EXIT_FAILURE);
    }
    return s;
}

/* create server context */
SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    /* Create new server-method instance */
    method = TLSv1_2_server_method();
    /* Create new context */
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

int main(int argc, char **argv)
{
    int sock;
    SSL_CTX *ctx;
    int status;
    pid_t cpid;

    /* Load cryptos, bring in and register error messages */
    SSL_library_init();
    ctx = create_context();
    /* load certificate, key and load ca also set verify */
    configure_context(ctx, "./key/host.crt", "./key/host.key");

    /* create server socket */
    sock = create_socket(DEFAULT_PORT);

    /* Handle connections */
    while(1) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        SSL *ssl;

        /* accept client connection */
        int client = accept(sock, (struct sockaddr*)&addr, &len);
        if (client < 0) {
            perror("Unable to accept");
            exit(EXIT_FAILURE);
        }

        /* set ssl */
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);
        SSL_set_verify_depth(ssl, 1);

        /* create a child process to process */
        if ((cpid = fork()) < 0) {
            perror("Unable to fork");
            exit(EXIT_FAILURE);
        }
        /* child process */
        if (cpid == 0) {
            if (SSL_accept(ssl) < 0) {
                ERR_print_errors_fp(stderr);
            }
            else {
                char buf[DEFAULT_SIZE];
                int bytes;
                char* reply;

                char method[4];
                char path[DEFAULT_SIZE + 20];
                char filename[DEFAULT_SIZE];

                /* print connection information */
                printf("Connected with %s encryption\n", SSL_get_cipher(ssl));

                /* client doesn't have certificate */
                // show_certificate(ssl);

                /* get request & decrypt & print */
                bytes = SSL_read(ssl, buf, sizeof(buf)); 
                buf[bytes] = 0;
                printf("Received Info:\n%s\n\n", buf);

                /* parse browser request */
                if (sscanf(buf, "%s %s", method, path)) { 
                    // reply index.html
                    if (!strcmp(path, "/") || !strcmp(path, "/index.html")) {
                        SSL_write(ssl, httpHeader_200, strlen(httpHeader_200));
                        reply = readFile("./src/index.html", &bytes);
                    }
                    // reply file list
                    else if (!strcmp(path, "/api/file_list/")) {
                        SSL_write(ssl, textHeader_200, strlen(textHeader_200));
                        reply = getFileList();
                    }
                    // reply file content
                    else if (!strncmp(path, "/api/download/", strlen("/api/download/"))) {
                        SSL_write(ssl, textHeader_200, strlen(textHeader_200));
                        int prefixLength = strlen("/api/download/");
                        strcat(filename, "./storage/");
                        strncpy(filename + strlen("./storage/"), path + prefixLength , strlen(path) - prefixLength);
                        filename[strlen(path) - prefixLength + strlen("./storage/")] = '\0';
                        reply = readFile(filename, &bytes);
                    }
                    /* reply error message */
                    else {
                        SSL_write(ssl, httpHeader_404, strlen(httpHeader_404));
                        reply = readFile("./src/404.html", &bytes);
                    }
                }
                /* reply to client */
                SSL_write(ssl, reply, strlen(reply));
            }
            exit(0);
        }
        /* parent process */
        else {
            /* close client connection */
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client);
            waitpid(cpid, &status, 0);
        }
    }

    /* close server */
    close(sock);
    SSL_CTX_free(ctx);

    return 0;
}