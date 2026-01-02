
ezsh: ezsh.c
	gcc -Wall -o ezsh ezsh.c -lreadline

clean:
	rm -f ezsh
