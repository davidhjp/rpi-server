CC=gcc
OBJ=src/server.o src/test.o

test: $(OBJ)
	$(CC) -o $@ $^ 

%.o: src/%.c src/%.h
	$(CC) -c -o $@ $<

clean:
	rm -rf *.o src/*.o test
