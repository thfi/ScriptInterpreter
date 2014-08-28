CFLAGS=-Wall -ansi -std=c99 -pedantic
LDFLAGS=
HEADERS=

all: scriptinterpreter

scriptinterpreter: scriptinterpreter.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *~
