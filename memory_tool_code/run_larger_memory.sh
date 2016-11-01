#!/bin/bash
#
##########################
# $1: Number of vaults
# $2: Number of WP entries
# $3: Associativity of memory
# $4: Number of traces
##########################

# 0 - Memcached
# 1 - RocksDB
# 2 - MySQL
# 3 - TPC-H
# 4 - TPC-DS
# 5 - Cassandra
# 6 - Neo4j

workload_path="/home/parsacom/vol/icfiler3-ext/picorel/ASPLOS2017/pin-traces/"
workload_name=( "memcached" "rocksdb" "mysql" "tpch" "tpcds" "cassandra" "neo4j" )
result_path="/net/parsafiler4/users/picorel/ISCA2017/associativity/"
memory_sizes=( 8 8 8 32 16 8 8 )
memory_tool_sizes=( 8 8 8 32 16 8 12 )
number_processes=( 1 2 4 8 16 32 64 )
storage_factor=( 8 64 )
associativity=( 1 2 4 8 16 32 64 128 )
operations=1000000000
page_size=4096
num_vaults=64
wp_entries=1024
cores=-1 # -1 all cores

echo -e "Workload Path: $workload_path"
echo -e "Result Path: $result_path"

for i in 0 1 2 3 4 6 # All except Cassandra
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

      #We should run associativities at least equal to the number of processes
      if [ "$j" -gt "$k" ]
      then
        continue
      fi
      echo -e "================="
      echo -e "Associativity: $k"
      echo -e "================="

      for l in "${storage_factor[@]}"
      do
        echo -e "=========="
        echo -e "Factor: $l"
        echo -e "=========="

        ./Fast3DCacheImpl $((1024*${memory_tool_sizes[$i]}/$l)) $i $page_size $k $cores $operations $workload_path${workload_name[$i]}"/"${memory_sizes[$i]}"GB/" $result_path $num_vaults $wp_entries ${memory_sizes[$i]} $j &
      done

    done

    wait #Wait for the processes 

  done
done

