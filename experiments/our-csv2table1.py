
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


def parsefile(file, graph_list, workernum = "16"):
    tarjan  = {}
    trim    = {}
    fasttrim = {}
    trim_repeat = {}
    trim_delete = {}


    # check the maximum number of workers
    f = open(file, 'rb')
    reader = csv.DictReader(f)
    for row in reader:
        if row["alg"] == "tarjan"  and row["workers"] == "1":   addtodict(tarjan, row["model"], row["mstime"])
        if row["alg"] == "trim"   and row["workers"] == workernum:   addtodict(trim, row["model"], row["mstime"])
        if row["alg"] == "fasttrim"   and row["workers"] == workernum:  addtodict(fasttrim, row["model"], row["mstime"])
        if row["alg"] == "trim" : 
            addtodict2(trim_repeat, row["model"], row["trimrepeat"])
            addtodict2(trim_delete, row["model"], row["trimdelete"])
    f.close()
 
    #print "\\begin{table*}"
    print "\\begin{tabular}{l|r|rrrr|rrrr}"
    print "\\hline"
    print "  & Tarjan & \multicolumn{4}{c|}{AC4Trim} & \multicolumn{4}{c|}{AC6Trim} \\\\"
    print " Name & Time(ms) & Time(ms) & \#Repeat & \#Delete V & Speedup VS Tarjan & \
        Time(ms) &\#Delete V & Speedup VS Tarjan & Speedup VS AC4Trim\\\\"
    print "\\hline"

    for filename,name in graph_list:
        model = filename[0:-4]
        tarjan_time  = mean_confidence_interval(tarjan[model])[0]
        trim_time  = mean_confidence_interval(trim[model])[0]
        fasttrim_time   = mean_confidence_interval(fasttrim[model])[0]
        repeat  = int( trim_repeat[model])
        delete = int(trim_delete[model])

        # speedup
        trim_vs_tarjan = tarjan_time / trim_time
        fasttrim_vs_tarjan  = tarjan_time / fasttrim_time
        fasttrim_vs_trim = trim_time / fasttrim_time

        # print name
        # print repeat
        # print delete
        print "%s &     %.1f &      %.1f & %d & %d & %.1f &    %.1f & %d & %.1f & %.1f \\\\" \
            % (name,    tarjan_time,    trim_time, repeat, delete, trim_vs_tarjan, \
                fasttrim_time, delete, fasttrim_vs_tarjan, fasttrim_vs_trim)

    print "\\hline"
    print "\\end{tabular}"
    #print "\end{table*}"




def parsefile1(file, graph_list, workernum = "16"):
    tarjan  = {}
    trim    = {}
    fasttrim = {}
    trim_repeat = {}
    trim_delete = {}


    # check the maximum number of workers
    f = open(file, 'rb')
    reader = csv.DictReader(f)
    for row in reader:
        if row["alg"] == "tarjan"  and row["workers"] == "1":   addtodict(tarjan, row["model"], row["mstime"])
        if row["alg"] == "trim"   and row["workers"] == workernum:   addtodict(trim, row["model"], row["mstime"])
        if row["alg"] == "fasttrim"   and row["workers"] == workernum:  addtodict(fasttrim, row["model"], row["mstime"])
        if row["alg"] == "trim" : 
            addtodict2(trim_repeat, row["model"], row["trimrepeat"])
            addtodict2(trim_delete, row["model"], row["trimdelete"])
    f.close()
 
    print "\\begin{table*}"
    print "\\begin{tabular}{l|r|rrrr|}"
    print "\\footnotesize"
    print "\\hline"
    print "  & Tarjan & \multicolumn{4}{c}{AC3Trim}  \\\\"
    print " Name & Time(ms) & Time(ms) & \#Repeat & \#Delete V & Speedup VS Tarjan \\\\"
    print "\\hline"

    for filename,name in graph_list:
        model = filename[0:-4]
        tarjan_time  = mean_confidence_interval(tarjan[model])[0]
        trim_time  = mean_confidence_interval(trim[model])[0]
        fasttrim_time   = mean_confidence_interval(fasttrim[model])[0]
        repeat  = int( trim_repeat[model])
        delete = int(trim_delete[model])

        # speedup
        trim_vs_tarjan = tarjan_time / trim_time
        fasttrim_vs_tarjan  = tarjan_time / fasttrim_time
        fasttrim_vs_trim = trim_time / fasttrim_time

        # print name
        # print repeat
        # print delete
        print "%s &     %.1f &      %.1f & %d & %d & %.1f \\\\" \
            % (name,    tarjan_time,    trim_time, repeat, delete, trim_vs_tarjan)

    print "\\hline"
    print "\\end{tabular}"
    print "\end{table*}"



def parsefile2(file, graph_list, workernum = "16"):
    tarjan  = {}
    trim    = {}
    fasttrim = {}
    trim_repeat = {}
    trim_delete = {}


    # check the maximum number of workers
    f = open(file, 'rb')
    reader = csv.DictReader(f)
    for row in reader:
        if row["alg"] == "tarjan"  and row["workers"] == "1":   addtodict(tarjan, row["model"], row["mstime"])
        if row["alg"] == "trim"   and row["workers"] == workernum:   addtodict(trim, row["model"], row["mstime"])
        if row["alg"] == "fasttrim"   and row["workers"] == workernum:  addtodict(fasttrim, row["model"], row["mstime"])
        if row["alg"] == "trim" : 
            addtodict2(trim_repeat, row["model"], row["trimrepeat"])
            addtodict2(trim_delete, row["model"], row["trimdelete"])
    f.close()
 
    #print "\\begin{table*}"
    print "\\begin{tabular}{l|rrrr}"
    print "\\hline"
    print "\\footnotesize"
    print "  & \multicolumn{4}{c}{AC6Trim} \\\\"
    print " Name & Time(ms) &\#Delete V & Speedup VS Tarjan & Speedup VS AC4Trim\\\\"
    print "\\hline"

    for filename,name in graph_list:
        model = filename[0:-4]
        tarjan_time  = mean_confidence_interval(tarjan[model])[0]
        trim_time  = mean_confidence_interval(trim[model])[0]
        fasttrim_time   = mean_confidence_interval(fasttrim[model])[0]
        repeat  = int( trim_repeat[model])
        delete = int(trim_delete[model])

        # speedup
        trim_vs_tarjan = tarjan_time / trim_time
        fasttrim_vs_tarjan  = tarjan_time / fasttrim_time
        fasttrim_vs_trim = trim_time / fasttrim_time

        # print name
        # print repeat
        # print delete
        print "%s &    %.1f & %d & %.1f & %.1f \\\\" \
            % (name, fasttrim_time, delete, fasttrim_vs_tarjan, fasttrim_vs_trim)

    print "\\hline"
    print "\\end{tabular}"
    #print "\end{table*}"

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
    if (N_ARG == 4):
        INFILE  = str(sys.argv[1])
        GFILE   = str(sys.argv[2])
        n = str(sys.argv[3])
        #part = str(sys.argv[3])

        checkfile(INFILE)
        graph_list = parsefileGraphs2(GFILE)
        if n == "1":
            parsefile1(INFILE, graph_list)
        elif n == "2":
            parsefile2(INFILE, graph_list)
        else:
            parsefile1(INFILE, graph_list)
            parsefile2(INFILE, graph_list)
    else:
        print "ERROR: invalid command"
        print "Usage:"
        print " - {} INFILE  GFILE   1/2/3           # print table".format(str(sys.argv[0]))


if __name__ == "__main__":
    main()
