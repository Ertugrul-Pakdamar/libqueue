NAME = libqueue.a
CFLAGS = -Wall -Wextra -Werror -I./include

SRC = lib/queue.c \
		lib/alloc.c \
		lib/clear.c \
		lib/utils.c \
		lib/run.c

OBJ = build/queue.o \
		build/alloc.o \
		build/clear.o \
		build/utils.o \
		build/run.o

default: $(NAME)

main: $(NAME)
	@cc $(CFLAGS) -o main main.c $(NAME) -lssl -lcrypto
	@echo "Main executable created successfully."

$(NAME):
	@cc -c $(CFLAGS) $(SRC)
	@mv *.o build/
	@ar rcs $(NAME) $(OBJ)
	@echo "Library $(NAME) created successfully."

clean:
	@rm -f main
	@rm -f $(OBJ)
	@echo "Cleaned up successfully."

clean-all: clean
	@rm -f $(NAME)
	@echo "All cleaned up successfully."

run: clean-all main
	@./main

valgrind: clean-all main
	@valgrind --leak-check=full --show-leak-kinds=all ./main

re: clean default

re-run: clean run

.PHONY: default main clean run valgrind re re-run