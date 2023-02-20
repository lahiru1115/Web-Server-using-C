#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/file.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#define DFLT_MIME_TYPE "application/octet-stream"
#define ROOT "./src"

struct fl_data
{
    int size;
    void *data;
};

struct fl_data *fl_load(char *fl_name)
{
    char *buffer, *p;
    struct stat buf;
    int bytes_read, bytes_rem, total_bytes = 0;

    if (stat(fl_name, &buf) == -1)
    {
        return NULL;
    }

    if (!(buf.st_mode & S_IFREG))
    {
        return NULL;
    }

    FILE *fp = fopen(fl_name, "r");

    if (fp == NULL)
    {
        return NULL;
    }

    bytes_rem = buf.st_size;
    p = buffer = malloc(bytes_rem);

    if (buffer == NULL)
    {
        return NULL;
    }

    while (bytes_read = fread(p, 1, bytes_rem, fp), bytes_read != 0 && bytes_rem > 0)
    {
        if (bytes_read == -1)
        {
            free(buffer);
            return NULL;
        }

        bytes_rem -= bytes_read;
        p += bytes_read;
        total_bytes += bytes_read;
    }

    struct fl_data *fldata = malloc(sizeof *fldata);

    if (fldata == NULL)
    {
        free(buffer);
        return NULL;
    }

    fldata->data = buffer;
    fldata->size = total_bytes;

    return fldata;
}

void fl_free(struct fl_data *fldata)
{
    free(fldata->data);
    free(fldata);
}

void send_res(int fd, char *header, char *content_type, void *body, int content_len)
{
    const int max_res_size = 65536;
    char response[max_res_size];
    time_t rawtime;
    struct tm *info;

    time(&rawtime);

    int r = sprintf(response,
                    "%s\r\n"
                    "Connection: close\r\n"
                    "Content-Length: %d\r\n"
                    "Content-Type: %s\r\n"
                    "Date: %s\r\n",
                    header, content_len, content_type, asctime(localtime(&rawtime)));

    memcpy(response + r, body, content_len);
    send(fd, response, content_len + r, 0);
}

// Edit

void post_request(int fd, char *body)
{

    char response_body[1024];
    char *status;

    int fp = open("post.text", O_CREAT | O_WRONLY, 0644);

    if (fp)
    {
        flock(fp, LOCK_EX);
        write(fp, body, strlen(body));

        flock(fp, LOCK_UN);

        close(fp);

        status = "ok";
    }
    else
    {
        status = "error";
    }

    send_res(fd, "HTTP/1.1 201 Created", "application/json", response_body, sizeof(response_body));
}

char *strlower(char *s)
{
    for (char *p = s; *p != '\0'; p++)
    {
        *p = tolower(*p);
    }

    return s;
}

char *file_type_get(char *fl_name)
{
    char *ext = strrchr(fl_name, '.');

    if (ext == NULL)
    {
        return DFLT_MIME_TYPE;
    }

    ext++;

    strlower(ext);

    if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0)
    {
        return "text/html";
    }
    if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0)
    {
        return "image/jpg";
    }
    if (strcmp(ext, "txt") == 0)
    {
        return "text/plain";
    }
    if (strcmp(ext, "gif") == 0)
    {
        return "image/gif";
    }
    if (strcmp(ext, "png") == 0)
    {
        return "image/png";
    }
    if (strcmp(ext, "css") == 0)
    {
        return "text/css";
    }
    if (strcmp(ext, "js") == 0)
    {
        return "application/javascript";
    }
    if (strcmp(ext, "json") == 0)
    {
        return "application/json";
    }

    return DFLT_MIME_TYPE;
}

char *find_end_of_header(char *header)
{

    char *nl;

    if (strstr(header, "\n\n"))
    {
        nl = strstr(header, "\n\n");
    }
    else if (strstr(header, "\r\r"))
    {
        nl = strstr(header, "\r\r");
    }
    else
    {
        nl = strstr(header, "\r\n\r\n");
    }
    return nl;
}

void load_file(int fd, char *request_path)
{
    char filepath[4096];
    struct fl_data *fldata;
    char *mime_type;

    snprintf(filepath, sizeof(filepath), "%s%s", ROOT, request_path);
    fldata = fl_load(filepath);
    mime_type = file_type_get(filepath);
    if (fldata == NULL)
    {
        fldata = fl_load("./root/404.html");
        send_res(fd, "HTTP/1.1 200 OK", mime_type, fldata->data, fldata->size);
    }
    else
    {
        send_res(fd, "HTTP/1.1 200 OK", mime_type, fldata->data, fldata->size);
    }

    fl_free(fldata);
}

void handle_http_request(int d)
{
    const int request_buffer_size = 65536;
    char request[request_buffer_size];
    char *p;

    int bytes_recvd = recv(d, request, request_buffer_size - 1, 0);

    if (bytes_recvd < 0)
    {
        perror("recv");
        return;
    }
    request[bytes_recvd] = '\0';

    char method[10], path[100], protocol[20];
    sscanf(request, "%s %s %s", method, path, protocol);
    p = find_end_of_header(request);

    if (strcmp(method, "GET") == 0)
    {
        load_file(d, path);
    }
    else
    {
        post_request(d, p);
    }
}

int main()
{
    int client_socket, server_socket;
    socklen_t addrlen;
    int bufsize = 1024;
    char *buffer = malloc(bufsize);
    struct sockaddr_in address;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0)
    {
        printf("The socket was created\n");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(client_socket, (struct sockaddr *)&address, sizeof(address)) == 0)
    {
        printf("Binding Socket\n");
    }
    if (listen(client_socket, 50) < 0)
    {
        perror("server: listen");
    }

    while (1)
    {
        if ((server_socket = accept(client_socket, (struct sockaddr *)&address, &addrlen)) < 0)
        {
            perror("server: accept");
        }

        if (server_socket > 0)
        {
            printf("The Client is connected...\n");
        }

        handle_http_request(server_socket);
        close(server_socket);
    }
    return 0;
}
