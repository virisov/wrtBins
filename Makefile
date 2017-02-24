SHELL=/bin/sh

CC=g++
CFLAGS=-std=c++11
INC = asciiROfile.h BufrStruct.h dataVec.h Endian.h opnGNSS.h ROBinary2.h
OBJ0 = asciiROfile.o BufrStruct.o dataVec.o Endian.o opnGNSS.o ROBinary2.o
OBJ = $(OBJ0) wrtGns.o wrtBufr.o check_endian.o readBufr.o
CPP = asciiROfile.cpp BufrStruct.cpp dataVec.cpp Endian.cpp opnGNSS.cpp ROBinary2.cpp \
	wrtGns.cpp wrtBufr.cpp check_endian.cpp readBufr.cpp

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
a: $(OBJ0)
	ar cr libwrtbins.a $^
	ranlib libwrtbins.a
	
wrtGns: $(OBJ0)
	$(CC) $(CFLAGS) -c $@.cpp
	$(CC) -o $@ $@.o $(CFLAGS) -lwrtbins

wrtBufr: $(OBJ0)
	$(CC) $(CFLAGS) -c $@.cpp
	$(CC) -o $@ $@.o $(CFLAGS) -lwrtbins

check_endian: $(OBJ0)
	$(CC) $(CFLAGS) -c $@.cpp
	$(CC) -o $@ $@.o $(CFLAGS) -lwrtbins

readBufr: $(OBJ0)
	$(CC) $(CFLAGS) -c $@.cpp
	$(CC) -o $@ $@.o $(CFLAGS) -lwrtbins
	
clean:
	rm *.o -f
	rm *.exe -f
	rm *.a -f
		
