CC=clang
CC_OPTS=-O2 -g -std=gnu11 \
	      -Weverything \
	        -Wno-unused-parameter -Wno-format-nonliteral
LIBS=-L lib/libcello/ -I lib/libcello/include/ -static -lCello \
		 -I src/ \
     -static-libgcc

build: lib
	$(CC) $(CC_OPTS) src/*.c $(LIBS) -o avm


lib:
	curl --progress-bar --silent http://libcello.org/static/libCello-1.1.7.tar.gz | \
		tar xvzf - --directory=lib/libcello --strip-components 1
	$(MAKE) -C lib/libcello all
