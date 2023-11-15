CC ?= gcc
INC_PATH ?= $(realpath ../inc)
LIB_PATH ?= $(realpath ../lib)
LIBS ?= $(wildcard $(LIB_PATH)/*.a) -pthread -lrt -lm -lcrypto -ldl
LDFLAGS :=-g -L$(LIB_PATH) 
CFLAGS +=-g -I$(INC_PATH) -I$(INC_PATH)/openssl

EXAMPLES=secure_hybrid_its_networks

.PHONY: all

all: $(EXAMPLES)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

secure_hybrid_its_networks: src/secure_hybrid_its_networks.o src/lib/frozen.o src/lib/hmac_sha256.o src/lib/sha256.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) -Wl,-rpath,'$$ORIGIN/../../lib'

clean:
	rm -f *~ *.o $(EXAMPLES)
	rm -f *~ *.o src/$(EXAMPLES).o
	rm -f *~ *.o src/lib/frozen.o