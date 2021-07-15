#format the graph as the input graph for the experiments.
path = "/home/guo/Documents/SCC-test-data/"
#filepath = path + "a.txt"# "out.wiki_talk_en"
#filepath2 = path + "a2.txt"#"wiki-takl-en.bin"

filepath = path + "livej"
filepath2 = path + "patent2.txt"


edgeset = set()
n = 0
with open(filepath) as fp:
    cnt = 0
    for line in fp:
        if line[0] is '#': continue
        a = line.split('\t')
        v0 = int(a[0])
        v1 = int(a[1])
        if v0 is v1: print("loop %d\n", v0)
        if v1 > n: n = v1
        edge = str(v0) + ' ' + str(v1) + '\n'
        #print(edge)
        edgeset.add(edge)
        cnt+=1
        #if cnt % 100000 is 0: print(cnt)

n += 1
m = len(edgeset)

print("n, m", n, m);
