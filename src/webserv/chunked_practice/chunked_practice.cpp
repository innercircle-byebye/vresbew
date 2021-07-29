#include <cstdlib>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

#define CRLF 2

bool chunked_close_check(const char *buf, int len = -1) {
  bool result;

  size_t length = 0;
  char *ptr;

  ptr = (char *)buf;
  if (len == -1)
    length = strtoul(ptr, &ptr, 16) + CRLF;
  else
    length = len;

  while ((length = strtoul(ptr + length + CRLF, &ptr, 16)) >= 0) {
    std::cout << "------------fbuffer---------------" << std::endl;
    std::cout << ptr << std::endl;
    std::cout << "------------fbuffer---------------" << std::endl;
    if (strcmp(ptr, "\r\n\r\n") == 0)
      return (true);
  }

  result = false;

  return (result);
}

int main(int argc, char **argv) {
  if (argc == 2) {
    std::ifstream t(argv[1]);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());

    std::cout << "=============file===============" << std::endl;
    std::cout << str << std::endl;
    std::cout << "=============file===============" << std::endl;

    std::cout << "\n\nlet's check\n\n"
              << std::endl;

    if (chunked_close_check(str.c_str()))
      std::cout << "close" << std::endl;
    else
      std::cout << "still open" << std::endl;

  } else
    std::cout << "Usage: ./chunked_practice filepath" << std::endl;
}
