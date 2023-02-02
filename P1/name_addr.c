#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

/*
Simple program that exercies
getaddrinfo(), getnameinfo(), inet_aton(), inet_ntoa()

Input parameter: domain name (e.g. www.rice.edu)
*/

#define MAX_LEN 100

int main(int argc, char **argv)
{
	char *hostname;
	char hostname2[MAX_LEN];
	char *ipv4addr_in_ascii; /* i.e. a.b.c.d format */
	struct addrinfo *getaddrinfo_result, hints;
	struct sockaddr_in *socketaddr_ipv4_structure;
	struct sockaddr_in socketaddr_ipv4_structure2;

	/* start with the hostname from the command line input */

	hostname = argv[1];
	/* initializing all its members to zero */
	memset(&hints, 0, sizeof(struct addrinfo));
	/* indicates we want IPv4 */
	hints.ai_family = AF_INET;

	/*
	hostname: A pointer to a null-terminated string that contains the name of the host to be resolved.
	NULL: A pointer to a null-terminated string that specifies a service name.
	&hints: A pointer to a struct addrinfo structure that contains hints about the type of socket the caller supports. (In this case, IPv4)
	&getaddrinfo_result: A pointer to a pointer to a struct addrinfo structure that will receive the linked list of struct addrinfo structures returned by the function.
	*/
	if (getaddrinfo(hostname, NULL, &hints, &getaddrinfo_result) == 0)
	{
		/* contains an IPv4 address in network byte order */
		socketaddr_ipv4_structure = (struct sockaddr_in *)getaddrinfo_result->ai_addr;
		/* converts an IPv4 address from network byte order to a string representation in dotted-decimal notation */
		ipv4addr_in_ascii = inet_ntoa(socketaddr_ipv4_structure->sin_addr);
		printf("IP address in dot format after manipulations: %s\n", ipv4addr_in_ascii);
		freeaddrinfo(getaddrinfo_result);
	}

	/* now suppose we start with a valid ipv4addr_in_ascii e.g. 128.42.10.1 */
	/* converts an IPv4 address in dotted-decimal notation to a 32-bit binary value in network byte order */
	inet_aton(ipv4addr_in_ascii, &socketaddr_ipv4_structure2.sin_addr);
	socketaddr_ipv4_structure2.sin_family = AF_INET;
	socketaddr_ipv4_structure2.sin_port = 0;

	/*
	socket_addr: A pointer to a socket address structure (for example, a struct sockaddr_in for an IPv4 address).
	socket_addr_len: The length of the socket address structure.
	host: A pointer to a buffer that will receive the host name.
	host_len: The size of the buffer that will receive the host name.
	serv: A pointer to a buffer that will receive the service name.
	serv_len: The size of the buffer that will receive the service name.
	flags: Flags to modify the behavior of the function.
	*/
	if (getnameinfo((struct sockaddr *)&socketaddr_ipv4_structure2, sizeof(struct sockaddr_in), hostname2, MAX_LEN, NULL, 0, 0) == 0)
	{
		printf("Host name after manipulations: %s\n", hostname2);
	}

	return 0;
}
