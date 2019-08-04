all : connect.c listen.c
	make connect
	make listen
connect : connect.c
	gcc -m64 -std=gnu99 -Wall -o connect connect.c
listen : listen.c
	gcc -m64 -std=gnu99 -Wall -o listen listen.c
clean :
	rm listen connect
