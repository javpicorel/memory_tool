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


workload_path="/var/tmp/pin-traces/"
workload_name=( "memcached" "rocksdb" "mysql" "tpch" "tpcds" "cassandra" "neo4j" )
result_path="/home/parsacom/vol/icfiler3-ext/picorel/ASPLOS2017/results/"
memory_sizes=( 8 8 8 32 16 8 8 )
memory_tool_sizes=( 1 1 1 4 2 1 1 )
number_processes=( 1 2 4 8 16 32 64 )
associativity=( 1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 )
#operations=-1
#operations=10000000000
operations=5000000000
page_size=4096
cores=-1 # -1 all cores

#echo -e "Workload Path: $workload_path"
#echo -e "Result Path: $result_path"

#for i in 0 1 2 3 4 6 # All except Cassandra
for i in $1 
do
  echo -e "====================================="
  echo -e "Workload: ${workload_name[$i]}"
  echo -e "====================================="

  for j in "${number_processes[@]}"
  do
    echo -e "========================"
    echo -e "Number of processes: $j"
    echo -e "========================"

    for k in "${associativity[@]}"
    do

      #We should run associativities at least equal to the number of processes if scrambling is not enabled '0'
      if [ "$2" -eq 0 ] && [ "$j" -gt "$k" ]
      then
        continue
      fi
      echo -e "================="
      echo -e "Associativity: $k"
      echo -e "================="
 
      #echo "Input: $((1024*${memory_tool_sizes[$i]})) $i $page_size $k $cores $operations $workload_path${workload_name[$i]}"/"${memory_sizes[$i]}"GB/" $result_path ${memory_sizes[$i]} $j $2\n"
      ./Fast3DCacheImpl $((1024*${memory_tool_sizes[$i]})) $i $page_size $k $cores $operations $workload_path${workload_name[$i]}"/"${memory_sizes[$i]}"GB/" $result_path ${memory_sizes[$i]} $j $2 > $result_path${workload_name[$i]}"_"$j"x_"${memory_sizes[$i]}"MB_"${memory_tool_sizes[$i]}"GB_"$k"ways_"$2 &

    done

    wait #Wait for the processes 

  done
done
