#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <syslog.h>

#define DEFAULT_BUFFER_SIZE (200 * 1024)
#define DEFAULT_PORT 9100
#define DEFAULT_PATH "/var/log/printjob.log"
#define DEFAULT_LOG "printer-sim"

int create_socket(int port)
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_result = bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (bind_result == -1)
    {
        printf("Error: Failed to bind the socket\n");
        exit(EXIT_FAILURE);
    }

    int listen_result = listen(server_socket, 5);
    if (listen_result == -1)
    {
        printf("Error: Failed to listen on the socket\n");
        exit(EXIT_FAILURE);
    }

    printf("Printer server started on port %d\n", port);
    return server_socket;
}

int write_data(char *data_path, char *content, int len)
{
    FILE *fptr = fopen(data_path, "a");
    if (fptr == NULL)
    {
        printf("Error: Failed to open the data file\n");
        exit(EXIT_FAILURE);
    }

    fwrite(content, sizeof(char), len, fptr);
    fclose(fptr);
    return len;
}

char *get_ip_address()
{
    static char ip_address[INET_ADDRSTRLEN]; // buffer for the IP address
    struct sockaddr_in sa;
    socklen_t sa_len = sizeof(sa);

    // Get the hostname
    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    // Resolve hostname to IP address
    struct hostent *host = gethostbyname(hostname);
    if (host == NULL)
    {
        printf("Error: Unable to resolve hostname to IP address\n");
        return NULL;
    }

    // Copy the first IP address from the hostent structure
    inet_ntop(AF_INET, host->h_addr_list[0], ip_address, sizeof(ip_address));
    return ip_address;
}

int main_loop(int server_socket, char *data_path, int buffer_size)
{
    struct pollfd poll_fds[1];
    poll_fds[0].fd = server_socket;
    poll_fds[0].events = POLLIN | POLLPRI;

    while (1)
    {
        int poll_result = poll(poll_fds, 1, -1);
        if (poll_result >= 0)
        {
            if (poll_fds[0].revents & POLLIN)
            {
                struct sockaddr_in cliaddr;
                int addrlen = sizeof(cliaddr);
                int client_socket = accept(server_socket, (struct sockaddr *)&cliaddr, &addrlen);
                printf("Connection from %s\n", inet_ntoa(cliaddr.sin_addr));

                char buf[buffer_size];
                int len = read(client_socket, buf, buffer_size - 1);
                printf("Data size received: %i bytes\n", len);
                write_data(data_path, buf, len);
                close(client_socket);
            }
        }
        // Sleep 0.1 seconds
        usleep(10000);
    }
}

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    int buffer_size = DEFAULT_BUFFER_SIZE;
    char *data_path = DEFAULT_PATH;

    // Get the IP address and log it
    char *ip_address = get_ip_address();
    if (ip_address)
    {
        printf("Printer server starting on IP address: %s\n", ip_address);
    }
    else
    {
        printf("Error: Unable to retrieve IP address.\n");
    }

    printf("Listening on port: %d\n", port);
    printf("Logging print jobs to file: %s\n", data_path);

    int server_socket = create_socket(port);
    main_loop(server_socket, data_path, buffer_size);

    printf("Server end\n");
    close(server_socket);
    return 0;
}
