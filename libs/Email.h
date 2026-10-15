#ifndef EMAIL_H
#define EMAIL_H

#include <curl/curl.h>

#include <ctime>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Email {
class Email {
 private:
  class Data {
   public:
    std::string msg;
    std::size_t bytesleft;
    Data(std::string&& str) : msg{std::move(str)}, bytesleft{msg.size()} {}
    Data(std::string&) = delete;
  };  // Data

  std::string m_to;
  std::string m_from;
  std::string m_nameFrom;
  std::string m_subject;
  std::string m_body;
  std::vector<std::string> m_cc;

  std::string setPayloadText();
  std::string generateMessageId() const;
  static std::size_t payloadSource(void* ptr, std::size_t size,
                                   std::size_t nmemb, void* userp);

 public:
  Email(std::string_view to, std::string_view from, std::string_view nameFrom,
        std::string_view subject, std::string_view body,
        std::optional<std::vector<std::string>> cc = std::nullopt)
      : m_to{to},
        m_from{from},
        m_nameFrom{nameFrom},
        m_subject{subject},
        m_body{body},
        m_cc{cc.value_or(std::vector<std::string>())} {}

  Email(const Email&) = delete;
  Email& operator=(const Email&) = delete;
  Email(const Email&&) = delete;
  Email& operator=(const Email&&) = delete;

  std::string dateTimeNow() const;
  void send(std::string_view url, std::string_view username,
            std::string_view password);
};  // Email
}  // namespace Email

#endif
