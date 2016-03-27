CC=g++
CFLAGS=-std=c++11 -Wall
LIBS=-lboost_system -lboost_program_options -lboost_thread -luhd

IDIR_TX=tx/include
SOURCES_TX=tx/src/*

IDIR_RX=rx/include
SOURCES_RX=rx/src/*

#all: main_rx
all: main_tx main_rx

main_tx: $(SOURCES_TX)
	$(CC) $(CFLAGS) -I$(IDIR_TX) $(SOURCES_TX) $(LIBS) -o main_tx

main_rx: $(SOURCES_RX)
	$(CC) $(CFLAGS) -I$(IDIR_RX) $(SOURCES_RX) $(LIBS) -o main_rx

#main_rx:
	#g++ -std=c++11 -Wall -Irx/include rx/src/* -o main_rx

.PHONY: clean

clean:
	rm -f *~ .*~ */*~ */.*~ */*/*~ */*/.*~ *.o */*/*.o main_tx main_rx
