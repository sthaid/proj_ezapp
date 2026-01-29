
all: ../ezsh ../ezlogfilter 

../ezsh: ezsh.c
	gcc -Wall -o ../ezsh ezsh.c -lreadline

../ezlogfilter: ezlogfilter.c
	gcc -Wall -o ../ezlogfilter ezlogfilter.c -lreadline

clean:
	rm -f ../ezsh ../ezlogfilter
