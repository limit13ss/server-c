CC = gcc

MOD_HTTP_INCLUDE = modules/http/include
MOD_HTTP_SRC = modules/http/src

MOD_UTIL_INCLUDE = modules/utility/include
MOD_UTIL_SRC = modules/utility/src
MOD_UTIL_TESTS = modules/utility/tests

CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -Wconversion -D_POSIX_C_SOURCE=200809L
ALL_CFLAGS = $(CFLAGS) -I$(MOD_HTTP_INCLUDE) -I$(MOD_UTIL_INCLUDE)
DEBUG_CFLAGS = -g

SRC = $(wildcard $(MOD_HTTP_SRC)/*.c $(MOD_UTIL_SRC)/*.c)
OBJ = $(SRC:.c=.o)
OUT = out
OUT_TEST = test

application: $(OBJ)
	mkdir -p ./$(OUT)
	$(CC) $(OBJ) -o ./$(OUT)/$@

%.o: %.c
	$(CC) $(ALL_CFLAGS) -c $< -o $@

compile-flags:
	echo "$(ALL_CFLAGS)" | tr ' ' '\n' > compile_flags.txt

clean:
	rm -f $(OBJ) ./compile_flags.txt
	rm -rf ./$(OUT) ./$(OUT_TEST) 

test-ring-buffer:
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I$(MOD_UTIL_INCLUDE) $(MOD_UTIL_TESTS)/generic_ring_buffer_test.c -o ./$(OUT_TEST)/$@

test-queue:
	mkdir -p ./$(OUT_TEST)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I$(MOD_UTIL_INCLUDE) $(MOD_UTIL_TESTS)/queue_test.c $(MOD_UTIL_SRC)/queue.c -o ./$(OUT_TEST)/$@

test-thread-pool:
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I$(MOD_UTIL_INCLUDE) $(MOD_UTIL_TESTS)/thread_pool_test.c $(MOD_UTIL_SRC)/thread_pool.c $(MOD_UTIL_SRC)/queue.c -o ./$(OUT_TEST)/$@
