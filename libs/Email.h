#ifndef EMAIL_H
#define EMAIL_H

#include <curl/curl.h>

#include <ctime>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Date.h"

namespace Email {
class IEmailClient {
 public:
  virtual ~IEmailClient() = default;
  virtual CURLcode performSend(const std::string& url,
                               const std::string& username,
                               const std::string& password,
                               const std::string& mailFrom,
                               const std::vector<std::string>& recipients,
                               curl_read_callback readCallback,
                               void* userp) = 0;
};
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

  // Injected dependencies
  std::function<DT::Date()> m_dateProvider;
  std::function<std::string()> m_idProvider;
  std::shared_ptr<IEmailClient> m_curlClient;

 protected:
  std::string setPayloadText();
  std::string generateMessageId() const;
  static std::size_t payloadSource(char* ptr, std::size_t size,
                                   std::size_t nmemb, void* userp);

 public:
  // Constructor with dependency injection defaults
  Email(std::string_view to, std::string_view from, std::string_view nameFrom,
        std::string_view subject, std::string_view body,
        std::optional<std::vector<std::string>> cc = std::nullopt,
        std::function<DT::Date()> dateProvider = nullptr,
        std::function<std::string()> idProvider = nullptr,
        std::shared_ptr<IEmailClient> curlClient = nullptr);

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
