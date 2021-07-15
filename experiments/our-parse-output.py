import sys
import re
import os.path
import os.stat
import time
import datetime
import csv
import shutil


COLUMN="model,N,M,workers,alg,init-time,insert-num,insert-time,remove-num,remove-time,cnt-Vs,cnt-Vp,cnt-S,cnt-tag,date"


INFILE      = ""
FAILFOLDER  = ""
OUTFILE     = ""
CORRECTFILE = ""
dict        = {}


def exitparser():
    # try to print the contents to an output file
    global dict, INFILE, FAILFOLDER
    counter = 0
    while True:
        counter += 1
        failname = "{}/{}_{}_{}_{}.out".format(FAILFOLDER,dict["model"],dict["alg"],dict["workers"],counter)
        if not (os.path.isfile(failname)):
            break
    shutil.copy2(INFILE, failname)
    # and exit the program
    #sys.exit()
    

def checkfile(file):
    if not (os.path.isfile(file)):
        print "ERROR: cannot find file: {}".format(file)
        exitparser()


def parsevar(varname, line, regex):
    pattern = re.compile(regex)
    searchpattern = pattern.search(line)
    if (searchpattern):
        global dict
        if (dict.get(varname)):
            print "ERROR: multiple matches for {}".format(varname)
            exitparser()
        else:
            dict[varname] = searchpattern.group(1)


def parseerror(line, regex):
    pattern = re.compile(regex)
    searchpattern = pattern.search(line)
    if (searchpattern):
        print "ERROR: {}".format(line)
        exitparser()

 
def parseline(line):
    parseerror(line, r"Error")

    
    parsevar("N",       line, r"# of vertices: ([\S]+)") 
    parsevar("M",       line, r"# of (all) edges: ([\S]+)") 
    parsevar("M",       line, r"# of (all) edges: ([\S]+)") ]




    

    parsevar("insert-num",       line, r"# of edges to insert: ([\S]+)")
    parsevar("insert-num",       line, r"# of edges to insert: ([\S]+)")

    parsevar("delete",       line, r"Total Delete Vertex #: ([\S]+)")
    parsevar("N",            line, r"N = ([\S]+), M = [\S]+")
    parsevar("M",            line, r"N = [\S]+, M = ([\S]+)")
    parsevar("sccs",         line, r"Total # SCCs = ([\S]+)")
    parsevar("size1",        line, r"Total # Size-1 SCCs = ([\S]+)")
    parsevar("size2",         line, r"Total # Size-2 SCCs = ([\S]+)")
    parsevar("size3",         line, r"Total # Size-3 SCCs = ([\S]+)")
    parsevar("trimrepeat",    line, r"trim-repeat-time: ([\S]+)")
   
    parsevar("trimdelete",    line, r"Total Delete Vertex #: ([\S]+)")
    parsevar("traveledge",  line,  r"total-traversed-e #: ([\S]+)")
    parsevar("maxscc",        line, r"Max SCC size = ([\S]+)")
    parsevar("parstep",     line, r"my PARALLEL_STEP: ([\S]+)")
    parsevar("push",     line, r"Max stack push #: ([\S]+)")
    parsevar("es",     line, r"my SAMPLE_EDGE: ([\S]+)")
    parsevar("vs",     line, r"my SAMPLE_VER: ([\S]+)")


def afterparse():
    global dict
    # Offline output shows time in ms, we use seconds
    if (dict.get("mstime")):
        sectime = float(dict["mstime"])
        sectime /= 1000
        dict["time"] = str(sectime)
    if not (dict.get("initstates")): dict["initstates"] = "1"
    if (dict["alg"] != "ufscc"):
        if not (dict.get("ustates")):
            if not (dict.get("N")):
                print "ERROR: Cannot find N"
                exitparser()
            dict["ustates"] = dict["N"]
        if not (dict.get("utrans")):
            if not (dict.get("M")):
                print "ERROR: Cannot find M"
                exitparser()
            dict["utrans"] = dict["M"]


def checkitemcorrect(correct, item):
    global dict
    if not (dict.get(item)):
        print "ERROR: Cannot find {}".format(item)
        exitparser()
    if (dict[item] != correct):
        print "ERROR: {} = {} is incorrect (should be {}) ".format(item, dict[item], correct)
        exitparser()

def parsefile(file):
    f = open(file, 'r')
    for line in f:
        parseline(line)
    f.close()
    afterparse()



def trytoprint(varname):
    global dict
    if (dict.get(varname)):
        return dict.get(varname)
    else:
        return "-1";

def printtofile(outfile):
    # First line of OUTFILE should contain comma-separated info on column names
    global dict
    if os.path.exists(outfile):
        f = open(outfile, 'r+')
        f.write(COLUMN+"\n") #write the column
    else:
        f = open(outfile, 'r+')

    s = f.readline().strip()
    names = s.split(",")
    output  = ""
    for name in names:
        output += trytoprint(name) + ","
    output = output[:-1] # remove last ","
    f.read() # go to the last line
    f.write(output+"\n") # write the new line
    f.close()


def printtostdout():
    # find longest key name (for formatting)
    global dict
    maxlen = 1
    for varname in dict:
        maxlen = max(maxlen, len(varname))
    # print everything to stdout
    for varname in dict:
        print varname + ":" +  " " * (maxlen+1-len(varname)) + dict[varname]


def addextra():
    # Add timestamp
    global dict
    ts = time.time()
    dict["date"] = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d.%H:%M:%S')


def main():
    global dict, INFILE, OUTFILE, FAILFOLDER, CORRECTFILE
    N_ARG = len(sys.argv)
    if (N_ARG == 6):
        dict["model"]   = str(sys.argv[1])
        dict["alg"]     = str(sys.argv[2])
        dict["workers"] = str(sys.argv[3])
        FAILFOLDER      = str(sys.argv[4])
        INFILE          = str(sys.argv[5])
        checkfile(INFILE)
        parsefile(INFILE)
        addextra()
        printtostdout()
    elif (N_ARG == 7):
        dict["model"]   = str(sys.argv[1])
        dict["alg"]     = str(sys.argv[2])
        dict["workers"] = str(sys.argv[3])
        FAILFOLDER      = str(sys.argv[4])
        INFILE          = str(sys.argv[5])
        OUTFILE         = str(sys.argv[6])
        checkfile(INFILE)
        checkfile(OUTFILE)
        parsefile(INFILE)
        addextra()
        printtofile(OUTFILE)
    else:
        print "ERROR: invalid command"
        print "Usage:"
        print " - python parse_output.py  MODEL  ALG  WORKERS  FAILFOLDER  INFILE           # writes all output to the stdout"
        print " - python parse_output.py  MODEL  ALG  WORKERS  FAILFOLDER  INFILE  OUTFILE  # appends the data to the OUTFILE in the same format used"


if __name__ == "__main__":
    main()

