CFLAGS=-std=gnu99 -Wall -Wextra -g
test_mmal: mmal.o test_mmal.o
	gcc -o $@ $^
test: test_mmal
	./test_mmal
mmal.o: mmal.c mmal.h
test_mmal.o: test_mmal.c mmal.h
clean:
	-rm mmal.o test_mmal.o test_mmal
