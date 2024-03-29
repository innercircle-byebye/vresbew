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
				webserv/message/Request.cpp \
				webserv/message/Response.cpp \
				webserv/message/StatusMessage.cpp \
				webserv/message/MimeType.cpp \
				webserv/message/handler/MessageHandler.cpp \
				webserv/message/handler/RequestHandler.cpp \
				webserv/message/handler/ResponseHandler.cpp \
				webserv/message/handler/CgiHandler.cpp

LOGFILE		=	./error.log

SRCS		=	$(SRC_FILE:%.cpp=$(SRC_PATH)%.cpp)
OBJS		=	$(SRCS:.cpp=.o)

CXX			=	clang++
RM			=	rm -f
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++98 -pedantic # Do not use "nullptr"
LEAKS		=	-g -fsanitize=address -fsanitize=undefined

all:			$(NAME)

%.o:			%.cpp
				$(CXX) $(CXXFLAGS) -I$(SRC_PATH) -o $@ -c $<

$(NAME):		$(OBJS)
				$(CXX) $(CXXFLAGS) -I$(SRC_PATH) $(OBJS) -o $(NAME)

clean:
				$(RM) $(OBJS)
				$(RM) -r ./webserv.dSYM



fclean:			clean
				$(RM) $(NAME) $(LOGFILE)



debug:			fclean
				@echo "DEBUG MODE BUILD START...."
				@$(CXX) $(CXXFLAGS) -I$(SRC_PATH) $(LEAKS) $(SRCS) -o $(NAME)
				@echo "DEBUG MODE BUILD DONE"

re:				fclean all

.PHONY:			all clean fclean debug re
