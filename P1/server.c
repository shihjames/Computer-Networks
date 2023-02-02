#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

/* a linked list node data structure to maintain application information related to a connected socket */
/*
socket: socket descriptor that is associated with the client connected to this node.
client_addr: holds the address information of the client connected to this node.
pending_data: flag that indicates whether there is more data to be sent to the client associated with this node.
*next: a pointer to the next node in the linked list.
*/
struct node
{
	int socket;
	struct sockaddr_in client_addr;
	int pending_data;
	struct node *next;
};

/* remove the data structure associated with a connected socket used when tearing down the connection */
void dump(struct node *head, int socket)
{
	struct node *current, *temp;

	current = head;

	while (current->next)
	{
		if (current->next->socket == socket)
		{
			/* remove */
			temp = current->next;
			current->next = temp->next;
			/* don't forget to free memory */
			free(temp);
			return;
		}
		else
		{
			current = current->next;
		}
	}
}

/* create the data structure associated with a connected socket */
void add(struct node *head, int socket, struct sockaddr_in addr)
{
	struct node *new_node;

	new_node = (struct node *)malloc(sizeof(struct node));
	new_node->socket = socket;
	new_node->client_addr = addr;
	new_node->pending_data = 0;
	new_node->next = head->next;
	head->next = new_node;
}

/* simple server, takes one parameter, the server port number */
int main(int argc, char **argv)
{
	/* assign port number and check validity */
	unsigned short server_port = atoi(argv[1]);
	if (server_port < 18000 || server_port > 18200)
	{
		perror("Invalid port number. Please provide a port number between 18000 and 18200!");
		abort();
	}

	/* socket and option variables */
	/*
	sock: a file descriptor for the server socket.
	new_sock: new socket that is created when a client establishes a connection with the server.
	max: the maximum file descriptor number used in the select() function.
	optval: used to set the SO_REUSEADDR socket option, which allows the server to bind to an address that is already in use.
	*/
	int sock, new_sock, max, optval = 1;

	/* server socket address variables */
	struct sockaddr_in sin, addr;

	/* socket address variables for a connected client */
	socklen_t addr_len = sizeof(struct sockaddr_in);

	/* maximum number of pending connection requests */
	int BACKLOG = 5;

	/* variables for select */
	fd_set read_set, write_set;
	struct timeval time_out;
	int select_retval;

	/* a silly message */
	char *message = "Welcome! COMP/ELEC 429 Students!\n";

	/* number of bytes sent/received */
	int count;

	/* numeric value received */
	int num;

	/* linked list for keeping track of connected sockets */
	struct node head;
	struct node *current, *next;
	head.socket = -1;
	head.next = 0;

	/* a buffer to read data */
	char *buf;
	int BUF_LEN = 1000;
	buf = (char *)malloc(BUF_LEN);

	/* initialize dummy head node of linked list */
	head.socket = -1;
	head.next = 0;

	/* create a server socket to listen for TCP connection requests */
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("opening TCP socket");
		abort();
	}

	/* set option so we can reuse the port number quickly after a restart */
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
	{
		perror("setting TCP socket option");
		abort();
	}

	/* fill in the address of the server socket */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(server_port);

	/* bind server socket to the address */
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		perror("binding socket to address");
		abort();
	}

	/* put the server socket in listen mode */
	if (listen(sock, BACKLOG) < 0)
	{
		perror("listen on socket failed");
		abort();
	}

	/* now we keep waiting for incoming connections,
	check for incoming data to receive,
	check for ready socket to send more data */
	while (1)
	{

		/* set up the file descriptor bit map that select should be watching */
		FD_ZERO(&read_set);	 /* clear everything */
		FD_ZERO(&write_set); /* clear everything */

		FD_SET(sock, &read_set); /* put the listening socket in */
		max = sock;				 /* initialize max */

		/* put connected sockets into the read and write sets to monitor them */
		for (current = head.next; current; current = current->next)
		{
			FD_SET(current->socket, &read_set);

			if (current->pending_data)
			{
				/*
				there is data pending to be sent, monitor the socket in
				the write set so we know when it is ready to take more data
				*/
				FD_SET(current->socket, &write_set);
			}

			if (current->socket > max)
			{
				/* update max if necessary */
				max = current->socket;
			}
		}

		time_out.tv_usec = 100000; /* 1-tenth of a second timeout */
		time_out.tv_sec = 0;

		/* used to handle multiple connections */
		select_retval = select(max + 1, &read_set, &write_set, NULL, &time_out); /* make sure to pass max+1 !!! */
		if (select_retval < 0)
		{
			perror("select failed");
			abort();
		}

		if (select_retval == 0)
		{
			/* no descriptor ready, timeout happened */
			continue;
		}

		if (select_retval > 0) /* at least one file descriptor is ready */
		{
			if (FD_ISSET(sock, &read_set)) /* check the server socket */
			{
				/* there is an incoming connection, try to accept it */
				new_sock = accept(sock, (struct sockaddr *)&addr, &addr_len);

				if (new_sock < 0)
				{
					perror("error accepting connection");
					abort();
				}

				/*
				make the socket non-blocking so send and recv will
				return immediately if the socket is not ready.
				this is important to ensure the server does not get
				stuck when trying to send data to a socket that
				has too much data to send already.
				*/
				if (fcntl(new_sock, F_SETFL, O_NONBLOCK) < 0)
				{
					perror("making socket non-blocking");
					abort();
				}

				/* the connection is made, everything is ready */
				/* let's see who's connecting to us */
				printf("Accepted connection. Client IP address is: %s\n", inet_ntoa(addr.sin_addr));

				/* remember this client connection in our linked list */
				add(&head, new_sock, addr);

				/* let's send a message to the client just for fun */
				count = send(new_sock, message, strlen(message) + 1, 0);
				if (count < 0)
				{
					perror("error sending message to client");
					abort();
				}
			}

			/*
			heck other connected sockets, see if there is
			anything to read or some socket is ready to send
			more pending data
			*/
			for (current = head.next; current; current = next)
			{
				next = current->next;

				if (FD_ISSET(current->socket, &read_set))
				{
					/* we have data from a client */

					count = recv(current->socket, buf, BUF_LEN, 0);
					if (count <= 0)
					{
						/* something is wrong */
						if (count == 0)
						{
							printf("Client closed connection. Client IP address is: %s\n", inet_ntoa(current->client_addr.sin_addr));
						}
						else
						{
							perror("error receiving from a client");
						}

						/* connection is closed, clean up */
						close(current->socket);
						dump(&head, current->socket);
					}
					else
					{
						ssize_t sendBytes = send(current->socket, buf, count, MSG_DONTWAIT);
						printf("Send Bytes: %ld\n", sendBytes);
					}
				}
			}
		}
	}
}
