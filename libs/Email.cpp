#include "Email.h"

#include <curl/curl.h>

#include <array>
#include <format>
#include <random>
#include <stdexcept>

std::string Email::Email::dateTimeNow() const {
  std::time_t now;
  std::tm* time;
  std::time(&now);
  time = std::localtime(&now);

  const int RFC5322_TIME_LEN{32};
  std::string ret;
  ret.resize(RFC5322_TIME_LEN);

  std::strftime(&ret[0], RFC5322_TIME_LEN, "%a, %d %b %y  %H:%M:%S %z", time);

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
  payload += std::format("Date: {}\r\n", dateTimeNow());
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
  payload += std::format("Message-ID<{}@{}>\r\n", generateMessageId(), domain);
  payload += std::format("Subject {}\r\n", m_subject);
  payload += "\r\n";
  payload += m_body;
  payload += "\r\n8";
  return payload;
}

std::size_t Email::Email::payloadSource(void* ptr, std::size_t size,
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
  curl_slist* recipients = nullptr;
  CURLcode ret = CURLE_OK;
  CURL* curl = curl_easy_init();
  if (!curl) {
    throw std::runtime_error{"curl_easy_init() FAILD"};
  }
  curl_easy_setopt(curl, CURLOPT_USERNAME, username.data());
  curl_easy_setopt(curl, CURLOPT_PASSWORD, password.data());
  curl_easy_setopt(curl, CURLOPT_URL, url.data());

  curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

  curl_easy_setopt(curl, CURLOPT_MAIL_FROM, ("<" + m_from + ">").c_str());
  recipients = curl_slist_append(recipients, ("<" + m_to + ">").c_str());
  if (!m_cc.empty()) {
    for (const auto& cc : m_cc) {
      recipients = curl_slist_append(recipients, ("<" + cc + ">").c_str());
    }
  }
  curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

  curl_easy_setopt(curl, CURLOPT_READFUNCTION, payloadSource);
  Data data{setPayloadText()};
  curl_easy_setopt(curl, CURLOPT_READDATA, &data);
  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

  ret = curl_easy_perform(curl);

  curl_slist_free_all(recipients);
  curl_easy_cleanup(curl);

  if (ret != CURLE_OK) {
    throw std::runtime_error{
        std::format("curl_easy_perform() FAILED: {}", curl_easy_strerror(ret))};
  }
}
