#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "web.h"


#define PORT 8080
#define BUF 8192


extern char *result;
extern void execute_sql(const char *sql);

static void handle_request(int client)
{
    char buf[BUF];
    int n = read(client, buf, sizeof(buf) - 1);
    if(n <= 0) return;
    buf[n] = 0;

    char *body = strstr(buf, "\r\n\r\n");
    if(!body) return;
    body += 4;

    /* Execute SQL */
    execute_sql(body);

    int body_len = strlen(result);

    char response [1024]; // = calloc(1024, sizeof(char));
    
    int resp_len = snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        body_len,
        result
    );
    
    write(client, response, resp_len);
}

void start_http_server(void)
{
    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 10);

    printf("HTTP server listening on http://localhost:%d\n", PORT);

    while(1)
    {
        int client = accept(server, NULL, NULL);
        handle_request(client);
        printf("-----------------------------------------");
        printf("-----------------------------------------\n");
        close(client);
    }
}
