# Reliable File Transfer Protocol

### Packet format

-   A packet is 1400 bytes
    -   seqNum (unsigned short, 2 bytes): the sequence number of the packet
    -   checksum(unsigned short, 2 bytes): the checksum of the packet
    -   char message[1400]: Each packet could store upto 1466 bytes in this field.

### Protocals and Algorithms

-   Using approximately 1 MB memory to send file which could be large(up to 30MB).
    -   sendfile
        -   "buffer" is a buffer used to store data read from the file. The limit of file_buffer is 1400(bytes) \* 600.
    -   recvfile
        -   "recvBuf" could store up to 690 packets ~ 1M.
-   Protocols
    -   Stage 1: Ready to send packets
        -   sendfile sends directory packet seqNum = 0 to let recvfile know where to store the data.
        -   recvfile will calculate the checksum and send ACK if the packet is not corrupted.
        -   Once sendfile gets "ACK 0" from recvfile, sendfile will start to send data.
    -   Stage 2: Send packets
        -   First, we send packets one by one and wait for the corresponding ACK.
        -   recvfile will calculate the checksum and send ACK if the packet is not corrupted.
        -   If the sendfile doesn't receive ACK, simply send the packet agian.
    -   Stage 3: Disconnect
        -   When all packets were sent and got ACKs, the sendfile will send the last packet with seqNum 65535.
        -   recvfile will know this is the last packet and close the socket.
        -   If the socket didn't receive anything for 10 secs, the socket will close.

### Execution

-   run make file
    -   make all
-   run sendfile
    -   ./sendfile -r <recv_host>:<recv_port> -f <subdir>/<filename>
    -   e.g. sendfile -r 128.42.124.180:18010 -f ./test/test.txt
-   run recvfile
    -   ./recvfile -p <recv port>
    -   e.g. recvfile -p 18010
