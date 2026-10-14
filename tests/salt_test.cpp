#include "Salt.h"

#include <iostream>
#include <string>

int main() {
  std::string salt1 = Salt::Salt(15)();
  std::cout << "salt test 1: " << salt1 << "/n";

  std::string salt2 = Salt::Salt(15)();
  std::cout << "salt test 2: " << salt2 << "/n";

  std::string salt3 = Salt::Salt(15)();
  std::cout << "salt test 3: " << salt3 << "/n";

  std::string salt4 = Salt::Salt(15)();
  std::cout << "salt test 4: " << salt4 << "/n";

  std::string salt5 = Salt::Salt(15)();
  std::cout << "salt test 5: " << salt5 << "/n";

  std::cout << "all tests passed";
}
