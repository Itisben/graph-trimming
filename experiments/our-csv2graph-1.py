
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

#generate the table from the csv format.
def checkfile(file):
    if not (os.path.isfile(file)):
        print "ERROR: cannot find file: {}".format(file)
        sys.exit()

def getinfo(file, result_file):
    f = open(file, 'rw')
    rf = open(result_file, 'r')
    freader = csv.DictReader(f)
    rfreader = csv.DictReader(rf)
    fileinfo = []
    for frow in freader:
        print "write {}".format(frow['filename'])
        rf.seek(0)
        for rfrow in rfreader:
            #print frow['filename'] + ":" + rfrow['model']+".bin"
            if frow['filename'].strip() == rfrow['model'].strip() +".bin":
                #print "OKKK"
                if rfrow['alg'].strip() == 'tarjan'.strip():
                    print "write OK"
                    frow['# Nodes'] = rfrow['N']
                    frow['# Edges'] = rfrow['M']
                    frow['# SCC'] = rfrow['sccs']
                    #frow['Max SCC Size'] = rfrow['maxscc'] #not yet ready.
                    frow['# Size-1 SCC'] = rfrow['size1']
                    frow['# Size-2 SCC'] = rfrow['size2']
                    frow['# Size-3 SCC'] = rfrow['size3']
                    frow['Max SCC Size'] = rfrow['maxscc']
                    fileinfo.append(frow)   
                    break


    rf.close()
    f.close()
    f = open(file+"2.csv", "w+")
    fieldnames = ['Name','# Nodes','# Edges'	,'Diameter'	,'# SCC',	'Max SCC Size',	'# Size-1 SCC',	'# Size-2 SCC',	'# Size-3 SCC']
    fwriter = csv.DictWriter(f, fieldnames = fieldnames)
    fwriter.writeheader()
    for item in fileinfo:
        temp = []
        for i in item:
            if i not in fieldnames: temp.append(i)
        for t in temp:
            item.pop(t)
        fwriter.writerow(item)
    f.close
def parsefile(file):
    f = open(file, 'r')
    #reader = csv.DictReader(f)
    reader = csv.reader(f)
    
    
    print "\documentclass{standalone}"

    print "\\begin{document}"
    print ""
    print ""

    print "\\begin{table}[!h]"
    print "\\begin{tabular}{l|rrrrrrrr}"
    print "\\hline"
    
    i = 0
    for row in reader:
        if row[0] == "*" or row[0] == "": break
        row = row[:8]
        outstr = " & ".join(row)
        outstr = outstr.replace("#", "\\#")
        print outstr + "\\\\"
        if i is 0: print "\\hline"
        i = i +1
    print "\\hline"
            
    
    print "\\end{tabular}"
    print "\\end{table}"
    print ""
    print ""
    print "\\end{document}"
    f.close()


def main():
    global ALG1, ALG2, WORKERS1, WORKERS2
    N_ARG = len(sys.argv)
    if (N_ARG == 2):
        INFILE   = str(sys.argv[1])
        checkfile(INFILE)
        parsefile(INFILE)
    elif(N_ARG == 4):
        INFILE   = str(sys.argv[2])
        if "-g" != str(sys.argv[1]): 
            print "ERROR: invalid command"
            return
        RESULT_FILE = str(sys.argv[3])
        checkfile(INFILE)
        checkfile(RESULT_FILE)
        getinfo(INFILE, RESULT_FILE)
    else:
        print "ERROR: invalid command"
        print "Usage:"
        print " - {} INFILE                 # print table".format(str(sys.argv[0]))
        print " - {} -g INFILE RESULT-FILE  # get table information".format(str(sys.argv[0]))


if __name__ == "__main__":
    main()
