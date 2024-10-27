OUTD = $(OUTDIR)
TARGET = $(OUTDIR)/main.o

all: 
	$(CC) $(CCFLAGS) -I$(INCLUDE) -c Engine.cpp -o $(OUTD)/tmp.engine.o
	$(CC) $(CCFLAGS) -I$(INCLUDE) -c main.cpp -o $(OUTD)/tmp.entry.o
	$(CC) $(CCFLAGS) -I$(INCLUDE) -c Log.cpp -o $(OUTD)/tmp.log.o
	$(CC) $(CCFLAGS) -I$(INCLUDE) -c Misc.cpp -o $(OUTD)/tmp.misc.o
	
	$(LD) -r -o $(TARGET) $(OUTD)/tmp.*.o
	rm $(OUTD)/tmp.*
