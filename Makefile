NAME := chip8

SRC_DIR := src
BUILD_DIR := build

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

INC_DIRS := $(shell find $(SRC_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CC := gcc
CFLAGS := -Wall -Wextra -Werror
OFLAGS :=
CPPFLAGS := $(INC_FLAGS) -MMD -MP
LDFLAGS :=

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm $(NAME)
	rm -rf $(BUILD_DIR)

.PHONY: re
re: clean $(NAME)
