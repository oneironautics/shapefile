all:
	gcc -c -Wall -Werror -fpic Shapefile.c
	gcc -shared -o libshapefile.so Shapefile.o

clean:
	rm -rf *o *so
