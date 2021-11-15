.PHONY: main generator serial checker clean

main:
	mpicc main.c -o main -Wall -lpthread -g

generator:
	gcc generator.c -o generator -Wall

serial:
	gcc serial.c -o serial -Wall
	
run: main
	time -p mpiexec --oversubscribe -n 4 main $(RUN_ARGS)

clean:
	rm -f main generator serial checker results/*
