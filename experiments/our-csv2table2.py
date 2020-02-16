
import sys
import re
import os.path
import time
import datetime
import csv
import numpy as np
import scipy as sp
import scipy.stats
from sets import Set


allmodels    = Set([])
sortedmodels = []
sortedvalues = []


def checkfile(file):
    if not (os.path.isfile(file)):
        print "ERROR: cannot find file: {}".format(file)
        sys.exit()


def mean_confidence_interval(data, confidence=0.95):
    if (len(data) == 1):
        return data[0], 0
    a = 1.0*np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * sp.stats.t._ppf((1+confidence)/2., n-1)
    return m, h


def addtodict(dictx, model, time):
    if not model in dictx:
        dictx[model] = []
    dictx[model].append(float(time))

def addtodict2(dictx, model, value):
    if not model in dictx:
        dictx[model] = value

def sortmodels(seq):
    # sort models on values
    changed = True
    while changed:
        changed = False
        for i in xrange(len(seq) - 1):
            if seq[i] > seq[i+1]:
                seq[i], seq[i+1] = seq[i+1], seq[i]
                sortedmodels[i], sortedmodels[i+1] = sortedmodels[i+1], sortedmodels[i]
                changed = True

def parsefile1(file, graph_list):
    trim    = {}
    fasttrim = {}


    # check the maximum number of workers
    f = open(file, 'rb')
    reader = csv.DictReader(f)

    workernums = ['1','2','4','8','16']
    #workernums = ['1']
   
    for row in reader:
        for workernum in workernums:
            if row["alg"] == "trim"   and row["workers"] == workernum:   addtodict(trim, row["model"]+(workernum), row["mstime"])
            if row["alg"] == "fasttrim"   and row["workers"] == workernum:  addtodict(fasttrim, row["model"]+(workernum), row["mstime"])
          
    
    f.close()

 
    print "\\begin{table*}"
    print "\\begin{tabular}{l|r r r r r |}"
    print "\\hline"
    print "  &  \multicolumn{5}{c}{Speedups AC6Trim VS AC1Trim}  \\\\"
    print " Name & 1 & 2 & 4 & 8 & 16 \\\\"
    print "\\hline"

    #print trim
    for filename,name in graph_list:
        line = ""
        for workernum in workernums:
            model = filename[0:-4]+workernum
            trim_time  = mean_confidence_interval(trim[model])[0]
            fasttrim_time   = mean_confidence_interval(fasttrim[model])[0]

            # speedup
            fasttrim_vs_trim = trim_time / fasttrim_time

            buf = " %.1f "  % (fasttrim_vs_trim)
            if workernum != '16': buf += ' &'
            line += buf
            
            #print "%s &     %.1f &      %.1f & %d & %d & %.1f \\\\" \
            #    % (name,    tarjan_time,    trim_time, repeat, delete, trim_vs_tarjan)
        print name + ' &' + line + "\\\\"
    print "\\hline"
    print "\\end{tabular}"
    print "\end{table*}"





def parsefileGraphs2(file):
    graph_list =[]
    f = open(file, 'rb')
    reader = csv.DictReader(f)
    global g_graphs
    for row in reader:
        name = row["Name"]
        if name == "" or name == "*": break
        filename = row["filename"]
        graph_list.append((filename, name))
    f.close()
    return graph_list


def main():
    global ALG1, ALG2, WORKERS1, WORKERS2
    N_ARG = len(sys.argv)
    if (N_ARG == 3):
        INFILE  = str(sys.argv[1])
        GFILE   = str(sys.argv[2])
        checkfile(INFILE)
        graph_list = parsefileGraphs2(GFILE)
        parsefile1(INFILE, graph_list)
        
    else:
        print "ERROR: invalid command"
        print "Usage:"
        print " - {} INFILE  GFILE           # print table".format(str(sys.argv[0]))


if __name__ == "__main__":
    main()
