.PHONY: compile
compile:
	gcc -c fi_client.c
	gfortran -c fitin.F95
	gfortran -c elevator_fi.F95 -g
	gfortran -o elevator fi_client.o fitin.o elevator_fi.o

run: compile
	../../../bin/valgrind --tool=fitin --mod-bit=0 --mod-load-time=14 --fnname=MAIN__ ./elevator

run2: compile
	../../../bin/valgrind --tool=fitin --mod-bit=1 --mod-load-time=21 --persist-flip=yes --fnname=MAIN__ ./elevator
