
CC = gcc


CFLAGS := -g
#LDFLAGS


objs = main.o flv.o nalu.o
target = app


all : $(objs)
	$(CC) -o $(target) $(objs) $(CFLAGS) $(LDFLAGS)

%.o : %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY : clean
clean :
	rm -f *.o
	rm -f $(target)


