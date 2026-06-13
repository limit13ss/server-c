CC = gcc

HTTP_INCLUDE = modules/http/include
HTTP_SRC = modules/http/src

UTILITY_INCLUDE = modules/utility/include
UTILITY_SRC = modules/utility/src
UTILITY_TESTS = modules/utility/tests

CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -Wconversion -D_POSIX_C_SOURCE=200809L
ALL_CFLAGS = $(CFLAGS) -I$(HTTP_INCLUDE) -I$(UTILITY_INCLUDE)
DEBUG_CFLAGS = -g

SRC = $(wildcard $(HTTP_SRC)/*.c $(UTILITY_SRC)/*.c)
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
	mkdir -p ./$(OUT_TEST)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I$(UTILITY_INCLUDE) -I$(UTILITY_TESTS) $(UTILITY_TESTS)/test_generic_ring_buffer.c -o ./$(OUT_TEST)/$@

test-queue:
	mkdir -p ./$(OUT_TEST)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I$(UTILITY_INCLUDE) -I$(UTILITY_TESTS) $(UTILITY_TESTS)/test_queue.c $(UTILITY_SRC)/queue.c -o ./$(OUT_TEST)/$@

test-thread-pool:
	mkdir -p ./$(OUT_TEST)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I$(UTILITY_INCLUDE) -I$(UTILITY_TESTS) $(UTILITY_TESTS)/test_thread_pool.c $(UTILITY_SRC)/thread_pool.c $(UTILITY_SRC)/queue.c -o ./$(OUT_TEST)/$@

test-array-util:
	mkdir -p ./$(OUT_TEST)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I$(UTILITY_INCLUDE) -I$(UTILITY_TESTS) $(UTILITY_TESTS)/test_array_utils.c $(UTILITY_SRC)/array_utils.c -o ./$(OUT_TEST)/$@
