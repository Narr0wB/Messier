BASE = $(shell pwd)
UNAME = $(shell uname -s)

export SRCDIR  = $(BASE)/src
export OUTDIR  = $(BASE)/bin
export INCLUDES = -I$(BASE)/include -I$(SRCDIR)

setup: 
	@mkdir -p $(OUTDIR) 

export CC = g++
export LD = g++ 
export DEBUG = -g
export CCFLAGS = -g -O0 -std=c++20

TARGET = $(OUTDIR)/messier

objs:
	make -C $(SRCDIR)/movegen
	make -C $(SRCDIR)/search
	make -C $(SRCDIR)

$(TARGET): objs
	$(LD) $(OUTDIR)/*.o -o $(TARGET)

build: setup $(TARGET)

clean:
	rm bin/*

