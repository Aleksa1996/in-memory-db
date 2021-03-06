#include <netinet/in.h>
#include "db/item.h"

/**
 * Defines
 */
#define SERVER_PORT 9696
#define MAX_EPOLL_EVENTS 1000

#define COMMAND_SIZE 1000
#define GET_COMMAND "get"
#define SET_COMMAND "set"
#define REMOVE_COMMAND "remove"

/**
 * Server structure
 */
typedef struct Server
{
    int fd;
    int epoll_fd;
    int port;
    struct sockaddr_in address;
} Server;

/**
 * Server Request structure
 */
typedef struct Server_request
{
    char *data;
    int content_length;
} Server_request;

/**
 * Server Response structure
 */
typedef struct Server_response
{
    int success;
    char *message;
    char *data;
} Server_response;

/**
 * Server Command structure
 */
typedef struct Server_command
{
    char *command;
    char *key;
    char *value;
} Server_command;

Server create_server();

int server_accept_socket(struct Server server);

char *read_data_from_socket(int socket);

Server_request *server_parse_request(Server server, int socket);

Server_command *server_parse_request_as_command(Server_request *server_request, char *error);

int server_send_response(int socket, Server_response *server_response);

int server_command_validate(Server_command *server_command, char *error);

Item *create_item_from_server_command(Server_command *server_command);