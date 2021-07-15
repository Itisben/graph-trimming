# this can trasfer the csv table to the latex table

# graph info
graphinfo = '''
cambridge.6 3,354,295	9,483,191	418	2.83	65	8412
bakery.6	    11,845,035	40,400,559	176	3.41	47	2636196
leader-filters.7	26,302,351	91,692,858	71	3.49	73	26302351
dbpedia	3,966,925	13,820,853	67	3.48	116	1437127
baidu	2,141,301	17,794,839	20	8.31	9	598975
livej	4,847,571	68,993,773	19	14.23	8	592952
pokec	1,632,804	30,622,564	14	18.75	5	212042
wiki-talk-en	2,987,536	24,981,163	9	8.36	7	2611598
ER	1,000,000	8,000,000		8.00		
BA	1,000,000	8,000,000		8.00		
#WS	1,000,000	8,000,000		8.00		
RMAT	1,000,000	8,000,000		8.00		
'''

def ToLatexTable(f):
    lines = f.split("\n")
    for line in lines:
        i = line.split("\t")
        newline = "\t&"
        newline = newline.join(i)
        newline += " \\\\"
        print(newline)




ToLatexTable(graphinfo)
# def parsefile(fpath, begin, end):
#     f = open(file, 'rb')
#     reader = csv.DictReader(f)
#     global g_graphs
#     for row in reader:
#         line = ""
#         for i in row[begin, end]:
#             item = 
#     f.close()

# def main():
#     arglen = len(sys.argv)
#     if (arglen == 2):
#         fpath = str(sys.argv[1])
#         begin = int(sys.argv[2])
#         end = int(sys.argv[3])
#     else:
#         print("filepath, column begin, column end")
    

# if __name__ == "__main__":
#     main()