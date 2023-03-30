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
#include <stdbool.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#define MAXLINE 1024
#define MESSAGE_SIZE 1400
#define BUFFER_SIZE MESSAGE_SIZE * 600

// function for error detection
unsigned short cksum(unsigned short *buf, int count)
{
	register unsigned long sum = 0;
	while (count--)
	{
		sum += *buf++;
		// carry occurred, so wrap around
		if (sum & 0XFFFF0000)
		{
			sum &= 0XFFFF;
			sum++;
		}
	}
	return ~(sum & 0XFFFF);
}

// struct for packets
typedef struct Packet
{
	// sequence number of the packet
	int seqNum;
	// checksum for error detection
	int checksum;
	// actual data
	char message[1400];
};

// convert byte from host to network
void htonPkt(struct Packet *pkt)
{
	pkt->seqNum = htons(pkt->seqNum);
	pkt->checksum = htons(pkt->checksum);
}

// convert byte from network to host
void ntohPkt(struct Packet *pkt)
{
	pkt->seqNum = ntohs(pkt->seqNum);
	pkt->checksum = ntohs(pkt->checksum);
}

int main(int argc, char **argv)
{

	if (argc == 3 && strcmp(argv[1], "-p") == 0)
	{
		printf("[Start]\n");
	}
	else
	{
		printf("The command is incorrect. \n");
		return 0;
	}

	// get port number
	int portNum = atoi(argv[2]);
	// printf(portNum);

	if (portNum < 18000 || portNum > 18200)
	{
		perror("Please provide a port number between 18000 to 18200");
		abort();
	}

	// initialize socket
	int sock;
	// client address
	struct sockaddr_in sin;
	// length of sockaddr_in
	socklen_t addr_len = sizeof(struct sockaddr_in);
	// initialize sin
	memset(&sin, 0, sizeof(sin));

	// create buffer for receiver
	char recvBuf[MAXLINE];
	if (!recvBuf)
	{
		perror("Failure allocating buffer!\n");
		abort();
	}

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("UDP socket creation failed");
		exit(0);
	}

	// setting up UDP socket...
	struct timeval read_timeout;
	read_timeout.tv_sec = 1;
	read_timeout.tv_usec = 0;

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &read_timeout, sizeof(read_timeout)) < 0)
	{
		perror("Failure setting UDP socket\n");
		abort();
	}

	// IPv4 address
	sin.sin_family = AF_INET;
	// bound to any available interface
	sin.sin_addr.s_addr = INADDR_ANY;
	// set sin port number
	sin.sin_port = htons(portNum);

	// bind server socket to sin
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		perror("Failure binding socket to address\n");
		abort();
	}

	// receiver-related variable
	int recvByteCnt, curSeqNum = 0;
	int cksumRecv, cksumRes;
	int totalBytesRecv = 0;
	bool dirReceived = false;

	// file-related variable
	FILE *file = NULL;
	char fileName[256];
	bool fileOpened = false;

	// create 2 packet for receiving data and sending ack
	struct Packet dirPacket, recvPacket, ackPacket;

	while (true)
	{
		recvPacket.checksum = 0;
		recvPacket.seqNum = -1;
		strcpy(recvPacket.message, "");
		recvByteCnt = recvfrom(sock, &recvPacket, sizeof(recvPacket), 0, (struct sockaddr *)&sin, &addr_len);
		// receive packet
		if (recvByteCnt != -1)
		{
			// change it from network order to host order
			ntohPkt(&recvPacket);
			// get original checksum value
			int cksumRecv = recvPacket.checksum;
			// calculate checksum
			int cksumRes = cksum((unsigned short *)&recvPacket, sizeof(struct Packet) / sizeof(unsigned short));

			if (cksumRes != 0)
			{
				// packet corruption
				printf("[recv corrupt packet]\n");
				continue;
			}

			if (!dirReceived && recvPacket.seqNum != 0)
			{
				// continue waiting for directory packet
				continue;
			}
			else if (!dirReceived && recvPacket.seqNum == 0)
			{
				// received directory packet
				dirReceived = true;
				strcpy(fileName, recvPacket.message);

				char *dirc, *dName;
				dirc = strdup(&fileName);
				dName = dirname(dirc);

				// create directory if it not exists
				struct stat st = {0};
				if (stat(dName, &st) == -1)
				{
					printf("Creating directory...\n");
					mkdir(dName, 0777);
				}

				// open the file
				while (!fileOpened)
				{
					printf("Opening file\n");
					char *fName = strcat(fileName, ".recv");
					file = fopen(fName, "w+");
					fileOpened = true;
				}
				int retVal = 0;
				sendto(sock, &retVal, sizeof(int), 0, (const struct sockaddr *)&sin, sizeof(sin));
				curSeqNum++;
				strcpy(recvPacket.message, "");
			}
			else
			{
				int recvSeqNum = recvPacket.seqNum;
				if (recvSeqNum != 65535)
				{
					if (curSeqNum == recvSeqNum)
					{
						int length = strlen(recvPacket.message);
						if (length == 1400)
						{
							totalBytesRecv += MESSAGE_SIZE;
							printf("[recv data] %d (%d) ACCEPTED(in-order)\n", totalBytesRecv, sizeof(recvPacket.message));
							fwrite(recvPacket.message, 1, sizeof(recvPacket.message), file);
							int retVal = recvSeqNum;
							htons(&retVal);
							sendto(sock, &retVal, sizeof(int), 0, (const struct sockaddr *)&sin, sizeof(sin));
							curSeqNum++;
						}
						else
						{
							printf("length: %d\n", length);
							totalBytesRecv += length;
							printf("[recv data] %d (%d) ACCEPTED(in-order)\n", totalBytesRecv, sizeof(recvPacket.message));
							recvPacket.message[length] = '\0';
							fwrite(recvPacket.message, 1, length, file);
							int retVal = recvSeqNum;
							htons(&retVal);
							sendto(sock, &retVal, sizeof(int), 0, (const struct sockaddr *)&sin, sizeof(sin));
							curSeqNum++;
						}
					}
					else if (curSeqNum == recvSeqNum + 1)
					{
						printf("curSeqNum: %d, recvSeqNum: %d\n", curSeqNum, recvSeqNum);
						printf("[recv data] %d (%d) ACCEPTED(out-of-order)\n", recvSeqNum * 1400, sizeof(recvPacket.message));
						int retVal = curSeqNum - 1;
						htons(&retVal);
						sendto(sock, &retVal, sizeof(int), 0, (const struct sockaddr *)&sin, sizeof(sin));
						continue;
					}
				}
				else
				{
					printf("Final ACK\n");
					int retVal = 65535;
					htons(&retVal);
					sendto(sock, &retVal, sizeof(int), 0, (const struct sockaddr *)&sin, sizeof(sin));
					break;
				}
			}
		}
	}
	printf("[completed]\n");
	close(sock);
	exit(0);
}
