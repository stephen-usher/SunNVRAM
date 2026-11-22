CFLAGS=-g

sunnvram: sunnvram.o
	$(CC) -o $@ $<

clean:
	rm *.o sunnvram
