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

    const char *response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "OK\n";
    
    write(client, response, sizeof(response));
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
        close(client);
    }
}
