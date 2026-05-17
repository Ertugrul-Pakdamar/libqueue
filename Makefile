NAME   = libqueue.a
LIBMEM = libmem/libmem.a
LIBOSAL = libosal/libosal.a
CFLAGS = -Wall -Wextra -Werror -Iinclude -Ilibmem/include -Ilibosal/include

SRC = lib/queue_ops.c \
		lib/queue_alloc.c \
		lib/queue_clear.c \
		lib/queue_utils.c \
		lib/queue_run.c \
		lib/ring_ops.c \
		lib/listener.c

OBJ = build/queue_ops.o \
		build/queue_alloc.o \
		build/queue_clear.o \
		build/queue_utils.o \
		build/queue_run.o \
		build/ring_ops.o \
		build/listener.o

default: $(LIBMEM) $(LIBOSAL) $(NAME)

$(LIBMEM):
	@$(MAKE) -C libmem

$(LIBOSAL):
	@$(MAKE) -C libosal

main: $(NAME) $(LIBMEM) $(LIBOSAL)
	@cc $(CFLAGS) -o main main.c $(NAME) $(LIBMEM) $(LIBOSAL) -lpthread
	@echo "Main executable created successfully."

$(NAME):
	@cc -c $(CFLAGS) $(SRC)
	@mv *.o build/
	@ar rcs $(NAME) $(OBJ)
	@echo "Library $(NAME) created successfully."

clean:
	@rm -f main
	@rm -f $(OBJ)
	@$(MAKE) -C libmem clean
	@$(MAKE) -C libosal clean
	@echo "Cleaned up successfully."

clean-all: clean
	@rm -f $(NAME)
	@$(MAKE) -C libmem clean-all
	@$(MAKE) -C libosal clean-all
	@echo "All cleaned up successfully."

run: clean-all main
	@./main

valgrind: clean-all main
	@valgrind --leak-check=full --show-leak-kinds=all ./main

re: clean default

re-run: clean run

.PHONY: default main clean run valgrind re re-run