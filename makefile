CC=gcc
PATH 	  = ${CURDIR}
INC1      = ${PATH}/src/queue
INC2      = ${PATH}/src/scheduler
INCDIRS   = -I${INC1} -I${INC2}
CFLAGS    = ${INCDIRS}

# top-level rule to compile the whole program.
all: sched_example

# program is made of several source files.
sched_example: sched_example.o scheduler.o ical.o
	$(CC) $(CFLAGS) sched_example.o scheduler.o ical.o -o sched_example

# rule for file "sched_example.o".
sched_example.o: ${PATH}/example/sched_example.c ${PATH}/src/scheduler/scheduler.h ${PATH}/src/scheduler/ical.h
	$(CC) $(CFLAGS) -g -Wall -c ${PATH}/example/sched_example.c

# rule for file "scheduler.o".
scheduler.o: ${PATH}/src/scheduler/scheduler.c ${PATH}/src/scheduler/scheduler.h
	$(CC) $(CFLAGS) -g -Wall -c ${PATH}/src/scheduler/scheduler.c

# rule for file "ical.o".
ical.o: ${PATH}/src/scheduler/ical.c ${PATH}/src/scheduler/ical.h
	$(CC) $(CFLAGS) -c -Wall ${PATH}/src/scheduler/ical.c

# rule for cleaning files generated during compilations.
clean:
	rm -f sched_example sched_example.o scheduler.o ical.o