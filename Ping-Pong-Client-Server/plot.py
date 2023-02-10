import matplotlib.pyplot as plt
import numpy as np


def calBID(latency: float, bandwidthDelay: float) -> float:
    return latency - 2*bandwidthDelay


def bidFigure(filename):
    # filename = "time2.txt"
    xList = []
    yList = []
    with open(filename, "r") as fp:
        lines = fp.readlines()
        for line in lines:
            dataSize, count, bandwidthDelay, latency = line.split(", ")
            dataSize = int(dataSize)
            count = int(count)
            bandwidthDelay = float(bandwidthDelay)
            latency = float(latency)

            bid = calBID(latency, bandwidthDelay)
            xList.append(dataSize)
            yList.append(bid)

    x = np.array(xList)
    y = np.array(yList)

    plt.plot(x, y)
    plt.xlabel("Data Size")
    plt.ylabel("Bandwidth Independent Delay")
    plt.show()


def bandwidthFigure(filename):
    # filename = "times2.txt"
    xList = []
    yList = []
    with open(filename, "r") as fp:
        lines = fp.readlines()
        for line in lines:
            dataSize, count, bandwidthDelay, latency = line.split(", ")
            dataSize = int(dataSize)
            count = int(count)
            bandwidthDelay = float(bandwidthDelay)
            latency = float(latency)

            bid = calBID(latency, bandwidthDelay)
            xList.append(dataSize)
            yList.append(2*dataSize*10000/(latency - bid))

    x = np.array(xList)
    y = np.array(yList)

    plt.plot(x, y)
    plt.xlabel("Data Size(Byte)")
    plt.ylabel("Bandwidth(Byte/ms)")
    plt.show()


if __name__ == "__main__":
    bandwidthFigure("time2.txt")
    # bandwidthFigure("times2.txt")
    bidFigure("time2.txt")
