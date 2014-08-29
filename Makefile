CFLAGS?=-Wall -ansi -std=c99 -pedantic
LDFLAGS?=
scriptinterpreter_HEADERS:=utils.h
scriptinterpreter_OBJECTS:=scriptinterpreter.o utils.o
TEMPDIR:=/tmp/.scriptinterpreter_OBJECTS-$(shell echo $(scriptinterpreter_OBJECTS)$(scriptinterpreter_HEADERS)$(PWD) | md5sum | cut -f1 -d\ )

all: scriptinterpreter

scriptinterpreter: $(addprefix $(TEMPDIR)/,$(scriptinterpreter_OBJECTS))
	$(CC) $(LDFLAGS) -o $@ $^

$(TEMPDIR)/%.o: %.c $(scriptinterpreter_HEADERS)
	@mkdir -p $(TEMPDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *~
	rm -rf $(TEMPDIR)
