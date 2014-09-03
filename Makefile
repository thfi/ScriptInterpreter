CFLAGS?=-Wall -ansi -std=c99 -pedantic
LDFLAGS?=

scriptinterpreter_HEADERS:=utils.h
scriptinterpreter_OBJECTS:=scriptinterpreter.o utils.o
scriptinterpreter_TEMPDIR:=/tmp/.scriptinterpreter_OBJECTS-$(shell echo $(scriptinterpreter_OBJECTS)$(scriptinterpreter_HEADERS)$(PWD) | md5sum | cut -f1 -d\ )

processxml_HEADERS:=utils.h
processxml_OBJECTS:=processxml.o utils.o
processxml_TEMPDIR:=/tmp/.processxml_OBJECTS-$(shell echo $(processxml_OBJECTS)$(processxml_HEADERS)$(PWD) | md5sum | cut -f1 -d\ )
processxml_CFLAGS:=$(shell pkg-config expat --cflags)
processxml_LDFLAGS:=$(shell pkg-config expat --libs)


all: scriptinterpreter processxml


scriptinterpreter: $(addprefix $(scriptinterpreter_TEMPDIR)/,$(scriptinterpreter_OBJECTS))
	$(CC) $(LDFLAGS) -o $@ $^

$(scriptinterpreter_TEMPDIR)/%.o: %.c $(scriptinterpreter_HEADERS)
	@mkdir -p $(scriptinterpreter_TEMPDIR)
	$(CC) $(CFLAGS) $(processxml_CFLAGS) -c -o $@ $<


processxml: $(addprefix $(processxml_TEMPDIR)/,$(processxml_OBJECTS))
	$(CC) $(LDFLAGS) $(processxml_LDFLAGS) -o $@ $^

$(processxml_TEMPDIR)/%.o: %.c $(processxml_HEADERS)
	@mkdir -p $(processxml_TEMPDIR)
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -f *.o *~
	rm -rf $(processxml_TEMPDIR) $(scriptinterpreter_TEMPDIR)
