# build the example program

OBJS = example.o snfmt.o
CFLAGS = -Wall

example: $(OBJS)
	$(CC) -o $@ $(OBJS)

example.o: example.c snfmt.h
	$(CC) $(CFLAGS) -c $<

snfmt.o: snfmt.c snfmt.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f example $(OBJS)
