# =============================================================================
# libqueue — dynamic queue · SPSC ring buffer · async listener
# =============================================================================
#
#  Targets (run `make` or `make help` to see this list):
#
#    help       Show this message                     (default)
#    all        Build deps + libqueue.a
#    examples   Build all programs in examples/
#    valgrind   Run examples/01_sync_queue under Valgrind
#    clean      Remove object files and example binaries
#    fclean     clean + remove libqueue.a
#    re         fclean + all
#
# =============================================================================

# -- Toolchain ----------------------------------------------------------------

CC      := cc
AR      := ar
ARFLAGS := rcs

# -- Library name -------------------------------------------------------------

NAME    := libqueue.a

# -- Include paths ------------------------------------------------------------

CFLAGS  := -Wall -Wextra -Werror   \
           -Iinclude               \
           -Ideps/libmem/include      \
           -Ideps/libosal/include

# -- Sources & objects --------------------------------------------------------

SRC :=  src/queue_alloc.c  \
        src/queue_ops.c    \
        src/queue_clear.c  \
        src/queue_run.c    \
        src/queue_utils.c  \
        src/ring_ops.c     \
        src/listener.c

OBJ := $(SRC:src/%.c=build/%.o)

# -- Sub-project artifacts ----------------------------------------------------

LIBMEM  := deps/libmem/libmem.a
LIBOSAL := deps/libosal/libosal.a

# -- Terminal colors (no-op if terminal does not support them) ----------------

RESET  := \033[0m
BOLD   := \033[1m
DIM    := \033[2m
GREEN  := \033[32m
CYAN   := \033[36m
YELLOW := \033[33m

# =============================================================================
# Default target
# =============================================================================

.DEFAULT_GOAL := help

.PHONY: help
help:
	@printf "\n$(BOLD)libqueue$(RESET) — available targets\n\n"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make all"      "Build deps + $(NAME)"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make examples" "Build all programs in examples/"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make valgrind" "Run examples/01_sync_queue under Valgrind (leak check)"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make clean"    "Remove object files and example binaries"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make fclean"   "clean + remove $(NAME)"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make re"       "fclean + all"
	@printf "\n  $(DIM)Dependencies: deps/libmem  deps/libosal$(RESET)\n\n"

# =============================================================================
# Build targets
# =============================================================================

## all: Build all dependencies and libqueue.a
.PHONY: all
all: $(LIBMEM) $(LIBOSAL) $(NAME)

# -- Sub-projects -------------------------------------------------------------

$(LIBMEM):
	@printf "$(DIM)→ building deps/libmem$(RESET)\n"
	@$(MAKE) -C deps/libmem all --no-print-directory

$(LIBOSAL):
	@printf "$(DIM)→ building deps/libosal$(RESET)\n"
	@$(MAKE) -C deps/libosal all --no-print-directory

# -- Object compilation -------------------------------------------------------

build/%.o: src/%.c | build
	@printf "  $(DIM)CC$(RESET)  $<\n"
	@$(CC) $(CFLAGS) -c $< -o $@

build:
	@mkdir -p build

# -- Static library -----------------------------------------------------------

$(NAME): $(OBJ)
	@$(AR) $(ARFLAGS) $@ $^
	@printf "$(GREEN)$(BOLD)✓ $(NAME)$(RESET)\n"

# =============================================================================
# Examples
# =============================================================================

EXAMPLES_SRC := $(wildcard examples/*.c)
EXAMPLES_BIN := $(EXAMPLES_SRC:examples/%.c=examples/bin/%)
EXAMPLES_CFLAGS := $(CFLAGS) -Iexamples

## examples: Build all programs in examples/
.PHONY: examples
examples: $(LIBMEM) $(LIBOSAL) $(NAME) $(EXAMPLES_BIN)

examples/bin/%: examples/%.c $(NAME) $(LIBMEM) $(LIBOSAL) | examples/bin
	@printf "  $(DIM)CC$(RESET)  $<\n"
	@$(CC) $(EXAMPLES_CFLAGS) -o $@ $< $(NAME) $(LIBMEM) $(LIBOSAL) -lpthread
	@printf "$(GREEN)✓ $@$(RESET)\n"

examples/bin:
	@mkdir -p examples/bin

# =============================================================================
# Valgrind
# =============================================================================

## valgrind: Build examples then run 01_sync_queue under Valgrind
.PHONY: valgrind
valgrind: examples
	@printf "$(YELLOW)$(BOLD)▶  valgrind$(RESET)\n\n"
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		examples/bin/01_sync_queue

# =============================================================================
# Cleanup targets
# =============================================================================

## clean: Remove object files and example binaries
.PHONY: clean
clean:
	@rm -rf examples/bin $(OBJ)
	@$(MAKE) -C deps/libmem  clean --no-print-directory
	@$(MAKE) -C deps/libosal clean --no-print-directory
	@printf "$(DIM)cleaned$(RESET)\n"

## fclean: clean + remove libqueue.a
.PHONY: fclean
fclean: clean
	@rm -f $(NAME)
	@$(MAKE) -C deps/libmem  fclean --no-print-directory
	@$(MAKE) -C deps/libosal fclean --no-print-directory

## re: fclean + all
.PHONY: re
re: fclean all