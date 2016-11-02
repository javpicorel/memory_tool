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

workload_path="/mnt/raid10/pin-traces/"
workload_name=( "memcached" "rocksdb" "mysql" "tpch" "tpcds" "cassandra" "neo4j" )
result_path="/mnt/raid10/results/pagetables/"
memory_sizes=( 512 512 512 32 16 512 512 )
memory_tool_sizes=( 8 8 8 32 16 8 12 )
number_processes=( 1 2 4 8 16 32 64 )
factors=( 2 4 8 )
#operations=10000000000
operations=1000
page_size=4096
num_vaults=(16 16 16 64 32 16 32)
cores=-1 # -1 all cores
vault_id=0

echo -e "Workload Path: $workload_path"
echo -e "Result Path: $result_path"

#for i in 0 1 2 3 4 # All except Cassandra and Neo4j
for i in 0 # All except Cassandra and Neo4j
do
  echo -e "====================================="
  echo -e "Workload: ${workload_name[$i]}"
  echo -e "====================================="

  for j in "${number_processes[@]}"
  do
    echo -e "========================"
    echo -e "Number of processes: $j"
    echo -e "========================"

    for k in "${factors[@]}"
    do

      #We should run associativities at least equal to the number of processes
      echo -e "================="
      echo -e "Factor: $k"
      echo -e "================="

      ./Fast3DCacheImpl ${memory_sizes[$i]} $i $page_size $cores $operations $workload_path${workload_name[$i]}"/"${memory_tool_sizes[$i]}"GB/" $result_path ${num_vaults[$i]} $vault_id ${memory_tool_sizes[$i]} $j $k > $result_path${workload_name[$i]}"_"$j"x_"${memory_sizes[$i]}"MB_"${memory_tool_sizes[$i]}"GB_"$k"f_"${num_vaults[i]}"v"
    done

    wait #Wait for the processes 

  done
done

#./Fast3DCacheImpl 8192 0 4096 $3 -1 100000000 /home/parsacom/vol/icfiler3-ext/picorel/ASPLOS2017/pin-traces/memcached/8GB/ /tmp/ $1 $2 8 $4
#cat /tmp/memcached_region_0_4096B_page_8192MB_Memory_$3ways
