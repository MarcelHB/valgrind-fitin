.PHONY: compile
compile:
	gcc -c fi_client.c
	gfortran -c fitin.F95
	gfortran -c elevator_fi.F95 -g
	gfortran -o elevator fi_client.o fitin.o elevator.o

run: compile
	../../../bin/valgrind --tool=fitin --mod-bit=0 --mod-load-time=14 --fnname=MAIN__ ./elevator
