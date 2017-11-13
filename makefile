CC=gcc
SRC_DIR=src
SRC_FILES=$(shell find $(SRC_DIR) -type f -name *.c)
OBJ_FILES=$(patsubst %.c,%.o,$(SRC_FILES))
LOG_LEVEL=LOG_TRACE

test: $(OBJ_FILES)
	$(CC) -o $@ $^ 

src/log.o: src/log.c src/log.h
	$(CC) -DLOG_USE_COLOR -c -o $@ $<

%.o: %.c %.h
	$(CC) -DLOG_LEVEL=$(LOG_LEVEL) -c -o $@ $<

clean:
	rm -rf *.o src/*.o test
