PROGNAME := tcpecho

SRCDIR := src
BINDIR := bin

all: clean $(PROGNAME)

$(PROGNAME): $(SRCDIR)/tcpecho.c
	mkdir -p $(BINDIR)
	cc $(SRCDIR)/tcpecho.c -o bin/$(PROGNAME)

clean:
	rm -rf bin/
