# Ping-Pong Client Server

1. Concept

This program aim to measure the Bandwidth Independent Delay (BID). Since RTT includes the package transmission time affected by bandwidth, i.e. Bandwidth Dependent Delay (BDD), we calculate the Bandwidth Independent Delay by subtracting the Bandwidth Dependent Delay from RTT.

RTT − BDD = BID

We can verify it by sending packets with different data sizes.When the data size increases, the BID increases and the two have a linear relationship as mentioned in the specifications, this verifies the accuracy of our BID results.

2. Experiment
  1. We designed to send 655 packets with data size from 18 to 65518, from the client to the server and measured the total time spent.
  2. We will record a timestamp before and after the send() function tocalculate the dependent delay.
  3. When Calculating RTT, we ignored the latency for the ack package since it is small. After testing, we would made the latencies we observed and the data sizes into pair.
  4. In order to visual data, we used RANSACRegressor package to deal with outliers and find the line of best fit.
  5. Plot implementation Code(Python):

3. Result

![alt text](https://github.com/shihjames/Computer-Networks/blob/Ping-Pong-Client-Server/bid.png?raw=true)

The y_intercept of the line corresponds to Bandwidth Independent Delay and the x_intercept of the line corresponds to Data Size.
From the trend line in the graph, when the datasize we sent become bigger, the BID would also become bigger too.
It can be seen that the data on the x and y axes are proportional to each other, thereby proving our hypothesis to be correct.

4. Discussion
There are also some isssue we would like to discuss, trying to explained the outlier and error issues.
  1. Resource management problem:
  When calling the send function, there are overhead considerations with regards to the network and the operating system. When passing a buffer address to the send function, the address does not directly correspond to a physical address, resulting in overhead from the operating system, which is a clear issue with resource management.
  2. Packet transmission time overlapping problem:
  When the data size is too large, it will not only send one package, but multiple packages. This will result in inaccuracies in the calculation of independent delay, as the server will return an acknowledgment for the first received package before the client has finished sending all packages. As a result, when the data size increases, the calculation error of transmission time will also increases.
