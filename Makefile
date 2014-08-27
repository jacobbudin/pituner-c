SHELL	:= /bin/sh
CC 	:= gcc
CFLAGS	:= -g -Wall
LIBS	:= -lcurl -lbass -lwiringPi -lwiringPiDev

SDIR     = src
SRC	:= pituner.c ptn_signal.c ptn_pls.c parson.c

ETCDIR   = etc

BIN	:= pituner

INIT	:= extras/pituner.sh

ODIR	:= obj
OBJ     := $(patsubst %.c,$(ODIR)/%.o,$(SRC))

LDIR     = lib
CFLAGS  += -I$(LDIR)
LDFLAGS += -L$(LDIR)

all: $(BIN)

clean:
	$(RM) $(BIN) obj/*

install: $(BIN) $(INIT)
	@mkdir -p /etc/pituner/
	@cp -f $(ETCDIR)/* /etc/pituner/
	@chown -R root /etc/pituner/
	@chgrp -R root /etc/pituner/
	@cp -f $(BIN) /usr/sbin/pituner
	@chown root /usr/sbin/pituner
	@chgrp root /usr/sbin/pituner
	@cp -f $(INIT) /etc/init.d/pituner
	@chown root /etc/init.d/pituner
	@chgrp root /etc/init.d/pituner
	@update-rc.d pituner defaults > /dev/null

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(ODIR):
	@mkdir -p $@

$(ODIR)/%.o : %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean install
.SUFFIXES:
.SUFFIXES: .c .o

vpath %.c   $(SDIR) $(LDIR)
vpath %.h   $(SDIR) $(LDIR)
