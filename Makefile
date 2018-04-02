DEP = ./MyThread.h
CFL = -fPIC -c -O2 -I ../
COMP = g++

all:	dht

dht:	$(DEP) dht.c MyThread.c
	$(COMP) dht.c MyThread.c -lcrypto -lm -o dht
	
clean:
	rm -rf dht
