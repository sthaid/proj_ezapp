
all: ../ezsh ../ezlogfilter 

../ezsh: ezsh.c ../../src/ezapp_lib/utils_openssl.c
	gcc -I ../../src/ezapp_lib/include -Wall -o ../ezsh $^ -lreadline -lcrypto

../ezlogfilter: ezlogfilter.c
	gcc -Wall -o ../ezlogfilter ezlogfilter.c -lreadline

clean:
	rm -f ../ezsh ../ezlogfilter
