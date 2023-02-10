# Ping-Pong Client Server

This program aim to measure the Bandwidth Independent Delay (BID). Since RTT includes the package transmission time affected by bandwidth, i.e. Bandwidth Dependent Delay (BDD), we calculate the Bandwidth Independent Delay by subtracting the Bandwidth Dependent Delay from RTT.

RTT − BDD = BID

We can verify it by sending packets with different data sizes.When the data size increases, the BID increases and the two have a linear relationship as mentioned in the specifications, this verifies the accuracy of our BID results.
