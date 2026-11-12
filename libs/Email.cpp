#include "Email.h"

#include <curl/curl.h>

#include <array>
#include <format>
#include <memory>
#include <random>
#include <stdexcept>
#include <string_view>

// Default Real Curl Client implementation for production use
class RealEmailClient : public Email::IEmailClient {
 public:
  CURLcode performSend(const std::string& url, const std::string& username,
                       const std::string& password, const std::string& mailFrom,
                       const std::vector<std::string>& recipients,
                       curl_read_callback readCallback, void* userp) override {
    CURL* curl = curl_easy_init();
    if (!curl) return CURLE_FAILED_INIT;

    curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, mailFrom.c_str());

    curl_slist* rcpts = nullptr;
    for (const auto& r : recipients) {
      rcpts = curl_slist_append(rcpts, r.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, rcpts);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, readCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, userp);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(rcpts);
    curl_easy_cleanup(curl);
    return res;
  }
};  // RealEmailClient

// Inside Email constructor implementation:
Email::Email::Email(std::string_view to, std::string_view from,
                    std::string_view nameFrom, std::string_view subject,
                    std::string_view body,
                    std::optional<std::vector<std::string>> cc,
                    std::function<DT::Date()> dateProvider,
                    std::function<std::string()> idProvider,
                    std::shared_ptr<IEmailClient> curlClient)
    : m_to{to},
      m_from{from},
      m_nameFrom{nameFrom},
      m_subject{subject},
      m_body{body},
      m_cc{cc.value_or(std::vector<std::string>())},
      m_dateProvider{dateProvider ? std::move(dateProvider)
                                  : []() { return DT::getCurrentTimeLT(); }},
      m_idProvider{idProvider ? std::move(idProvider)
                              : [this]() { return generateMessageId(); }},
      m_curlClient{curlClient ? std::move(curlClient)
                              : std::make_shared<RealEmailClient>()} {}

std::string Email::Email::dateTimeNow() const {
  std::time_t now;
  std::tm* time;
  std::time(&now);
  time = std::localtime(&now);

  const int RFC5322_TIME_LEN{32};
  std::string ret;
  ret.resize(RFC5322_TIME_LEN);

  // std::strftime returns the number of characters written (excluding the null
  // terminator)
  size_t len =
      std::strftime(&ret[0], ret.size(), "%a, %d %b %Y %H:%M:%S %z", time);

  // Resize the string down to the actual number of characters written
  ret.resize(len);
  return ret;
}

std::string Email::Email::generateMessageId() const {
  std::time_t now;
  std::time(&now);
  std::tm* time;
  time = std::localtime(&now);

  std::string ret;
  ret.resize(16);

  std::strftime(&ret[0], 16, "%Y%m%d%H%M%S.", time);

  static const char alphanum[] =
      "01234567890"
      "abcdefghijklmnopqrstuvexyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const int strLen{std::size(alphanum) - 1};

  const int MESSAGE_ID_LEN{37};
  ret.reserve(MESSAGE_ID_LEN);

  std::random_device rd;
  std::mt19937 mt(rd());
  while (ret.size() < MESSAGE_ID_LEN) {
    std::uniform_int_distribution<int> dist(0, strLen - 1);
    ret += alphanum[dist(mt)];
  }
  return ret;
}

std::string Email::Email::setPayloadText() {
  std::string payload;
  payload.reserve(1024 + m_body.size() + (m_cc.size() * 30));
  // Use the injected date provider, calling getStr() or streaming it
  // Use the injected date provider and format as RFC 5322 with weekday
  DT::Date currentDate = m_dateProvider();
  payload += std::format("Date: {}\r\n", currentDate.toStringWithDay());

  payload += std::format("To: <{}>\r\n", m_to);
  payload += std::format("From: <{}>({})\r\n", m_from, m_nameFrom);
  if (!m_cc.empty()) {
    payload += "Cc: ";
    bool first = true;
    for (const auto& recipient : m_cc) {
      if (!first) {
        payload += ", ";
      }
      payload += recipient;
      first = false;
    }
    payload += "\r\n";
  } else {
    payload += "Cc:\r\n";
  }
  // let std::format handle the '@' concatenation natively
  std::size_t atPos = m_from.find('@');
  std::string_view domain = (atPos != std::string::npos)
                                ? std::string_view(m_from).substr(atPos + 1)
                                : "unknown;com";

  // Added missing colon and space for Message-ID and Subject headers
  payload +=
      std::format("Message-ID: <{}@{}>\r\n", generateMessageId(), domain);
  payload += std::format("Subject: {}\r\n", m_subject);

  payload += "\r\n";
  payload += m_body;
  payload += "\r\n8";
  return payload;
}

std::size_t Email::Email::payloadSource(char* ptr, std::size_t size,
                                        std::size_t nmemb, void* userp) {
  auto* data = static_cast<Data*>(userp);
  // check fir invalid arguments or if transfer is already finished
  const std::size_t bufferCapacity = size * nmemb;
  if (bufferCapacity == 0 || data == nullptr || data->bytesleft == 0) {
    return 0;
  }
  // calculste exactly how much data can he copied in this crunch
  const std::size_t bytesToCopy = std::min(bufferCapacity, data->bytesleft);

  // copy efficiently into libcurl's biffer
  // use the counter to calculate our exact position in the string
  //(Full Lengnth - Remaining Bytes = Current StaertingPosition
  const std::size_t offset = data->msg.size() - data->bytesleft;

  // copy the chunk starting at the offset
  data->msg.copy(reinterpret_cast<char*>(ptr), bytesToCopy, offset);

  data->bytesleft -= bytesToCopy;

  return bytesToCopy;
}

void Email::Email::send(std::string_view url, std::string_view username,
                        std::string_view password) {
  // Delegate to the injected curl client (which defaults to RealEmailClient in
  // production)
  Data data{setPayloadText()};

  // Build recipients list for the client
  std::vector<std::string> recipients;
  recipients.push_back("<" + m_to + ">");
  if (!m_cc.empty()) {
    for (const auto& cc : m_cc) {
      recipients.push_back("<" + cc + ">");
    }
  }

  std::string mailFrom = "<" + m_from + ">";

  CURLcode ret = m_curlClient->performSend(
      std::string(url), std::string(username), std::string(password), mailFrom,
      recipients, payloadSource, &data);

  if (ret != CURLE_OK) {
    throw std::runtime_error{
        std::format("curl_easy_perform() FAILED: {}", curl_easy_strerror(ret))};
  }
}
