CC=gcc
SRC_DIR=src
SRC_FILES=$(shell find $(SRC_DIR) -type f -name *.c)
OBJ_FILES=$(patsubst %.c,%.o,$(SRC_FILES))
LOG_LEVEL=LOG_TRACE
CFLAGS=-Wall
LIBS=-lcheck -lpthread
MAC=-DLOG_USE_COLOR -DLOG_LEVEL=$(LOG_LEVEL) 

test: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

src/log.o: src/log.c src/log.h
	$(CC) $(CFLAGS) $(MAC) -c -o $@ $<

%.o: %.c %.h
	$(CC) $(CFLAGS) $(MAC) -c -o $@ $<

clean:
	rm -rf *.o src/*.o test *.stackdump
