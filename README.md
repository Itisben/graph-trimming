This is the c++ code for the draft paper "Efficient Parallel Graph Trimming by Arc-Consistency". The experiments test three trimming algorithms,includind AC3Trim, AC4Trim, and AC6Trim.

# The Structure of the Folder

1. ```~/code/gm_graph/``` : include the library of CSR format for storing and traversing graph.

1. ```~/code/src/``` : include the c++ source code for AC3Trim, AC4Trim, and AC6Trim.

1. ```~/code/tools/``` : include tools for converting common graph to CSR format.

1. ```~/experiments/```: include all the experiment scripts.

1. ```~/experiments/graphs/```: include implicit BEEM model checking graphs. The explicit graphs are so large that can not be uploaded. But they are easy to download from corresponding websites.

1. ```~/experiments/results/```: include all the test results.

1. ```~/experiments/our-convert/```: include all python script that generate the experiment result to csv file which is used in the paper writing in latex.

# Compiling and Executing

1. The g++ and OpenMP is used for compliling.

1. Compile the gm_graph runtime library and the scc binary by running make in the directory ```~/code/```

1. To execute application, in the directory ```~/code/```, run:

```./trim <graph_name> <num_threads> <method> {-s 4096|-es 100|-vs 100}```


Calling ./trim with no input parameters will print the help, which describes all of the options:

```method  *9: Our Concurrent AC-6 Trim```

```method  *91: Our Sequential AC-6 Trim```

```method *10: our Concurrent AC-4 Trim```

```method *11: our Concurrent AC-3 Trim```

```-s 4096: dynamic parallel step (trunk size), defalut 4096```

```-se 100: sample the edges by percent, default 100%```

```-sv 100: sample the vertices by percent, default 100%```

For example, ```./trim graphname 16 9``` will run with AC6Trim with 16 workers with default trunk size 4096 without sampling.

# Executing with Script

1. By varing the trunk size, the experiment can run with a scritp ```~/experiments/our-benchmark-parstep.sh```. Before running, all the paths of tested graphs should be set. 

1. By varing the workers from 1 to 32, the experiments can run with a script ```~/experiments/our-benchmark.sh```. 

1. By sampling the edges and vertices, the experiments can run with a script ```~/experiments/our-benchmark-sample.sh```.



# Graph Conversion Tool
There is a graph conversion utility to convert the adjacency list or edge list graph files into the binary format used by the current application. 

* Before conversion, you can use the python script ```~/experiment/our-convert/convert-g.py``` to remove the useless headlines of the input file.

* To execute this tool, in the directory ```~/code/tools/```, run:

```./convert <InputName> <OutputName> [ -?=<0/1> -GMInputFormat=<string> -GMMeasureTime=<0/1> -GMNumThreads=<int> -GMOutputFormat=<string> ]```

Calling ./convert with no input parameters will print the help, which describes all of the options. 



# Concurrent Programming With OpenMP

* the "#pragma omp ..." may not have compile errors if you write not correct. For example, the "#pragma critical" is wrong since it loses "omp" inside, but the c++ compiler doesn't give any warning or errors. 

* For efficiency, we can use "CAS" primitive to implement "lock" and "unlock". Note that all the shared variables by multi-worker should use atomic read and write. 
 