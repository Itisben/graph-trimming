import sys, getopt
import random

#format the graph as the input graph for the experiments.
#path = "/media/guo/WIN/experiment-data/SCC-test-data"
path = "/media/guob15/WIN/experiment-data"
#filepath = path + "a.txt"# "out.wiki_talk_en"
#filepath2 = path + "a2.txt"#"wiki-takl-en.bin"



filepath = ""
filepath2 = ""
spliter = ''
percent = 100
cnt = 0

def start():
	f = open(filepath2, "w")
	with open(filepath) as fp:
		for line in fp:
		    #print("line {} contents {}".format(cnt, line))
		    if line[0] is '#': continue
		    if line[0] is '%': continue
		    a = line.split(spliter)
		    v0 = int(a[0])
		    v1 = int(a[1])
		    # if v0 == v1: continue
		    # if v0 > v1: v1, v0 = v0, v1
		    # if v1 > n: n = v1
		    edge = str(v0) + ' ' + str(v1) + '\n'
		    #print(edge)
		    f.write(edge)
		    cnt+=1
		    #if cnt > 1000: break
		    if cnt % 1000000 is 0: print(cnt)
	print("m:", cnt)
	f.close

def sample_v():
    random.seed()
    f = open(filepath2, "w")
    with open(filepath) as fp:
        for line in fp:
            if line[0] is '#': continue
            if line[0] is '%': continue
            if random.randint(0,99) < (100 - percent): continue
            a = line.split(spliter)
            v0 = int(a[0])
            v1 = int(a[1])
            # if v0 == v1: continue
            # if v0 > v1: v1, v0 = v0, v1
            # if v1 > n: n = v1
            edge = str(v0) + ' ' + str(v1) + '\n'
            f.write(edge)
            cnt+=1
            #if cnt > 1000: break
            if cnt % 1000000 is 0: print(cnt)
	print("m:", cnt)
	f.close

def main(argv):
    global filepath, filepath2, spliter, percent
    try:
        opts, args = getopt.getopt(argv,"hi:o:s:r")
    except getopt.GetoptError:
        print ('... -i <inputfile> -o <outputfile> -s <spliter> -r <random edge percent>')
        sys.exit(2)
    if len(args) < 3:
        print ('... -i <inputfile> -o <outputfile> -s <spliter>')
    for opt, arg in opts:
        if opt == '-h':
            print ('... -i <inputfile> -o <outputfile> -s <spliter>')
            sys.exit()
        elif opt in ("-i", "--ifile"):
            filepath = arg
        elif opt in ("-o", "--ofile"):
            filepath2 = arg
        elif opt in ("-s"):
            spliter = arg
        elif opt in ("-r", "--random"):
            percent = arg
    print ('Input file is: ' + filepath)
    print ('Output file is: '+ filepath2)
    print ('Spliter is: ' + spliter)
    print ('Percent is: ' + str(percent))
    if percent < 100 : sample_v()
    
    
if __name__ == "__main__":
    main(sys.argv[1:])
    #start()
