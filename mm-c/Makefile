mm: mm.o decoder.o md5_hash.o md5.o
	gcc -pthread -o $@ $^
.c.o:
	gcc -O3 -c $<
clean:
	rm -rf *.o mm