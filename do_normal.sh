wmax=1000
level=5
echo "Enter max_level"
level_max=$1

echo "begin simulation: wmax=$wmax, level=$level -> $level_max, groups=$group"
for l in $(seq $level 1 $level_max); do
        ./normal_path_oram_lifetime_simulation $wmax $l | grep pgs
done
