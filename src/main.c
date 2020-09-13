#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

#include "server.h"
#include "db/db.h"

DB db;

//////////////////////////////////////////////  make app && make clean && ./app /////////////////////////////////////////////
int main(int argc, char const *argv[])
{
	int epoll_event_count = 0, i = 0;
	char *error = (char *)malloc(1000 * sizeof(char));
	struct epoll_event events[MAX_EPOLL_EVENTS];
	Server server = create_server();

	if (error == NULL)
	{
		fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
		exit(1);
	}

	while (1)
	{
		/**
		 * Wait for events
		 */
		epoll_event_count = epoll_wait(server.epoll_fd, events, MAX_EPOLL_EVENTS, 30000);

		if (epoll_event_count <= 0)
		{
			continue;
		}

		/**
		 * Go through events that occurred
		 */
		for (i = 0; i < epoll_event_count; i++)
		{
			/**
			 * If fd equals to server fd that means that we have pending client
			 */
			if (events[i].data.fd == server.fd)
			{
				server_accept_socket(server);
				continue;
			}

			/**
			 * Parse incoming request
			 */
			Server_request *server_request = server_parse_request(server, events[i].data.fd);

			if (server_request == NULL)
			{
				continue;
			}

			/**
			 * Try to parse request as command
			 */
			Server_command *server_command = server_parse_request_as_command(server_request, error);

			/**
			 * Server response
			 */
			Server_response server_response;

			/**
			 * Check server command
			 */
			if (server_command == NULL)
			{
				server_response.success = 0;
				server_response.message = error;
				server_response.data = "";

				server_send_response(events[i].data.fd, &server_response);

				free(server_request);
				free(server_command);

				continue;
			}

			server_response.success = 0;
			server_response.message = "Command failed.";
			server_response.data = "";
			Item *item;

			if (strncmp(server_command->command, GET_COMMAND, COMMAND_SIZE) == 0)
			{
				item = create_item_from_server_command(server_command);
				printf("%d\n", item);

				if (DB_get(&db, item) == 1)
				{
					server_response.success = 1;
					server_response.message = "Command succeeded.";
					server_response.data = item->value;
				}
			}

			if (strncmp(server_command->command, SET_COMMAND, COMMAND_SIZE) == 0)
			{
				item = create_item_from_server_command(server_command);
				printf("%d\n", item);

				if (DB_set(&db, item) == 1)
				{
					server_response.success = 1;
					server_response.message = "Command succeeded.";
					server_response.data = "";
				}
			}

			if (strncmp(server_command->command, REMOVE_COMMAND, COMMAND_SIZE) == 0)
			{
				item = create_item_from_server_command(server_command);
				printf("%d\n", item);

				if (DB_remove(&db, item) == 1)
				{
					server_response.success = 1;
					server_response.message = "Command succeeded.";
					server_response.data = "";
				}
			}

			/**
			 * Send response
			 */
			server_send_response(events[i].data.fd, &server_response);

			// DB_print(&db);

			free(server_request);
			free(server_command);
		}
	}

	free(error);
	close(server.epoll_fd);
	close(server.fd);

	return 0;
}