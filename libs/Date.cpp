//Date implementation

#include"Date.h"

#include<regex>
#include<utility>
#include<sstream>

#include<compare>
#include<iostream>
#include<string>
#include<string_view>
#include<stdexcept>
#include<ctime>
#include"include/nlohmann/json.hpp"


void DT::Date::swap(DT::Date& otherDate) noexcept {
    std::swap(m_date.tm_mon, otherDate.m_date.tm_mon);
    std::swap(m_date.tm_mday, otherDate.m_date.tm_mday);
    std::swap(m_date.tm_year, otherDate.m_date.tm_year);
    std::swap(m_date.tm_hour, otherDate.m_date.tm_hour);
    std::swap(m_date.tm_min, otherDate.m_date.tm_min);
    std::swap(m_date.tm_sec, otherDate.m_date.tm_sec);
    std::swap(m_date.tm_isdst, otherDate.m_date.tm_isdst);
    std::swap(m_isLT, otherDate.m_isLT);
}
void DT::swap(DT::Date& one, DT::Date& other) noexcept {
    one.swap(other);
}

bool DT::Date::validateDateTime(std::string_view str) const {
  //  std::regex r("^\\d{4}-\\d{2}-\\d{2}\\s\\d{2}:\\d{2}:\\d{2}$");
  // The (\\.\\d+)? at the end allows for optional fractional seconds (.123456)
std::regex r(R"(^\d{4}-\d{2}-\d{2}\s\d{2}:\d{2}:\d{2}$)");

    std::string s(str);

    return std::regex_match(s, r);
/*
// 1. Use a raw string literal R"(...)" to avoid double backslashes
    // 2. Fix the spacing between the date and time patterns
    static const std::regex r(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})");

    // 3. Match using string_view iterators to avoid string copying/allocation
    return std::regex_match(str.begin(), str.end(), r);
    */
}


void DT::Date::convertToGmt(){
    std::time_t time = std::mktime(&m_date);
    m_date = *std::gmtime(&time);
    m_isLT = false;
}

void DT::Date::convertToLT(){
    std::time_t time = std::mktime(&m_date);
    m_date = *std::localtime(&time);
    m_isLT = true;
}

void DT::Date::setTM(std::string_view str){
    if(str == " "){
        std::time_t now = std::time(0);
        m_date = *std::gmtime(&now);
        m_isLT = false;
        return;
    }
    if(!validateDateTime(str)){
        throw DateError("Incorret Date format");
    }

    auto assignFunction = [this](const std::smatch& m){
        m_date.tm_mon = std::stoi(m[2].str()) - 1;
        m_date.tm_mday = std::stoi(m[3].str());
        m_date.tm_year = std::stoi(m[1].str()) - 1900;
        m_date.tm_hour = std::stoi(m[4].str());
        m_date.tm_min = std::stoi(m[5].str());
        m_date.tm_sec = std::stoi(m[6].str());
    };

    std::regex r("(\\d{4})-(\\d{2})-(\\d{2}) "
            "(\\d{2}):(\\d{2}):(\\d{2})");
    std::smatch m;
    std::string s(str);
    std::regex_match(s, m, r);
    assignFunction(m);

    normalize();

    convertToGmt();
}

DT::Date::Date() = default;

DT::Date::Date(const std::tm& date)
    :m_date(date), m_isLT(false){
        convertToGmt();
}

DT::Date::Date(std::string_view date){
    setTM(date);
}

DT::Date::Date(const DT::Date& otherDate) : DT::Date(otherDate.m_date){}

DT::Date& DT::Date::operator=(const DT::Date& rhs){
    if(&rhs != this){
        DT::Date copy(rhs);
        using std::swap;
        swap(*this, copy);
        normalize();
    }
    return *this;
}


std::string DT::Date::getStr() const {
    std::string ret;
    const int LEN(32);
    ret.resize(LEN);
    const int newSize = std::strftime(&ret[0], LEN, "%x %I:%M%p %z", &m_date);
    ret.resize(newSize);
    return std::move(ret);
}
std::string DT::Date::getDBStr() const {
    std::string ret;
    const int LEN(32);
    ret.resize(LEN);
    const int newSize = std::strftime(&ret[0], LEN, "%Y-%m-%d %H:%M:%S", &m_date);
    ret.resize(newSize);
    return std::move(ret);
}
bool DT::Date::empty() const {
    return m_date.tm_mon == 0
        && m_date.tm_mday == 0
        && m_date.tm_year== 0
        && m_date.tm_hour== 0
        && m_date.tm_min == 0
        && m_date.tm_sec == 0;
}

void DT::Date::clear(){

    m_date.tm_mon = 0;
    m_date.tm_mday = 0;
    m_date.tm_year = 0;
    m_date.tm_hour = 0;
    m_date.tm_min = 0;
    m_date.tm_sec = 0;
}

DT::Date::operator std::string() const {
    return getStr();
}

std::ostream& operator <<(std::ostream& os, const DT::Date& date){
    os << date.getStr();
    return os;
}

std::time_t getCurrentTime(){
    return std::time(nullptr);
}

DT::Date DT::getCurrentTimeGMT(){
    std::time_t now = getCurrentTime();
    std::tm ct = *std::gmtime(&now);
    DT::Date ret{ct};

    return static_cast<const DT::Date&>(ret);

}


DT::Date DT::getCurrentTimeLT(){
    std::time_t now = getCurrentTime();
    std::tm ct = *std::localtime(&now);
    DT::Date ret{ct};
    return static_cast<const DT::Date&>(ret);
}
