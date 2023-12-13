SDL2_CFLAGS = `pkgconf --cflags sdl2`
SDL2_LIBS = `pkgconf --libs sdl2`

#debug:
#	cc -Wall -Wextra -O0 -g -o npdebug nanopond.c 
test: 
	cc -Wall -Wextra -Og -g -o mod_nanopond mod_nanopond.c
gui:
	cc -Wall -Wextra -Ofast -Og -o nanopond nanopond.c -lpthread
	cc -Wall -Wextra -Ofast -Og -o mod_nanopond mod_nanopond.c
	./nanopond > c1
	./mod_nanopond > c2
	diff -u c1 c2
	rm nanopond mod_nanopond c1 c2
clean:
	rm -f *.o nanopond *.dSYM
