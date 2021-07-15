#!/bin/bash

# binaries
#OFFLINE_SCC="/home/guo/Documents/git-code/scc-trim-v3/ours-hong-ufscc/scc"
OFFLINE_SCC="/home/guob15/Documents/git-code/scc-trim-v3/ours-hong-ufscc/scc"

# variables
#TIMEOUT_TIME="1200s" # 10 minutes timeout
TIMEOUT_TIME="24000s" # 200 minutes timeout
WORKER_LIST="1 2 4 6 8 10 12 14 16 32"
#WORKER_LIST="16"

# misc fields
BENCHDIR=`pwd`
TMPFILE="${BENCHDIR}/test.out" # Create a temporary file to store the output
FAILFOLDER="${BENCHDIR}/failures"

# input graphs folders
OUR_TEST_GRAPH="/media/guob15/WIN/experiment-data/SCC-test-data/TestTrim/large"
#OUR_TEST_GRAPH="/home/guob15/Documents/TestTrim"

# results
OUR_RESULT="${BENCHDIR}/results/our-results-large-6-18-2021.csv"

#result column
COLUMN="model,N,M,alg,workers,mstime,delete,trimrepeat,trimdelete,traveledge,sccs,maxscc,size1,size2,size3,parstep,push,vs,es,date"

#repeat times
REPEAT_TIME="5"

trap "exit" INT #ensures that we can exit this bash program

# create new output file, or append to the exisiting one
create_results() {
    output_file=${1}
    if [ ! -e ${output_file} ]
    then 
        touch ${output_file}
        #add column in output csv.
        echo "${COLUMN}" > ${output_file}
    fi
}

# create necessary folders and files
init() {
    if [ ! -d "${FAILFOLDER}" ]; then
      mkdir "${FAILFOLDER}"
    fi
    if [ ! -d "${BENCHDIR}/results" ]; then
      mkdir "${BENCHDIR}/results"
    fi
    touch ${TMPFILE}
    create_results ${OUR_RESULT}
}

# test_real_offline MODEL ALG WORKERS
# e.g. test_real_offline livej.bin ufscc 64
test_real_offline() {
    name=${1}
    alg=${2}
    if [ "${alg}" = "fasttrim" ]
    then
        algnumber=9 #our ac6trim
    elif [ "${alg}" = "tarjan" ]
    then
        algnumber=3
    elif [ "${alg}" = "ufscc" ]
    then
        algnumber=5
    elif [ "${alg}" = "trim" ]
    then
        algnumber=11 #ac3trim
    elif [ "${alg}" = "hong" ]
    then
        algnumber=2
    elif [ "${alg}" = "ac4trim" ]
    then
        algnumber=10
    fi 

    workers=${3}
    
    echo "Running ${alg} (${algnumber}) on ${OUR_TEST_GRAPH}/${name} with ${workers} worker(s)"
    timeout ${TIMEOUT_TIME} ${OFFLINE_SCC} ${OUR_TEST_GRAPH}/${name} ${workers} ${algnumber} -s 4096 &> ${TMPFILE}
    echo ${TIMEOUT_TIME} ${OFFLINE_SCC} ${OUR_TEST_GRAPH}/${name} ${workers} ${algnumber}

    base=${name%.bin} # without the .bin#
    python our-parse-output.py "${base}" "${alg}" "${workers}" "${FAILFOLDER}" "${TMPFILE}" "${OUR_RESULT}" 
}


# test_all_real_offline ALG WORKERS
# e.g. test_all_real_offline ufscc 64
test_all_real_offline() {
    alg=${1}
    workers=${2}
    if [ ! "$(ls -A ${OUR_TEST_GRAPH})" ]; then
        echo "graphs/real-offline folder is empty"
    else
        for file in ${OUR_TEST_GRAPH}/*.bin
        do
            name=${file##*/} # remove preceding path
            test_real_offline ${name} ${alg} ${workers}
        done
    fi
}

do_tests() {
    
    for workers in `echo ${WORKER_LIST}`
    do
        test_all_real_offline fasttrim "${workers}"
        test_all_real_offline trim "${workers}"
        test_all_real_offline ac4trim "${workers}"
    done
}


# do_all_experiments ITERATIONS
do_all_experiments() {
    iterations=${1}
    #test_all_real_offline tarjan 1
    for experiments in `seq 1 ${iterations}`
    do
        echo ""
        echo "iteraton ${experiments}"
        do_tests
    done
}

############################################################

# initialize
init

# performs all experiments N times
do_all_experiments ${REPEAT_TIME}

# cleanup
rm "${TMPFILE}"
