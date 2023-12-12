SDL2_CFLAGS = `pkgconf --cflags sdl2`
SDL2_LIBS = `pkgconf --libs sdl2`

#debug:
#	cc -Wall -Wextra -O0 -g -o npdebug nanopond.c 
#test: 
#	cc -Wall -Wextra -Og -g -o mod_nanopond_change_rand mod_nanopond_change_rand.c
gui:
	cc -Wall -Wextra -Ofast $(SDL2_CFLAGS) $(SDL2_LIBS) -o nanopond nanopond.c -lpthread
	cc -Wall -Wextra -Ofast $(SDL2_CFLAGS) $(SDL2_LIBS) -o mod_nanopond_change_rand mod_nanopond_change_rand.c -lpthread
	./nanopond > c1
	./mod_nanopond_change_rand > c2
	diff -u c1 c2
	rm nanopond mod_nanopond_chane_rand c1 c2
clean:
	rm -f *.o nanopond *.dSYM
