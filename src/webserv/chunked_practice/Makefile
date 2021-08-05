CXX=clang++
CXXFLAGS= -Wall -Wextra -Werror -std=c++98 -pedantic -g -fsanitize=address
BINS= chunked_practice
OBJS= chunked_practice.o

chunked_practice: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

clean:
	rm -rf *dSYM $(BINS) $(OBJS)
