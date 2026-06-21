CC = gcc

HTTP_INCLUDE = modules/http/include
HTTP_SRC = modules/http/src
HTTP_TESTS = modules/http/tests

UTILITY_INCLUDE = modules/utility/include
UTILITY_SRC = modules/utility/src
UTILITY_TESTS = modules/utility/tests

APP_INCLUDE = modules/app/include
APP_SRC = modules/app/src

#- D_POSIX_C_SOURCE = 200809L
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -Wconversion -D_GNU_SOURCE
ICFLAGS = $(CFLAGS) -I$(HTTP_INCLUDE) -I$(UTILITY_INCLUDE) -I$(APP_INCLUDE)
DEBUG_CFLAGS = -g

SRC = $(wildcard $(HTTP_SRC)/*.c $(UTILITY_SRC)/*.c $(APP_SRC)/*.c)
OBJ = $(SRC:.c=.o)

CLIENT_PARSER_SRC = $(wildcard $(HTTP_SRC)/*.c $(UTILITY_SRC)/*.c $(HTTP_TESTS)/test_client_parser.c)
CLIENT_PARSER_OBJ = $(CLIENT_PARSER_SRC:.c=.o.test)

OUT = out
OUT_TEST = test
MK_OUT_TEST = mkdir -p ./$(OUT_TEST)

application: $(OBJ)
	mkdir -p ./$(OUT)
	$(CC) $(OBJ) -o ./$(OUT)/$@

%.o: %.c
	$(CC) $(ICFLAGS) -c $< -o $@

compile-flags:
	echo "$(ICFLAGS)" | tr ' ' '\n' > compile_flags.txt

clean:
	rm -f $(OBJ) ./compile_flags.txt
	rm -f $(CLIENT_PARSER_OBJ)
	rm -rf ./$(OUT) ./$(OUT_TEST)

test-ring-buffer:
	$(MK_OUT_TEST)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I$(UTILITY_INCLUDE) -I$(UTILITY_TESTS) $(UTILITY_TESTS)/test_generic_ring_buffer.c -o ./$(OUT_TEST)/$@

test-queue:
	$(MK_OUT_TEST)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I$(UTILITY_INCLUDE) -I$(UTILITY_TESTS) $(UTILITY_TESTS)/test_queue.c $(UTILITY_SRC)/queue.c -o ./$(OUT_TEST)/$@

test-thread-pool:
	$(MK_OUT_TEST)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -I$(UTILITY_INCLUDE) -I$(UTILITY_TESTS) $(UTILITY_TESTS)/test_thread_pool.c $(UTILITY_SRC)/thread_pool.c $(UTILITY_SRC)/queue.c -o ./$(OUT_TEST)/$@

%.o.test: %.c
	$(CC) $(ICFLAGS) $(DEBUG_CFLAGS) -c $< -o $@

test-client-parser: $(CLIENT_PARSER_OBJ)
	$(MK_OUT_TEST)
	$(CC) $(CLIENT_PARSER_OBJ) -o ./$(OUT_TEST)/$@
