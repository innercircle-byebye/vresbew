#include <cstdlib>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

bool chunked_okay_to_finish(const char *buf, int len = -1) {
  bool result;

  size_t length = 0;
  char *ptr = NULL;
  if (len == -1)
    length = strtoul(buf, &ptr, 16);
  else
    length = len;

  std::cout << "------------fbuffer---------------" << std::endl;
  std::cout << ptr << std::endl;
  std::cout << "------------fbuffer---------------" << std::endl;

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

    if (chunked_okay_to_finish(str.c_str()))
      std::cout << "close" << std::endl;
    else
      std::cout << "still open" << std::endl;

  } else
    std::cout << "Usage: ./chunked_practice filepath" << std::endl;
}
