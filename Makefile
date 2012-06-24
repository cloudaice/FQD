fqdfs : fqdfs.o log.o
	gcc -g `pkg-config fuse --libs` -o fqdfs fqdfs.o log.o

fqdfs.o : fqdfs.c log.h params.h
	gcc -g -Wall `pkg-config fuse --cflags` -c fqdfs.c

log.o : log.c log.h params.h
	gcc -g -Wall `pkg-config fuse --cflags` -c log.c

clean:
	rm -f fqdfs *.o

