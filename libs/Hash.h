// this is a function object that takes a string
// and returns a hash of it. uses the Cryptopp library and SHA512

#ifndef HASH_H
#define HASH_H

#include <cstring>
#include <string>

#include "cryptopp/sha.h"

namespace Hash {
class Hash {
 private:
  CryptoPP::SHA512 m_sha;
  unsigned char* m_buffer;
  unsigned char* m_inputBytes;
  char* m_hashedBytes;

 public:
  std::string operator()(const std::string& input) {
    m_buffer = new unsigned char[m_sha.DigestSize()];
    m_inputBytes = new unsigned char[input.size()];
    std::memcpy(m_inputBytes, input.data(), input.size());
    m_sha.CalculateDigest(m_buffer, m_inputBytes, input.size());
    m_hashedBytes = new char[m_sha.DigestSize()];
    std::memcpy(m_hashedBytes, m_buffer, m_sha.DigestSize());
    return std::string(m_hashedBytes, m_sha.DigestSize());
  }
  ~Hash() {
    delete[] m_buffer;
    delete[] m_inputBytes;
    delete[] m_hashedBytes;
  }
};
}  // namespace Hash
#endif
