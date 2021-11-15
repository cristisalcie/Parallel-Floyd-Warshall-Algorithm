.PHONY: main generator serial checker clean

main:
	gcc main.c -o main -Wall -lpthread

generator:
	gcc generator.c -o generator -Wall

serial:
	gcc serial.c -o serial -Wall
	
run: main
	time -p ./main $(RUN_ARGS)

clean:
	rm -f main generator serial checker results/*
