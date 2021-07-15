# this is for drawing the experiment figure by generating the 
# csv data table: 
# data0.data: number of traversed edges;
# data1.data: running time.
# data2.data: running time for trunk size. 
import sys
import re
import os.path
import time
import datetime
import csv
import numpy as np
import scipy as sp
import scipy.stats
import re


path = "/mnt/d/git-code/scc-trim-v3/experiments/results/"
files = [
    "our-results-parstep-6-4-2021.csv"
]


all_graph_name = set()

algs = ["trim", "ac4trim", "fasttrim"] # AC3Trim, AC4Trim, AC6Trim
workers = [1, 2, 4, 6, 8, 10, 12, 14, 16, 32]
data = []
parsteps = [1,2, 4,8,16,
        32, 64,128, 256,
        512,1024,
        2048,4096,
        8192,16384,
        32768,65536,
        131072,262144,
        524288,1048576
]


def mean_confidence_interval(data, confidence=0.95):
    a = 1.0*np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * sp.stats.t._ppf((1+confidence)/2., n-1)
    return round(m, 2), round(h, 2), round(h, 2)

####################################################################################
#parse the parallel step size 
def parse_data_parstep_time(model, step, alg):
    global data
    nums = []
    for row in data:
        if model != row["model"]: continue
        if str(step) != row["parstep"]: continue
        if alg != row["alg"]: continue
        nums.append(float(row["mstime"]))
    return mean_confidence_interval(nums)



def output_csvtable_parstep_time(isall):
    global all_graph_name, graph_name
    #header with plus and minus
    head="step,AC3Trim,plus1,minus1,AC4Trim,plus2,minus2,AC6Trim,plus3,minus3,M"
    if isall: graphs = all_graph_name
    else: graphs = graph_name
    for model in graphs:
        print("\\begin{filecontents*}{"+model+"--.csv}") #to make the csv file name different by adding "-"
        print(head)
        for w in parsteps:
            line = [str(w)]
            for alg in algs:
                result = parse_data_parstep_time(model,w, alg)
                for item in result: line.append(str(item))
            print(",".join(line))
        print("\\end{filecontents*}")
        print("")







# read the multi-file to the data
def read_data():
    global data
    for p in files:
        p = path + p
        f = open(p,'rt')
        reader = csv.DictReader(f)
        for row in reader:
            all_graph_name.add(row["model"])
            data.append(row)
        f.close()


def main():
    read_data()
    print("")

    output_csvtable_parstep_time(True)

if __name__ == "__main__":
    main()
