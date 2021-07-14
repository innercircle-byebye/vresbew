NAME		=	webserv

SRC_PATH	=	./src/
SRC_FILE	=	webserv/main.cpp \
				webserv/config/HttpConfig.cpp \
				webserv/config/ServerConfig.cpp \
				webserv/config/LocationConfig.cpp \
				webserv/config/Tokenizer.cpp \
				webserv/socket/Connection.cpp \
				webserv/socket/Cycle.cpp \
				webserv/socket/Kqueue.cpp \
				webserv/socket/Listening.cpp \
				webserv/socket/SocketManager.cpp \
				webserv/message/Response.cpp \
				webserv/message/handler/MessageHandler.cpp \
				webserv/message/handler/RequestHandler.cpp \
				webserv/message/handler/ResponseHandler.cpp


LOGFILE		=	./error.log

SRCS		=	$(SRC_FILE:%.cpp=$(SRC_PATH)%.cpp)
OBJS		=	$(SRCS:.cpp=.o)

CXX			=	clang++
RM			=	rm -f
CXXFLAGS	=	-I./src/ -Wall -Wextra -Werror -std=c++98 -pedantic # Do not use "nullptr"
LEAKS		=	-g -fsanitize=address -fsanitize=undefined

# 컴파일 시 -I.$(SRC_PATH) 를 주긴 했는데 이를 조금 더 깔끔하게 하는 법이 있을것 같습니다
# %.o:			%.cpp
# 				$(CXX) $(CXXFLAGS) -I$(SRC_PATH) -c $< -o $@

all:			$(NAME)

$(NAME):		$(OBJS)
				$(CXX) $(CXXFLAGS) $(SRCS) -o $(NAME)

clean:
				$(RM) $(OBJS)

fclean:			clean
				$(RM) $(NAME) $(LOGFILE)

debug:			fclean $(OBJS)
				$(CXX) $(CXXFLAGS) $(INCLUDES) $(LEAKS) $(SRCS) -o $(NAME)

re:				fclean all

.PHONY:			all clean fclean debug re
