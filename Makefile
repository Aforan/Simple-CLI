compile-all: term

term:
	g++ src/term.c -o bin/term

clean:
	rm bin/* -rf