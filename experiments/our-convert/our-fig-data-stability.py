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

#path = "/home/guob15/Documents/git-code/scc-trim-v3/experiments/results/"
path = "/mnt/d/git-code/scc-trim-v3/experiments/results/"
files = [
    "our-results-6-9-2021.csv"
    , "our-results-large-6-2-2021.csv"
]
graph_name = [
    "leader_filters.7",  "wiki_talk_en", "livej" 
]

graph_name_all = [
    "cambridge.6",
    "bakery.6",
    "leader_filters.7",

    "dbpedia",
    "baidu",
    "livej",
    "patent", #"pokec","
    "wiki_talk_en"
    "wikitalk"

    "er-1m-8m",
    "ba-1m-8m",
    "rmat-1m-8m",

    "com-friendster",
    "twitter",
    "twitter_mpi"
]

graph_title_all = [
    "cambridge.6",
    "bakery.6",
    "leader-filters.7",

    "dbpedia",
    "baidu",
    "livej",
    "patent", #"pokec","
    "wiki-talk-en"
    "wikitalk"

    "er-1m-8m",
    "ba-1m-8m",
    "rmat-1m-8m",

    "com-friendster",
    "twitter",
    "twitter-mpi"
]


algs = ["trim", "ac4trim", "fasttrim"] # AC3Trim, AC4Trim, AC6Trim
data = []
all_graph_name=set()


#return the mean, min and max time. 
def parse_data_edge_num(model, worker, alg, id):
    global data
   
    i = 0
    for row in data:
        if model != row["model"]: continue
        if str(worker) != row["workers"]: continue
        if alg != row["alg"]: continue
        if i == id: 
            return(float(row["traveledge"]))
        i = i+1

def parse_data_m(model):
    global data
    for row in data:
        if model != row["model"]: continue
        return row["M"]
        
#csv table for running time
def output_csvtable_edge_num():
    global graph_name
    #header with plus and minus
    head="repeat,AC3Trim,AC4Trim,AC6Trim"
    for model in graph_name:
        print ("\\begin{filecontents*}{"+model+"-snum.csv}")
        print (head)
        for id in range(50):
            line =[str(id+1)]
            for alg in algs:
                if alg is "ac4trim":
                    result = parse_data_edge_num(model, 16, alg, id) + float(parse_data_m(model))/16
                else: result = parse_data_edge_num(model, 16, alg, id)
                line.append(str(result))
            print (",".join(line))
        print ("\\end{filecontents*}")
        print ("")


################################################################
#return the mean, min and max time. 
def parse_data_time(model, worker, alg, id):
    global data
   
    i = 0
    for row in data:
        if model != row["model"]: continue
        if str(worker) != row["workers"]: continue
        if alg != row["alg"]: continue
        if i == id: 
            return(float(row["mstime"]))
        i = i+1

#csv table for running time
def output_csvtable_time():
    global graph_name
    #header with plus and minus
    head="repeat,AC3Trim,AC4Trim,AC6Trim"
    for model in graph_name:
        print ("\\begin{filecontents*}{"+model+"-stime.csv}")
        print (head)
        for id in range(50):
            line =[str(id+1)]
            for alg in algs:
                result = parse_data_time(model, 16, alg, id)
                line.append(str(result))
            print (",".join(line))
        print ("\\end{filecontents*}")
        print ("")

############################################################


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
    print ("")
    print ("")
    output_csvtable_edge_num()
    output_csvtable_time()
    print ("")
    print ("")
    
    #ShowLatex()

if __name__ == "__main__":
    main()
