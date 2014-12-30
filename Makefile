CC=clang
CC_OPTS=-O3 -g3 -std=gnu11 \
				-fsanitize=address -fsanitize=undefined \
	      -Weverything \
	        -Wno-unused-parameter -Wno-format-nonliteral
LIBS= -I src/ \
     -static-libgcc

build: 
	$(CC) -D EXECUTABLE -D NDEBUG $(CC_OPTS) src/*.c $(LIBS) -o avm
