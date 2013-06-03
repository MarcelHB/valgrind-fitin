#!/bin/bash

VALGRIND_DIR="../../../"
VALGRIND_BIN="bin/valgrind"
ORIG_PROG="mydhrystone"
FS_PROGS="mydhrystone-BCBOOL mydhrystone-CSBOOL mydhrystone-EPBOOL mydhrystone-RMBOOL mydhrystone-SHBOOL"

INJECTION_RUNS=50
DHRYTEST_RUNS=50

RES_DIR="results"

START_TIME=$(date +%Y%m%d%H%M%S.%N)

if [ ! -d "$RES_DIR" ]; then
	mkdir "$RES_DIR"
fi

READS_STR="\[FITIn\] Monitored variable accesses: "

$VALGRIND_DIR/$VALGRIND_BIN --tool=fitin --golden-run=yes --log-file="$RES_DIR/$ORIG_PROG-gr.log" --include="$PWD" "$PWD/$ORIG_PROG" $DHRYTEST_RUNS > "$RES_DIR/$ORIG_PROG-gr.out"
ORIG_READS=$(grep "$READS_STR" $RES_DIR/$ORIG_PROG-gr.log | sed "s/$READS_STR\([:digit:]*\)/\1/g")
#echo $ORIG_READS
#echo $ORIG_BYTES
cntr=0
for j in $(seq 0 1 $((ORIG_READS-1)))
do
	for k in $(seq 0 1 31)
	do
		jprint=$(printf %04d $cntr)
		ORIG_TIME=$j
		ORIG_BIT=$k
		echo "PROG: " $ORIG_PROG  "; ORIG_TIME: " $ORIG_TIME "; ORIG_BIT: "	$ORIG_BIT 
		$VALGRIND_DIR/$VALGRIND_BIN --tool=fitin --mod-bit=$ORIG_BIT --mod-load-time=$ORIG_TIME --persist-flip=yes --log-file="$RES_DIR/$ORIG_PROG-$jprint.log" --include="$PWD" "$PWD/$ORIG_PROG" $DHRYTEST_RUNS > "$RES_DIR/$ORIG_PROG-$jprint.out"
#$VALGRIND_DIR/$VALGRIND_BIN --tool=fitin --mod-bit=$FS_BIT --mod-load-time=$FS_TIME --log-file="$RES_DIR/$FS_PROG-$jprint.log" --include="$PWD" "$PWD/$FS_PROG" $DHRYTEST_RUNS > "$RES_DIR/$FS_PROG-$jprint.out"
		cntr=$((cntr + 1))
	done
done

for FS_PROG in $FS_PROGS
do
	cntr=0
	$VALGRIND_DIR/$VALGRIND_BIN --tool=fitin --golden-run=yes --log-file="$RES_DIR/$FS_PROG-gr.log" --include="$PWD" "$PWD/$FS_PROG" $DHRYTEST_RUNS > "$RES_DIR/$FS_PROG-gr.out"
	FS_READS=$(grep "$READS_STR" $RES_DIR/$FS_PROG-gr.log | sed "s/$READS_STR\([:digit:]*\)/\1/g")
#echo $FS_READS

	READS_FACTOR=$((FS_READS / $ORIG_READS))

	for j in $(seq 0 1 $((ORIG_READS-1)))
	do
		for k in $(seq 0 1 31)
		do
			jprint=$(printf %04d $cntr)
			FS_TIME=$((j * READS_FACTOR))
			FS_BIT=$k
			echo "FS_PROG: " $FS_PROG  "; FS_TIME: " $FS_TIME "; FS_BIT: "	$FS_BIT 
#$VALGRIND_DIR/$VALGRIND_BIN --tool=fitin --mod-bit=$ORIG_BIT --mod-load-time=$ORIG_TIME --log-file="$RES_DIR/$ORIG_PROG-$jprint.log" --include="$PWD" "$PWD/$ORIG_PROG" $DHRYTEST_RUNS > "$RES_DIR/$ORIG_PROG-$cntr.out"
			$VALGRIND_DIR/$VALGRIND_BIN --tool=fitin --mod-bit=$FS_BIT --mod-load-time=$FS_TIME --persist-flip=yes --log-file="$RES_DIR/$FS_PROG-$jprint.log" --include="$PWD" "$PWD/$FS_PROG" $DHRYTEST_RUNS > "$RES_DIR/$FS_PROG-$jprint.out"
			cntr=$((cntr + 1))
		done
	done
done

END_TIME=$(date +%Y%m%d%H%M%S.%N)

printf "Time elapsed:    %.3F\n"  $(echo "$END_TIME - $START_TIME"|bc )
