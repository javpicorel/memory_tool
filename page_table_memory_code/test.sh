##########################
# $1: Number of vaults
# $2: Number of WP entries
# $3: Associativity of memory
# $4: Number of traces
##########################
./Fast3DCacheImpl 8192 0 4096 $3 -1 100000000 /home/parsacom/vol/icfiler3-ext/picorel/ASPLOS2017/pin-traces/memcached/8GB/ /tmp/ $1 $2 8 $4
cat /tmp/memcached_region_0_4096B_page_8192MB_Memory_$3ways
