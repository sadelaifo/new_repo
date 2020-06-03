#! /bin/bash
echo $(date)
wmax=10000
level=5
groups=2
threads=5
echo "Enter max_level, max_group"
group_max=$2
level_max=$1

# Fix All but Wmax

echo "begin simulation: wmax=1000, level=$level -> $level_max, groups=1, threads=$threads"
for g in $(seq $level 1 $level_max); do
#	./start_gap_simulation 1000 $g 1 $threads | grep pgs
	./start_gap_simulation_sanitize 1000 $g 1 $threads | grep percent
done

exit

echo "begin simulation: wmax=1 -> 16384, level=$level_max, groups=$group_max, threads=$threads"
for g in 1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072; do
#	./start_gap_simulation $wmax $level_max $g $threads | grep percent
	./start_gap_simulation $g $level_max $group_max $threads | grep pgs
done

# Fix ALL but # of groups
echo "begin simulation: wmax=$wmax, level=$level_max, groups=1->$group_max"

for g in 1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384; do
	./start_gap_simulation $wmax $level_max $g $threads | grep pgs
done

echo $(date)
