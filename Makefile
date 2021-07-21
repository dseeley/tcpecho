PROGNAME := tcpecho
SRCDIR := src
BINDIR := bin

TAG := $(if $(TAG),$(TAG),latest)
IMG_NAME := $(if $(IMG_NAME),$(IMG_NAME),tcpecho)
DOCKERREPO = dseeley/$(IMG_NAME)


all: clean $(PROGNAME)

debug: CCFLAGS += -DDEBUG -g
debug: all


$(PROGNAME): $(SRCDIR)/tcpecho.c
	mkdir -p $(BINDIR)
	$(CC) $(CCFLAGS) $(SRCDIR)/tcpecho.c  $(SRCDIR)/gateway_netlink.c -lpthread -o bin/$(PROGNAME)

clean:
	rm -rf bin/


dockerbuild: FORCE
	docker build --pull -t $(DOCKERREPO):$(TAG) -t $(DOCKERREPO):latest .

dockerpush: dockerbuild
	docker push $(DOCKERREPO):$(TAG)
	docker push $(DOCKERREPO):latest

FORCE:
