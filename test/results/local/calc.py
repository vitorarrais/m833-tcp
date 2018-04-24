from numpy import genfromtxt
import numpy as np
import scipy as sp
import scipy.stats
import matplotlib.pyplot as plt


def mean_confidence_interval(data, confidence=0.95):
    a = 1.0*np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * sp.stats.t._ppf((1+confidence)/2., n-1)
    return m, h


for i in range(1,7):

    data = genfromtxt("result{}.csv".format(i), delimiter = ',')
    m0, h0 = mean_confidence_interval(data[:,0])
    m1, h1 = mean_confidence_interval(data[:,1])

    plt.hist(data[:,0], len(data[:,0]), normed=0, facecolor='green', alpha=0.75)
    plt.title("Option {}".format(i))
    plt.xlabel("Time in µs")
    plt.savefig("option_{}_query.png".format(i))
    plt.clf()
    plt.hist(data[:,1], len(data[:,1]), normed=0, facecolor='blue', alpha=0.75)
    plt.xlabel("Time in µs")
    plt.savefig("option_{}_communication.png".format(i))
    plt.clf()

    with open("mean.txt", "a") as file:
        file.write("Option {}\nQuery: {} +- {}\nCommunication: {} +- {}\n\n".format(i, m0, h0, m1, h1))
