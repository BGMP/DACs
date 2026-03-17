SRCDIR = ./src

CC          = gcc
CFLAGS      =  -O9


all:   test cleanO

clean: 
	rm -f *.o core test

cleanO: 
	rm -f *.o   

TestCreateLCP:   basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -lm -o testCreateLCP $(SRCDIR)/TestCreateLCP.c

TestCreateRAW:   basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -lm -o testCreateRAW $(SRCDIR)/TestCreateRAW.c

TestAppendFull:   basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -lm -o testAppendFull $(SRCDIR)/TestAppendFull.c

TestInsertFull:   basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -lm -o testInsertFull $(SRCDIR)/TestInsertFull.c

TestReplaceFull:   basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -lm -o testReplaceFull $(SRCDIR)/TestReplaceFull.c

TestRebuildFull:   basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -lm -o testRebuildFull $(SRCDIR)/TestRebuildFull.c

TestDeleteFull:   basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -lm -o testDeleteFull $(SRCDIR)/TestDeleteFull.c

TestTruncateDataset:   basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -lm -o testTruncateDataset $(SRCDIR)/TestTruncateDataset.c

TestTabla1:   basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -o testTabla1 $(SRCDIR)/TestTabla1.c -lm

TestTabla2:   basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -o testTabla2 $(SRCDIR)/TestTabla2.c -lm

TestPlayground:	basics.o bitrankw32int.o dacs.o
	$(CC) $(CFLAGS) basics.o bitrankw32int.o dacs.o -lm -o testPlayground $(SRCDIR)/TestPlayground.c

dacs.o:  basics.o bitrankw32int.o
	$(CC) $(CFLAGS) -c $(SRCDIR)/dacs.c

bitrankw32int.o: 
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(SRCDIRUTILS)/bitrankw32int.c
	
basics.o: basics.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(SRCDIRUTILS)/basics.c
	
basics.c: basics.h
#	touch $(SRCDIR)/$(SRCDIRUTILS)/basics.c	

basics.h: 
#	touch $(SRCDIR)/$(SRCDIRUTILS)/basics.h
	
