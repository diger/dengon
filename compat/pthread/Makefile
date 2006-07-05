CC=gcc
CFLAGS=-I../../headers/posix
all: libpthread.so

libpthread.so: $(patsubst %.c,%.o,$(wildcard *.c))
	$(CC) $(CFLAGS) -nostart $^ -o $@
	
%.o: %.c
	$(CC) $(CFLAGS) -c -MD $<  

include $(wildcard *.d)

.PHONY: clean objclean
objclean:
	rm *.o *.d

clean: 
	rm *.o *.d libpthread.so
	
