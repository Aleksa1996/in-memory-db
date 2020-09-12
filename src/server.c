#include <netinet/in.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "epoll_util.h"
#include "server.h"
#include "cJSON/cJSON.h"

/**
 * Create server
 */
Server create_server()
{
    int server_socket, opt = 1, epoll;
    Server server;
    struct sockaddr_in address;

    /**
	 * Create server socket and set options to it
	 */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    /**
	 * Setup address
	 */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    /**
	 * Bind socket and listen
	 */
    bind(server_socket, (struct sockaddr *)&address, sizeof(address));
    listen(server_socket, 3);

    /**
     * Fill server structure
     */
    server.fd = server_socket;
    server.epoll_fd = epoll_create1(0);
    server.port = SERVER_PORT;
    server.address = address;

    /**
     * Add server socket to epoll watcher
     */
    epoll_ctl_add(server.epoll_fd, server.fd);

    return server;
}

/**
 * Accept socket that wants to connect to server
 */
int server_accept_socket(Server server)
{
    int accepted_socket, address_size = sizeof(struct sockaddr_in);
    accepted_socket = accept(server.fd, (struct sockaddr *)&server.address, &address_size);
    epoll_ctl_add(server.epoll_fd, accepted_socket);

    return accepted_socket;
}

/**
 * Read data from socket
 */
char *read_data_from_socket(int socket)
{
    int read_number, read_size = 1024, data_size = 1024, i = 0, character_index = 0;
    char *data, read_buffer[read_size];

    /**
     * Dynamic allocation of memory
     */
    data = (char *)malloc(data_size * sizeof(char));
    if (data == NULL)
    {
        fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
        exit(1);
    }

    while (read_number = read(socket, read_buffer, read_size))
    {
        /**
         * Assign additional memory if needed
         */
        if ((character_index + read_number) > data_size)
        {
            data_size *= 2;
            data = realloc(data, data_size * sizeof(char));
        }

        /**
         * Append read data
         */
        for (i = 0; i < read_number; i++)
        {
            data[character_index++] = read_buffer[i];
        }

        /**
         * Here we know that we read all data that came from socket
         */
        if (read_number < read_size)
        {
            data[character_index++] = '\0';
            break;
        }
    }

    if (read_number == 0)
    {
        return NULL;
    }

    return data;
}

/**
 * Parse server request
 */
Server_request *server_parse_request(Server server, int socket)
{
    Server_request *server_request = (Server_request *)malloc(sizeof(server_request));
    char *data = NULL;

    if (server_request == NULL)
    {
        fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
        exit(1);
    }

    /**
     * Read data from socket
     */
    data = read_data_from_socket(socket);

    if (data == NULL)
    {
        epoll_ctl_remove(server.epoll_fd, socket);
        close(socket);

        return NULL;
    }

    server_request->content_length = 1;
    server_request->data = data;

    return server_request;
}

/**
 * Parse server request as command
 */
Server_command *server_parse_request_as_command(Server_request *server_request, char *error)
{
    Server_command *server_command = (Server_command *)malloc(sizeof(Server_command));
    cJSON *json_request = cJSON_Parse(server_request->data);

    if (server_command == NULL)
    {
        fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
        exit(1);
    }

    if (json_request == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            strcpy(error, "JSON parse error: Request not valid.");
            fprintf(stderr, "cJSON parse error: %s\n", error_ptr);
            return NULL;
        }
    }

    cJSON *command = cJSON_GetObjectItem(json_request, "command");
    if (cJSON_IsString(command) == 0 || (command->valuestring == NULL))
    {
        strcpy(error, "JSON parse error: 'command' key not found.");
        printf("cJSON parse error: 'command' key not found.\n");
        return NULL;
    }

    cJSON *key = cJSON_GetObjectItemCaseSensitive(json_request, "key");
    if (cJSON_IsString(key) == 0 || (key->valuestring == NULL))
    {
        strcpy(error, "JSON parse error: 'key' key not found.");
        printf("cJSON parse error: 'key' key not found.\n");
        return NULL;
    }

    cJSON *value = cJSON_GetObjectItemCaseSensitive(json_request, "value");
    if (cJSON_IsString(value) == 0 || (value->valuestring == NULL))
    {
        strcpy(error, "JSON parse error: 'value' key not found.");
        printf("cJSON parse error: 'value' key not found.\n");
        return NULL;
    }

    server_command->command = strdup(command->valuestring);
    server_command->key = strdup(key->valuestring);
    server_command->value = strdup(value->valuestring);

    if (server_command_validate(server_command, error) == 0)
    {
        // strcpy(error, command_validation_error);
        printf("%s\n", error);
        return NULL;
    }

    cJSON_Delete(json_request);

    return server_command;
}

/**
 * Server send response to client
 */
int server_send_response(int socket, Server_response *server_response)
{
    char *data = NULL;

    cJSON *json_response = cJSON_CreateObject();

    if (
        cJSON_AddBoolToObject(json_response, "success", server_response->success) == NULL ||
        cJSON_AddStringToObject(json_response, "message", server_response->message) == NULL ||
        cJSON_AddStringToObject(json_response, "data", server_response->data) == NULL)
    {
        return 0;
    }

    data = cJSON_PrintUnformatted(json_response);

    write(socket, data, strlen(data));

    free(data);
    cJSON_Delete(json_response);

    return 1;
}

/**
 * Validate server command received from client
 */
int server_command_validate(Server_command *server_command, char *error)
{
    char allowed_commands[3][100] = {
        {"get"},
        {"set"},
        {"remove"}};
    int allowed_commands_size = sizeof(allowed_commands), allowed_commands_row_size = sizeof(allowed_commands[0]), i, j, valid = 0;

    /**
     * Command is supported ?
     */
    for (i = 0; i < allowed_commands_size / allowed_commands_row_size; i++)
    {
        if (strcmp(server_command->command, allowed_commands[i]) == 0)
        {
            valid = 1;
        }
    }

    if (valid == 0)
    {
        sprintf(error, "Command '%s' does not exists.", server_command->command);
        return 0;
    }

    /**
     * Value required when we use set command
     */
    if (strcmp(server_command->command, "set") == 0 && server_command->value[0] == '\0')
    {
        sprintf(error, "Command 'set' requires a value.", server_command->command);
        return 0;
    }

    return 1;
}