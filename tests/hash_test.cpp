#include "Hash.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
  std::string str{"hey"};
  std::string hashed = Hash::Hash()(str);
  std::cout << "string we are hashing: " << str << "/n";
  std::cout << hashed << "/n";

  std::cout << "now checj to see if the same word or phrase hashesout to the "
               "same thing as it's supped to: ";
  std::string str2{"hey"};
  std::string hashed2 = Hash::Hash()(str2);

  assert(hashed == hashed2);

  std::cout << "all tests passed";
}
