NAME := chip8

SRC_DIR := src
BUILD_DIR := build

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

INC_DIRS := $(shell find $(SRC_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CC := gcc
SDL2_CFLAGS := $(shell sdl2-config --cflags)
SDL2_LIBS := $(shell sdl2-config --libs)
CFLAGS := -Wall -Wextra -Werror $(SDL2_CFLAGS)
OFLAGS :=
CPPFLAGS := $(INC_FLAGS) -MMD -MP
LDFLAGS := $(SDL2_LIBS)

$(NAME): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm $(NAME)
	rm -rf $(BUILD_DIR)

.PHONY: re
re: clean $(NAME)
