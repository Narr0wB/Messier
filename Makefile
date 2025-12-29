BASE = $(shell pwd)
UNAME = $(shell uname -s)

export SRCDIR  = $(BASE)/src
export OUTDIR  = $(BASE)/bin
export INCLUDE = $(BASE)/src

setup: 
	@mkdir -p $(OUTDIR) 

export CC = g++
export LD = ld
export DEBUG = -g
export CCFLAGS = $(DEGUG) -O3 -std=c++20

TARGET = $(OUTDIR)/messier

objs:
	make -C $(SRCDIR)/movegen
	make -C $(SRCDIR)/search
	make -C $(SRCDIR)/uci
	make -C $(SRCDIR)

$(TARGET): objs
	$(LD) $(OUTDIR)/*.o -o $(TARGET)

build: setup $(TARGET)

clean:
	rm build/*.o

