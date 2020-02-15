This is the c++ code for draft paper "Linear Time Parallel Graph Trimming" . The experiments test existing trimming method used by Hong's algorithm (so-called AC1Trim), and our new trimming algorithm (so-called AC6Trim). 

# The Structure of the Folder

1. ```~/ours-hong-ufscc/``` : include Hong's algorithm (with AC1Trim) and our AC6Tim. 

1. ```~/ours-hong-ufscc/gm_graph/``` : include the library of CSR format storing graph.

1. ```~/ours-hong-ufscc/src/``` : include the c++ source code.

1. ```~/ours-hong-ufscc/tools/``` : include tools for converting common graph to CSR format.

1. ```~/ours-hong-ufscc/tools``` : include the test results on my laptop.

1. ```~/experiments/``` : inlude all the testing scripts.

1. ```~/experiments/graphs``` : inlude implicit BEEM model checking graphs. The explicit graphs are so large that can not uploaded. But they are easy to download from correspounding websites.

# Compiling and Executing

1. Compile the gm_graph runtime library and the scc binary by running make in the directory ```~/ours-hong-ufscc/```
1. To execute application, in the directory ```~/ours-hong-ufscc/```, run:

```./scc <graph_name> <num_threads> <method> {-d|-a|-p} ```

Calling ./scc with no input parameters will print the help, which describes
all of the options. 

* method 4: the existing AC1Trim which is from Hong's algorithm (Algorithm 2 in paper with only removing vertices without outgoing edges). 
It is locaed in the file ```~/ours-hong-ufscc/src/scc_trim1.cc```.

* method 6: our sequential AC6Trim (Algorithm 3 in paper). 
It is locaed in the file ```~/ours-hong-ufscc/src/scc_our_fast_trim.cc```.

* method 9: our parallel AC6Trim (Algorithm 5 in paper). 
It is locaed in the file ```~/ours-hong-ufscc/src/scc_our_fast_trim_pra-4.cc```.


# Executing with Script

1. All the experiments can run with a script ```~/experiments/our-benchmark.sh```. Before running, all the path of tested graphs should be set. 

1. The experiments result is recored in file ```~/experiments/results/our-results.csv```. 

1. My testing results are locaed in file ```~/experiments/results/our-results-11-1-2019.csv```. 


# Graph Conversion Tool
There is a graph conversion utility to convert adjacency list or edge list graph files into the binary format used by current application. 

* To execute this tool, in the directory ```~/ours-hong-ufscc/tools/```, run:

```./convert <InputName> <OutputName> [ -?=<0/1> -GMInputFormat=<string> -GMMeasureTime=<0/1> -GMNumThreads=<int> -GMOutputFormat=<string> ]```

Calling ./convert with no input parameters will print the help, which describes
all of the options. 


