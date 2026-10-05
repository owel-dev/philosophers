NAME = philo
CC = gcc
CFLAGS = -Wall -Wextra -Werror -pthread
RM = rm -f

FILE = 	0_philo\
		1_routines\
		2_utils \
		3_arg \
		4_end

SRCS = $(addprefix ./, $(addsuffix .c, $(FILE)))
OBJS = $(addprefix ./, $(addsuffix .o, $(FILE)))

%.o: %.c
	@$(CC) $(INCS) -c $< -o $@

all : $(NAME)

$(NAME) : $(OBJS)
	@$(CC) $(LIBFT) $(OBJS) -o $@

TEST_DIR = tests
TEST_BUILD = $(TEST_DIR)/build
TEST_SRCS = $(filter-out ./0_philo.c, $(SRCS))

test: $(NAME) | $(TEST_BUILD)
	@$(CC) $(CFLAGS) -I . $(TEST_DIR)/test_arg.c $(TEST_SRCS) -o $(TEST_BUILD)/test_arg
	@./$(TEST_BUILD)/test_arg
	@$(CC) $(CFLAGS) $(TEST_DIR)/test_scenario.c -o $(TEST_BUILD)/test_scenario
	@./$(TEST_BUILD)/test_scenario ./$(NAME)

$(TEST_BUILD):
	@mkdir -p $(TEST_BUILD)

clean:
	@$(RM) $(OBJS)

fclean: clean
	@$(RM) $(NAME)
	@$(RM) -r $(TEST_BUILD)

re: clean all

.PHONY: all test clean fclean re
