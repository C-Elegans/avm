CC=clang
CC_OPTS=-g -std=gnu11 \
				-fsanitize=address -fsanitize=undefined \
	      -Weverything \
	        -Wno-unused-parameter -Wno-format-nonliteral
LIBS= -I src/ \
     -static-libgcc

build: lib
	$(CC) $(CC_OPTS) src/*.c $(LIBS) -o avm


lib:
	curl --progress-bar --silent http://libcello.org/static/libCello-1.1.7.tar.gz | \
		tar xvzf - --directory=lib/libcello --strip-components 1
	$(MAKE) -C lib/libcello all
