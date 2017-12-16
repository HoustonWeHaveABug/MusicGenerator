MUSIC_GENERATOR_C_FLAGS=-O2 -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wreturn-type -Wshadow -Wstrict-prototypes -Wswitch -Wwrite-strings

music_generator: music_generator.o
	gcc -o music_generator music_generator.o

music_generator.o: music_generator.c music_generator.make
	gcc -c ${MUSIC_GENERATOR_C_FLAGS} -o music_generator.o music_generator.c

clean:
	rm -f music_generator music_generator.o
