#ifndef HASH_H
#define HASH_H

#include <string>

#include "cryptopp/sha.h"

namespace Hash {
class Hash {
 private:
  CryptoPP::SHA512 m_sha;

 public:
  std::string operator()(const std::string& input) {
    // 1. Allocate the output buffer on the stack (SHA-512 digest is always 64
    // bytes)
    unsigned char buffer[CryptoPP::SHA512::DIGESTSIZE];

    // 2. Calculate the digest directly using the input string's data pointer
    m_sha.CalculateDigest(buffer,
                          reinterpret_cast<const unsigned char*>(input.data()),
                          input.size());

    // 3. Return a std::string constructed directly from the stack buffer
    return std::string(reinterpret_cast<char*>(buffer),
                       CryptoPP::SHA512::DIGESTSIZE);
  }

  // Destructor is no longer needed! RAII handles everything automatically.
};
}  // namespace Hash
#endif
