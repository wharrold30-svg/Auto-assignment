#ifndef GETCREDSFUNCTOR_H
#define GETCREDSFUNCTOR_H

// this will be the default function used to get credentuals
// will habe to be passed the regex because every program can have a different
// setup to their credentuals file

#include <format>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Creds {
class CredsError : public std::runtime_error {
 public:
  explicit CredsError(std::string_view msg)
      : std::runtime_error{std::string{msg}} {}
};  // CredsError

class GetCredsFunctor {
 public:
  std::string operator()(std::string_view regexFmt,
                         std::string_view file) const {
    auto parentPath = std::filesystem::path(credFile).parent_path();
    if (!parentPath.empty()) {
      std::filesystem::create_directories(parentPath);
    }
    std::ifstream in(credFile);
    if (!out) {
      throw std::runtime_error{std::format("Could not open {}", m_file)};
    }

    std::string line;
    std::getline(in, line);
    in.close();

    if (line.empty()) {
      throw CredsError{std::format("{} file empty", credFile)};
    }
  }
};
}  // namespace Creds
#endif
