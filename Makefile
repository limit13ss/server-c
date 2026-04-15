CC = gcc

MOD_HTTP_CFLAGS = -Imodules/http/include
MOD_HTTP_SRC = modules/http/src

MOD_UTILITY_CFLAGS = -Imodules/utility/include
MOD_UTILITY_SRC = modules/utility/src

CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -Wconversion $(MOD_HTTP_CFLAGS) $(MOD_UTILITY_CFLAGS)
DEBUG_CFLAGS = -g

SRC = $(wildcard $(MOD_HTTP_SRC)/*.c $(MOD_UTILITY_SRC)/*.c)
OBJ = $(SRC:.c=.o)
OUT = bin

application: $(OBJ)
	$(CC) $(OBJ) -o $(OUT)/$@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

compile-flags:
	echo "$(CFLAGS)" | tr ' ' '\n' > compile_flags.txt

clean:
	rm -f $(OBJ) $(OUT)/application compile_flags.txt
