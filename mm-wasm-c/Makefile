mm: mm.o md5_hash.o md5.o
	gcc -o $@ $^
.c.o:
	gcc -O3 -c $<
clean:
	rm -rf *.o mm