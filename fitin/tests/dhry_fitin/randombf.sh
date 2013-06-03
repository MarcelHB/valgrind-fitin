#!/bin/bash

VALGRIND_DIR="/home/marcel/Source/valgrind-fitin"
VALGRIND_BIN="bin/valgrind"
ORIG_PROG="mydhrystone"
FS_PROGS="mydhrystone-BCBOOL mydhrystone-CSBOOL mydhrystone-EPBOOL mydhrystone-RMBOOL mydhrystone-SHBOOL"

INJECTION_RUNS=100
DHRYTEST_RUNS=50

RES_DIR="results"
RND_SEED=1234


START_TIME=$(date +%Y%m%d%H%M%S.%N)
RANDOM=$RND_SEED
MAX_RND=32767

# Get a random number from 0 to $1 (max 32767) which is equally distributed and stores it to $RAND
function get_rand () {
	RAND=$RANDOM
	while [ $RAND -gt $(( $1 * $(($MAX_RND / $1)) )) ] # $RAND > $1 * floor(MAX / $1)
	do
		RAND=$RANDOM
	done
	
	RAND=$(($RAND % $1))
}


if [ ! -d "$RES_DIR" ]; then
	mkdir "$RES_DIR"
fi

READS_STR="\[FITIn\] Monitored variable accesses: "

$VALGRIND_DIR/$VALGRIND_BIN --tool=fitin --golden-run=yes --log-file="$RES_DIR/$ORIG_PROG-gr.log" --include="$PWD" "$PWD/$ORIG_PROG" $DHRYTEST_RUNS > "$RES_DIR/$ORIG_PROG-gr.out"
ORIG_READS=$(grep "$READS_STR" $RES_DIR/$ORIG_PROG-gr.log | sed "s/$READS_STR\([:digit:]*\)/\1/g")
#echo $ORIG_READS
for FS_PROG in $FS_PROGS
do
	$VALGRIND_DIR/$VALGRIND_BIN --tool=fitin --golden-run=yes --log-file="$RES_DIR/$FS_PROG-gr.log" --include="$PWD" "$PWD/$FS_PROG" $DHRYTEST_RUNS > "$RES_DIR/$FS_PROG-gr.out"
done
#echo $FS_READS



for j in $(seq 0 1 $(($INJECTION_RUNS - 1)))
do
	jprint=$(printf %04d $j)
	get_rand $ORIG_READS
	ORIG_TIME=$RAND
	ORIG_BIT=$(($RANDOM % 32)) 
	$VALGRIND_DIR/$VALGRIND_BIN --tool=fitin --mod-bit=$ORIG_BIT --mod-load-time=$ORIG_TIME --log-file="$RES_DIR/$ORIG_PROG-$jprint.log" --include="$PWD" "$PWD/$ORIG_PROG" $DHRYTEST_RUNS > "$RES_DIR/$ORIG_PROG-$jprint.out"
	
	for FS_PROG in $FS_PROGS
	do
		FS_READS=$(grep "$READS_STR" $RES_DIR/$FS_PROG-gr.log | sed "s/$READS_STR\([:digit:]*\)/\1/g")
		READS_FACTOR=$((FS_READS / $ORIG_READS))

		FS_TIME=$(($ORIG_TIME * READS_FACTOR)) 
		FS_BIT=$ORIG_BIT 
		echo "ORIG_TIME: " $ORIG_TIME "; FS_TIME: " $FS_TIME "; ORIG_BIT: "	$ORIG_BIT 
		$VALGRIND_DIR/$VALGRIND_BIN --tool=fitin --mod-bit=$FS_BIT --mod-load-time=$FS_TIME --log-file="$RES_DIR/$FS_PROG-$jprint.log" --include="$PWD" "$PWD/$FS_PROG" $DHRYTEST_RUNS > "$RES_DIR/$FS_PROG-$jprint.out"
	done
done

END_TIME=$(date +%Y%m%d%H%M%S.%N)

printf "Time elapsed:    %.3F\n"  $(echo "$END_TIME - $START_TIME"|bc )
