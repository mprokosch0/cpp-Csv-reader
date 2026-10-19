NAME = test

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++20 -Iincludes -MMD -MP -O3

SRCS =	srcs/main.cpp \


OBJS_DIR = objs

DEPS	= $(OBJS:.o=.d)

OBJS =	$(SRCS:%.cpp=$(OBJS_DIR)/%.o)

$(OBJS_DIR)/%.o:%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS)  -c $< -o $@

$(NAME) : $(OBJS)
	$(CXX) $(CXXFLAGS)  -o $(NAME) $(OBJS) 

all : $(NAME)

clean :
	rm -rf $(OBJS_DIR)

fclean : clean
	rm -f $(NAME)

re : fclean all

-include $(DEPS)

.PHONY : all clean fclean re