#!/bin/bash
#this is test the parallel step of OpenMP, defalut is 8192
# binaries
OFFLINE_SCC="/home/guob15/Documents/git-code/scc-trim-v3/code/trim"
#OFFLINE_SCC="/mnt/d/git-code/scc-trim-v3/ours-hong-ufscc/scc"

# variables
TIMEOUT_TIME="1200s" # 10 minutes timeout
#2^      0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20#
PAR_STEP="1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576" 
#PAR_STEP="8192 16384" 

# misc fields
BENCHDIR=`pwd`
TMPFILE="${BENCHDIR}/test.out" # Create a temporary file to store the output
FAILFOLDER="${BENCHDIR}/failures"

# input graphs folders
OUR_TEST_GRAPH="/media/guob15/WIN/experiment-data/SCC-test-data/TestTrim/ParStep"
#OUR_TEST_GRAPH="/mnt/d/ppopp16-offline/real-offline"

# results
OUR_RESULT="${BENCHDIR}/results/our-results-parstep-6-4-2021.csv"

#result column
COLUMN="model,N,M,alg,workers,mstime,delete,trimrepeat,trimdelete,traveledge,sccs,maxscc,size1,size2,size3,parstep,vs,es,date"

#repeat times
REPEAT_TIME="50"

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
        algnumber=9
    elif [ "${alg}" = "tarjan" ]
    then
        algnumber=3
    elif [ "${alg}" = "ufscc" ]
    then
        algnumber=5
    elif [ "${alg}" = "trim" ]
    then
        algnumber=4
    elif [ "${alg}" = "hong" ]
    then
        algnumber=2
    elif [ "${alg}" = "ac4trim" ]
    then
        algnumber=10
    fi 

    step=${3}
    
    echo "Running ${alg} (${algnumber}) on ${OUR_TEST_GRAPH}/${name} with 16 worker(s) with step ${step}"
    echo ${TIMEOUT_TIME} ${OFFLINE_SCC} ${OUR_TEST_GRAPH}/${name} 16 ${algnumber} "-s" ${step}
    timeout ${TIMEOUT_TIME} ${OFFLINE_SCC} ${OUR_TEST_GRAPH}/${name} 16 ${algnumber} "-s" ${step} &> ${TMPFILE}

    base=${name%.bin} # without the .bin#
    python our-parse-output.py "${base}" "${alg}" "${workers}" "${FAILFOLDER}" "${TMPFILE}" "${OUR_RESULT}" 
}


# test_all_real_offline ALG WORKERS
# e.g. test_all_real_offline ufscc 64
test_all_real_offline() {
    alg=${1}
    step=${2}
    if [ ! "$(ls -A ${OUR_TEST_GRAPH})" ]; then
        echo "graphs/real-offline folder is empty"
    else
        for file in ${OUR_TEST_GRAPH}/*.bin
        do
            name=${file##*/} # remove preceding path
            test_real_offline ${name} ${alg} ${step}
        done
    fi
}

do_tests() {
    for step in `echo ${PAR_STEP}`
    do
        test_all_real_offline fasttrim "${step}"
        test_all_real_offline trim "${step}"
        test_all_real_offline ac4trim "${step}"
    done
}


# do_all_experiments ITERATIONS
do_all_experiments() {
    iterations=${1}
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
#rm "${TMPFILE}"
