CFLAGS=-g -Wall -O3 -mtune=native
LDFLAGS=-lrt
all: bch_test

bch_test: bch_test.o bch_enc.o bch_dec.o

bch_test.o bch_enc.o bch_dec.o: bch.h
bch.h: bignum.h

clean:
	rm *.o bch_test
