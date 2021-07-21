CXX=clang++
CXXFLAGS= -Wall -Wextra -Werror -g -fsanitize=address
BINS= cgi_practice
OBJS= cgi_practice.o

cgi_practice: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

clean:
	rm -rf *dSYM $(BINS) $(OBJS)
