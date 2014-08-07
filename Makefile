SHELL	:= /bin/sh
CC 	:= gcc
CFLAGS	:= -g -Wall
LIBS	:= -lbass -lwiringPi -lwiringPiDev

SDIR     = src
SRC	:= pituner.c parson.c

BIN	:= pituner

ODIR	:= obj
OBJ     := $(patsubst %.c,$(ODIR)/%.o,$(SRC))

LDIR     = lib
CFLAGS  += -I$(LDIR)
LDFLAGS += -L$(LDIR)

all: $(BIN)

clean:
	$(RM) $(BIN) obj/*

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(ODIR):
	@mkdir -p $@

$(ODIR)/%.o : %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean
.SUFFIXES:
.SUFFIXES: .c .o

vpath %.c   $(SDIR) $(LDIR)
vpath %.h   $(SDIR) $(LDIR)
