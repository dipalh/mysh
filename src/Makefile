CFLAGS = -g -Wall -Wextra -Werror -fsanitize=address,leak,object-size,bounds-strict,undefined -fsanitize-address-use-after-scope

all: mysh

mysh: mysh.o builtins.o commands.o variables.o io_helpers.o linkedlist.o processes.o jobs.o socket.o chat_helpers.o
	gcc ${CFLAGS} -o $@ $^ 

%.o: %.c builtins.h commands.h variables.h io_helpers.h linkedlist.h processes.h jobs.h socket.h chat_helpers.h
	gcc ${CFLAGS} -c $< 

clean:
	rm *.o mysh
