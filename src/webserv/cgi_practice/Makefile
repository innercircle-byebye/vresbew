CXX=clang++
CXXFLAGS= -Wall -Wextra -Werror -g -fsanitize=address
BINS= cgi_practice
OBJS= cgi_practice.o

cgi_practice: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

bipipe:
	$(CXX) $(CXXFLAGS) bipipe.cpp -o bipipe

clean:
	rm -rf *dSYM $(BINS) $(OBJS)
	rm -rf bipipe
