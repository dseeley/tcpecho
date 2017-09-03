PROGNAME := tcpecho

SRCDIR := src
BINDIR := bin

all: clean $(PROGNAME)

$(PROGNAME): $(SRCDIR)/tcpecho.c
	mkdir -p $(BINDIR)
	cc $(SRCDIR)/tcpecho.c  $(SRCDIR)/gateway_netlink.c -o bin/$(PROGNAME)

clean:
	rm -rf bin/
