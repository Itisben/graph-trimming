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
import math

#path = "/home/guob15/Documents/git-code/scc-trim-v3/experiments/results/"
path = "/mnt/d/git-code/scc-trim-v3/experiments/results/"
files = [
    "our-results-large-sample-6-11-2021.csv",
    "our-results-large-6-2-2021.csv"
]

graph_name = [ "com-friendster",
    "twitter",
    "twitter_mpi"]

all_graph_name = set()

algs = ["trim", "ac4trim", "fasttrim"] # AC3Trim, AC4Trim, AC6Trim

data = []

versample = [10,20,30,40,50,60,70,80,90,100]

all_graph_name = set()
def mean_confidence_interval(data, confidence=0.95):
    if len(data) == 1: return round(data[0], 2), 0, 0
    a = 1.0*np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * sp.stats.t._ppf((1+confidence)/2., n-1)
    return round(m, 2), round(h, 2), round(h, 2)

##################################################################################
# edge sample num and time
def parse_data_edgesample(model, percent, alg, is_num, is_es):
    global data
    nums = []
    for row in data:
        if model != row["model"]: continue
        if is_es:
            if "100" != row["vs"]: continue
            if str(percent) != row["es"]: continue
        else: #vs
            if "100" != row["es"]: continue
            if str(percent) != row["vs"]: continue
        if alg != row["alg"]: continue
        if "16" != row["workers"]: continue
        if is_num: nums.append(float(row["traveledge"]))
        else: nums.append(float(row["mstime"]))
    return mean_confidence_interval(nums)
def parse_data_m(model, percent, alg):
    global data
    nums = []
    for row in data:
        if model != row["model"]: continue
        if "100" != row["vs"]: continue
        if str(percent) != row["es"]: continue
        if alg != row["alg"]: continue
        if "16" != row["workers"]: continue
        nums.append(float(row["M"]))
    return round(np.mean(nums), 2)



def output_csvtable_edgesample():
    global graph_name
    #edge num
    head="vs,AC3Trim,plus1,minus1,AC4Trim,plus2,minus2,AC6Trim,plus3,minus3,M,AC4TrimAll"
    for model in graph_name:
        print("\\begin{filecontents*}{"+model+"-esnum.csv}") #to make the csv file name different by adding "-")
        print( head)
        for p in versample:
            line = [str(p)]
            for alg in algs:
                result = parse_data_edgesample(model,p, alg, True, True)
                for item in result: line.append(str(item))
            m = parse_data_m(model, p, alg)
            line.append(str(m))
            ac4all = round(float(line[4]) + m/16,2)
            line.append(str(ac4all))
            print( ",".join(line))
        print( "\\end{filecontents*}")
        print( "")
    #time
    head="vs,AC3Trim,plus1,minus1,AC4Trim,plus2,minus2,AC6Trim,plus3,minus3"
    for model in graph_name:
        print("\\begin{filecontents*}{"+model+"-estime.csv}") #to make the csv file name different by adding "-")
        print( head)
        for p in versample:
            line = [str(p)]
            for alg in algs:
                result = parse_data_edgesample(model,p, alg, False, True)
                for item in result: line.append(str(item))
            print( ",".join(line))
        print( "\\end{filecontents*}")
        print( "")


##################################################################################
def output_csvtable_versample():
    global graph_name
    #header with plus and minus
    head="vs,AC3Trim,plus1,minus1,AC4Trim,plus2,minus2,AC6Trim,plus3,minus3,M,AC4TrimAll"
    for model in graph_name:
        print("\\begin{filecontents*}{"+model+"-vsnum.csv}") #to make the csv file name different by adding "-")
        print( head)
        for p in versample:
            line = [str(p)]
            for alg in algs:
                result = parse_data_edgesample(model,p, alg, True, False)
                for item in result: line.append(str(item))
            m = parse_data_m(model, p, alg)
            line.append(str(m))
            ac4all = round(float(line[4]) + m/16, 2)
            line.append(str(ac4all))
            print( ",".join(line))
        print( "\\end{filecontents*}")
        print( "")
    
    for model in graph_name:
        print("\\begin{filecontents*}{"+model+"-vstime.csv}") #to make the csv file name different by adding "-")
        print( head)
        for p in versample:
            line = [str(p)]
            for alg in algs:
                result = parse_data_edgesample(model,p, alg, False, False)
                for item in result: line.append(str(item))
            print( ",".join(line))
        print( "\\end{filecontents*}")
        print( "")

#######################################################################################
#out put the number of deleted vertices
def parse_data_sample_delete_ratio(model, percent, is_es):
    global data
    nums = []
    for row in data:
        if model != row["model"]: continue
        if is_es:
            if "100" != row["vs"]: continue
            if str(percent) != row["es"]: continue
        else: #vs
            if "100" != row["es"]: continue
            if str(percent) != row["vs"]: continue
        if "16" != row["workers"]: continue
        nums.append(float(row["trimdelete"])/float(row["N"])*100)
    return mean_confidence_interval(nums)
def output_csv_table_delete_num(is_es):
    global graph_name
    head="vs,f,plus1,minus1,t,plus2,minus2,tm,plus3,minus3"
    if is_es: print("\\begin{filecontents*}{deletenumedge.csv}") #to make the csv file name different by adding "-")
    else: print("\\begin{filecontents*}{deletenumver.csv}")
    print(head)
    for p in versample:
        line = [str(p)]
        for model in graph_name:
            result = parse_data_sample_delete_ratio(model,p, is_es)
            for item in result: line.append(str(item))
        print( ",".join(line))
    print( "\\end{filecontents*}")
    print( "")

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
    print( "")
    print( "")
    
    output_csvtable_edgesample()
    output_csvtable_versample()
    
    print( "")
    print( "")
    
    output_csv_table_delete_num(True)
    output_csv_table_delete_num(False)

if __name__ == "__main__":
    main()