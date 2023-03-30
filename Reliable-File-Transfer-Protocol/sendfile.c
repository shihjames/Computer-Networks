// UDP client program
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h> // for file size reading
#include <sys/time.h> // for time out
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <stdbool.h>

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
	int seqNum;
	int checksum;
	char message[1401];
};

// convert byte from network to host
void htonPkt(struct Packet *pkt)
{
	pkt->seqNum = htons(pkt->seqNum);
	pkt->checksum = htons(pkt->checksum);
}

/*
sendfile -r <recv host>:<recv port> -f <subdir>/<filename>
*/
int main(int argc, char **argv)
{
	if (argc == 5 && strcmp(argv[1], "-r") == 0 && strcmp(argv[3], "-f") == 0)
	{
		printf("[Start]\n");
	}
	else
	{
		printf("The command is incorrect. \n");
		return 0;
	}

	// get recv host & port
	char *recv = argv[2];
	char *recvHost = strtok(recv, ":");
	char *recvPortChar;
	while (recv != NULL)
	{
		recvPortChar = recv;
		recv = strtok(NULL, ":");
	}
	int recvPort = atoi(recvPortChar);
	// get directory
	char *fileDir = argv[4];
	int dirLength = strlen(fileDir);
	printf("recvPort is %d\n", recvPort);
	printf("fileDir is %s\n", fileDir);

	// initialize socket and buffer
	int sockfd;
	char sendbuffer[MAXLINE], recbuffer[MAXLINE];
	struct sockaddr_in servaddr;
	int recvAddr;

	// get server ip address
	struct addrinfo hints, *res;
	int status;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;		// Use IPv4
	hints.ai_socktype = SOCK_DGRAM; // Use UDP

	if ((status = getaddrinfo(recvHost, NULL, &hints, &res)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	else
	{
		recvAddr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr; // ip adress
	}

	// Then we creat UDP socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("UDP socket creation failed");
		exit(0);
	}
	struct timeval read_timeout;
	read_timeout.tv_sec = 1;
	read_timeout.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

	memset(&servaddr, 0, sizeof(servaddr));

	int n, len;
	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(recvPort);
	servaddr.sin_addr.s_addr = recvAddr;

	/* first packet with seq = 0 and file path */
	int recVal = -1;

	// send first packet to test socket and necessary info
	struct Packet firstPacket;
	firstPacket.checksum = 0;
	firstPacket.seqNum = 0;
	strcpy(firstPacket.message, fileDir);
	firstPacket.checksum = cksum((unsigned short *)&firstPacket, sizeof(struct Packet) / sizeof(unsigned short));
	htonPkt(&firstPacket);

	int firstAck = -1;

	// to do change the message type in packet to network headers
	// add packet checksum
	do
	{
		printf("first send \n");
		int firstSend = sendto(sockfd, &firstPacket, sizeof(struct Packet), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
		firstAck = recvfrom(sockfd, &recVal, sizeof(int), 0, (struct sockaddr *)&servaddr, &len);

		if (firstAck != -1)
		{
			// recVal = ntohs(recVal);
			printf("rec val: %d", recVal);
			if (recVal != 0)
				printf("!!!!!%d", recVal);
		}
		else
		{
			printf("recieve packat fail");
		}
	} while (recVal != 0); // 65535 number should be change, we just wanna checked the ack packet num is right.
	sleep(3);
	/* send file data */

	// initial file pointer
	FILE *file = fopen(fileDir, "r");
	if (file == NULL)
	{
		printf("Error opening file!");
		return 1;
	}

	// check total file bytes
	struct stat info;
	stat(fileDir, &info);
	int fileSize = info.st_size;

	printf("file size is %d\n", fileSize);
	int totalBytesLeft = fileSize;

	// int packet_size = sizeof(packet);
	int filePos = 0;
	int round = 0;
	int seq = 0;
	while (totalBytesLeft > 0)
	{
		printf("totalBytesLeft is %d\n", totalBytesLeft);
		bool lastRound = false;
		/* assign approx 1Mb data into buffer*/
		char buffer[BUFFER_SIZE];
		printf("***************\n");
		printf("***************\n");
		printf("***************\n");
		printf("***************\n");
		printf("***************\n");
		printf("***************\n");
		// fseek(file, 5, SEEK_SET); // move file pointer to 5th byte
		fseek(file, filePos, SEEK_SET);
		if (totalBytesLeft >= BUFFER_SIZE)
		{
			fread(buffer, sizeof(char), BUFFER_SIZE, file);
			totalBytesLeft -= BUFFER_SIZE;
			filePos += BUFFER_SIZE;
		}
		else
		{
			printf("!!!!!!!!!!!!!!!!!!\n");
			printf("!!!!!!!!!!!!!!!!!!\n");
			fread(buffer, sizeof(char), totalBytesLeft, file);
			filePos += totalBytesLeft;

			lastRound = true;
		}

		int i = 0;
		time_t lastSendTime;
		/* start sending packet*/
		while (i < 600)
		{
			i++;
			seq = i + round * (BUFFER_SIZE / MESSAGE_SIZE);

			printf("this is the correct seq: %d\n", seq);

			if (lastRound && totalBytesLeft > MESSAGE_SIZE)
			{
				// clean packet
				struct Packet packet;
				packet.checksum = 0;
				packet.seqNum = seq;
				strcpy(packet.message, "");
				totalBytesLeft -= MESSAGE_SIZE;
				memcpy(packet.message, buffer + (i - 1) * MESSAGE_SIZE, 1400);
				packet.checksum = cksum((unsigned short *)&packet, sizeof(struct Packet) / sizeof(unsigned short));
				htonPkt(&packet);

				do
				{
					printf("this is the last round messages!\n");
					printf("[send data] %d (%d)\n", seq * MESSAGE_SIZE, MESSAGE_SIZE);
					sendto(sockfd, &packet, sizeof(packet), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
					int ack = recvfrom(sockfd, &recVal, sizeof(int), 0, (struct sockaddr *)&servaddr, &len);
					if (ack != -1)
					{
						// recVal = atoi(recVal);
					}
					else
					{
						printf("recieve packet fail\n");
					}
					printf("ack: %d\n", recVal);
				} while (recVal != seq);
			}

			else if (lastRound && (totalBytesLeft - MESSAGE_SIZE < 0))
			{
				// clean packet
				struct Packet packet;
				packet.checksum = 0;
				packet.seqNum = seq;
				strcpy(packet.message, "");
				memcpy(packet.message, buffer + (i - 1) * MESSAGE_SIZE, totalBytesLeft);
				packet.message[totalBytesLeft] = '\0';
				packet.checksum = cksum((unsigned short *)&packet, sizeof(struct Packet) / sizeof(unsigned short));
				htonPkt(&packet);
				do
				{
					printf("this is the last packet !!\n");
					printf("[send data] %d (%d)\n", (seq-1) * MESSAGE_SIZE + totalBytesLeft, MESSAGE_SIZE);
					sendto(sockfd, &packet, sizeof(packet), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
					int ack = recvfrom(sockfd, &recVal, sizeof(int), 0, (struct sockaddr *)&servaddr, &len);
					if (ack != -1)
					{
						// recVal = atoi(recVal);
					}
					else
					{
						printf("recieve packet fail\n");
					}
					printf("ack: %d\n", recVal);
				} while (recVal != seq);
				totalBytesLeft = 0;
				break;
			}
			else
			{
				// clean packet
				struct Packet packet;
				packet.checksum = 0;
				packet.seqNum = seq;
				strcpy(packet.message, "");
				memcpy(packet.message, buffer + (i - 1) * MESSAGE_SIZE, MESSAGE_SIZE);
				packet.checksum = cksum((unsigned short *)&packet, sizeof(struct Packet) / sizeof(unsigned short));
				htonPkt(&packet);
				do
				{
					printf("[send data] %d (%d)\n", seq * MESSAGE_SIZE, MESSAGE_SIZE);
					sendto(sockfd, &packet, sizeof(packet), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
					int ack = recvfrom(sockfd, &recVal, sizeof(int), 0, (struct sockaddr *)&servaddr, &len);
					printf("ack: %d\n", recVal);
					if (ack != -1)
					{
						// recVal = atoi(recVal);
					}
					else
					{
						printf("recieve packet fail\n");
					}
				} while (recVal != seq);
			}
		}
		if (lastRound)
		{
			break;
		}
		round++;
	}
	printf("escaped!!!!!!!\n");
	// send last package with seq -1 indicated the end of file
	struct Packet lastPacket;
	lastPacket.checksum = 0;
	lastPacket.seqNum = -1;

	lastPacket.checksum = cksum((unsigned short *)&lastPacket, sizeof(struct Packet) / sizeof(unsigned short));
	htonPkt(&lastPacket);
	// to do change the message type in packet to network headers
	// add packet checksum;
	int lastrecVal = -1;
	do
	{
		printf("last packet seq num = %d\n", ntohs(lastPacket.seqNum));
		int lastSend = sendto(sockfd, &lastPacket, sizeof(lastPacket), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
		int lastAck = recvfrom(sockfd, &lastrecVal, sizeof(int), 0, (struct sockaddr *)&servaddr, &len);
		if (lastAck != -1)
		{
			// printf("valid ack\n");
		}
		else
		{
			printf("recieve packat fail\n");
		}
		ntohs(&lastrecVal);
		printf("last round %d\n", lastrecVal);
	} while (lastrecVal != 65535);

	fclose(file);
	close(sockfd);
	return 0;
}