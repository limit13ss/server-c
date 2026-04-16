CC = gcc

MOD_HTTP_INCLUDE = modules/http/include
MOD_HTTP_SRC = modules/http/src

MOD_UTIL_INCLUDE = modules/utility/include
MOD_UTIL_SRC = modules/utility/src
MOD_UTIL_TESTS = modules/utility/tests

CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -Wconversion
ALL_CFLAGS = $(CFLAGS) -I$(MOD_HTTP_INCLUDE) -I$(MOD_UTIL_INCLUDE)
DEBUG_CFLAGS = -g

SRC = $(wildcard $(MOD_HTTP_SRC)/*.c $(MOD_UTIL_SRC)/*.c)
OBJ = $(SRC:.c=.o)
OUT = bin
OUT_TEST = test

application: $(OBJ)
	$(CC) $(OBJ) -o $(OUT)/$@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

compile-flags:
	echo "$(ALL_CFLAGS)" | tr ' ' '\n' > compile_flags.txt

clean:
	rm -f $(OBJ) $(OUT)/application compile_flags.txt

test-ring-buffer:
	$(CC) $(CFLAGS) -I$(MOD_UTIL_INCLUDE) $(MOD_UTIL_TESTS)/generic_ring_buffer_test.c -o $(OUT_TEST)/$@
