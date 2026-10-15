#ifndef GETCREDSFUNCTOR_H
#define GETCREDSFUNCTOR_H

#include <filesystem>
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
    auto parentPath = std::filesystem::path(file).parent_path();
    if (!parentPath.empty()) {
      std::filesystem::create_directories(parentPath);
    }

    std::ifstream in(string_data_or_file(file));  // using file string
    // Fixed: changed 'credFile' to 'file', 'out' to 'in', and 'm_file' to
    // 'file' (Standardizing on `file` parameter)
    std::ifstream fileStream{std::string(file)};
    if (!fileStream.is_open()) {
      throw std::runtime_error{std::format("Could not open {}", file)};
    }

    std::string line;
    std::getline(fileStream, line);
    fileStream.close();

    if (line.empty()) {
      throw CredsError{std::format("{} file empty", file)};
    }

    // extract the creds using the regexFmt
    std::regex r{std::string(regexFmt)};
    std::smatch result;
    if (std::regex_match(line, result, r)) {
      if (result.size() > 1) {
        return result[1];
      }
    }

    throw CredsError{std::format("Invalid formatted regex string: {}", file)};
  }

 private:
  // Helper just to keep compatibility if needed, using string_view cleanly
  std::string string_data_or_file(std::string_view f) const {
    return std::string(f);
  }
};  // GetCredsFunctor

}  // namespace Creds
#endif
