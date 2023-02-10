import matplotlib.pyplot as plt
import numpy as np

def calBID(rtt: float, bandwidthDelay: float) -> float:
    return rtt - 2*bandwidthDelay

if __name__ == "__main__":
    filename = "times2.txt"
    xList = []
    yList = []
    with open(filename, "r") as fp:
        lines = fp.readlines()
        for line in lines:
            dataSize, count, bandwidthDelay, rtt = line.split(", ")
            dataSize = int(dataSize)
            count = int(count)
            bandwidthDelay = float(bandwidthDelay)
            rtt = float(rtt)

            bid = calBID(rtt, bandwidthDelay)
            xList.append(dataSize)
            yList.append(bid)
        
    x = np.array(xList)
    y = np.array(yList)

    plt.plot(x, y)
    plt.xlabel("Data Size")
    plt.ylabel("Bandwidth Independent Delay")
    plt.show()