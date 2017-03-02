#!/bin/bash
#
##########################
# $1: Workload number
# $2: Scrambling option 
##########################

# 0 - Memcached
# 1 - RocksDB
# 2 - MySQL
# 3 - TPC-H
# 4 - TPC-DS
# 5 - Cassandra
# 6 - Neo4j


workload_path="/mnt/raid10/traces/"
workload_name=( "memcached" "rocksdb" "mysql" "tpch" "tpcds" "cassandra" "neo4j" "test" 
                "memcached-filtered" "rocksdb-filtered" "mysql-filtered" "tpch-filtered" "tpcds-filtered" "cassandra-filtered" "neo4j-filtered" "test-filterd"
                "memcached-mixed" "rocksdb-mixed" "mysql-mixed" "tpch-mixed" "tpcds-mixed" "cassandra-mixed" "neo4j-mixed"
              )
result_path="/root/results/pt_disk/"
memory_sizes=( 8 8 8 8 8 8 8 8 
               8 8 8 8 8 8 8 8
               8 8 8 8 8 8 8
             )
memory_tool_sizes=( 4 4 4 4 4 4 12 4
                    4 4 4 4 4 4 12 4
                    4 4 4 4 4 4 12
                  )
number_processes=( 1 2 4 8 )
factor=( 1 2 4 8 16 32 )
#operations=-1
operations=10000000000
#operations=1000
page_size=4096
cores=-1 # -1 all cores

if [ -z "$1" ]
  then
    echo "No workload supplied, exiting..."
    exit
fi

if [ -z "$2" ]
  then
    echo "No scrambling option supplied, exiting..."
    exit
fi

scramble=$2
workload=$1

#echo -e "Workload Path: $workload_path"
#echo -e "Result Path: $result_path"

#for i in 0 1 2 3 4 6 # All except Cassandra
for i in $workload 
do
  echo -e "====================================="
  echo -e "Workload: ${workload_name[$i]}"
  echo -e "====================================="

  for j in "${number_processes[@]}"
  do
    echo -e "========================"
    echo -e "Number of processes: $j"
    echo -e "========================"

    for k in "${factor[@]}"
    do

      echo -e "================="
      echo -e "Factor: $k"
      echo -e "================="
 
      #echo "Input: $((1024*${memory_tool_sizes[$i]})) $i $page_size $k $cores $operations $workload_path${workload_name[$i]}"/"${memory_sizes[$i]}"GB/" $result_path ${memory_sizes[$i]} $j $2\n"
      if [ $workload -gt 15 ] #Mix
      then
        if [ $j -eq 4 ] 
        then
          ./Fast3DCacheImpl $((1024*${memory_tool_sizes[$i]})) $i $page_size $cores $operations $workload_path${workload_name[$i]}"/" $result_path 16 0 $((${memory_sizes[$i]}/$j)) $j $scramble $k > $result_path${workload_name[$i]}"_"$j"x_"$((${memory_sizes[$i]}/$j))"GB_Trace_"${memory_tool_sizes[$i]}"GB_Memory_"$k"factor_"$scramble"_scramble" &
        fi
      else #Not mix
        ./Fast3DCacheImpl $((1024*${memory_tool_sizes[$i]})) $i $page_size $cores $operations $workload_path${workload_name[$i]}"/" $result_path 16 0 $((${memory_sizes[$i]}/$j)) $j $scramble $k > $result_path${workload_name[$i]}"_"$j"x_"$((${memory_sizes[$i]}/$j))"GB_Trace_"${memory_tool_sizes[$i]}"GB_Memory_"$k"factor_"$scramble"_scramble" &
      fi
    done

    wait #Wait for the processes 

  done
done
