// a funtion object that produces abuser definedlength stringbof random
// letters, numbers, andbsymbols to be used as a salt for a password

#ifndef SALT_H
#define SALT_H

#include <array>
#include <random>
#include <string>

namespace Salt {
class Salt {
 private:
  static inline const char m_alphanum[] =
      "0123456789"
      "!@#$%&^"
      " "
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  int m_charLength;
  std::size_t m_stringLength;
  std::random_device m_rm;

 public:
  Salt(const std::size_t size) : m_stringLength{size} {
    m_charLength = std::size(m_alphanum);
  }

  std::string operator()() {
    static std::mt19937 mt{m_rm()};

    std::string salt;

    for (std::size_t i = 0; i < m_stringLength; ++i) {
      // although it goes against convention, instantiating
      // the distribution inside of thr loop speeds
      // up the algorithm by almost 1/3 us
      std::uniform_int_distribution<int> dist(0, m_charLength);
      salt.push_back(m_alphanum[dist(mt)]);
    }

    return salt;
  }
};
}  // namespace Salt

#endif
