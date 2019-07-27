









#ifndef WEBRTC_BASE_STRINGENCODE_H_
#define WEBRTC_BASE_STRINGENCODE_H_

#include <string>
#include <sstream>
#include <vector>

#include "webrtc/base/checks.h"

namespace rtc {







size_t utf8_encode(char* buffer, size_t buflen, unsigned long value);


size_t utf8_decode(const char* source, size_t srclen, unsigned long* value);



size_t escape(char * buffer, size_t buflen,
              const char * source, size_t srclen,
              const char * illegal, char escape);

size_t unescape(char * buffer, size_t buflen,
                const char * source, size_t srclen,
                char escape);





size_t encode(char * buffer, size_t buflen,
              const char * source, size_t srclen,
              const char * illegal, char escape);

size_t decode(char * buffer, size_t buflen,
              const char * source, size_t srclen,
              char escape);



const char* unsafe_filename_characters();



size_t url_encode(char * buffer, size_t buflen,
                  const char * source, size_t srclen);

size_t url_decode(char * buffer, size_t buflen,
                  const char * source, size_t srclen);


size_t html_encode(char * buffer, size_t buflen,
                   const char * source, size_t srclen);

size_t html_decode(char * buffer, size_t buflen,
                   const char * source, size_t srclen);


size_t xml_encode(char * buffer, size_t buflen,
                  const char * source, size_t srclen);

size_t xml_decode(char * buffer, size_t buflen,
                  const char * source, size_t srclen);


char hex_encode(unsigned char val);

bool hex_decode(char ch, unsigned char* val);


size_t hex_encode(char* buffer, size_t buflen,
                  const char* source, size_t srclen);




size_t hex_encode_with_delimiter(char* buffer, size_t buflen,
                                 const char* source, size_t srclen,
                                 char delimiter);


std::string hex_encode(const char* source, size_t srclen);
std::string hex_encode_with_delimiter(const char* source, size_t srclen,
                                      char delimiter);


size_t hex_decode(char* buffer, size_t buflen,
                  const char* source, size_t srclen);





size_t hex_decode_with_delimiter(char* buffer, size_t buflen,
                                 const char* source, size_t srclen,
                                 char delimiter);


size_t hex_decode(char* buffer, size_t buflen, const std::string& source);
size_t hex_decode_with_delimiter(char* buffer, size_t buflen,
                                 const std::string& source, char delimiter);




typedef size_t (*Transform)(char * buffer, size_t buflen,
                            const char * source, size_t srclen);
size_t transform(std::string& value, size_t maxlen, const std::string& source,
                 Transform t);


std::string s_transform(const std::string& source, Transform t);


inline std::string s_url_encode(const std::string& source) {
  return s_transform(source, url_encode);
}
inline std::string s_url_decode(const std::string& source) {
  return s_transform(source, url_decode);
}



size_t split(const std::string& source, char delimiter,
             std::vector<std::string>* fields);



size_t tokenize(const std::string& source, char delimiter,
                std::vector<std::string>* fields);


size_t tokenize_append(const std::string& source, char delimiter,
                       std::vector<std::string>* fields);








size_t tokenize(const std::string& source, char delimiter, char start_mark,
                char end_mark, std::vector<std::string>* fields);







template <class T>
static bool ToString(const T &t, std::string* s) {
  DCHECK(s);
  std::ostringstream oss;
  oss << std::boolalpha << t;
  *s = oss.str();
  return !oss.fail();
}

template <class T>
static bool FromString(const std::string& s, T* t) {
  DCHECK(t);
  std::istringstream iss(s);
  iss >> std::boolalpha >> *t;
  return !iss.fail();
}



template<typename T>
static inline std::string ToString(const T& val) {
  std::string str; ToString(val, &str); return str;
}

template<typename T>
static inline T FromString(const std::string& str) {
  T val; FromString(str, &val); return val;
}

template<typename T>
static inline T FromString(const T& defaultValue, const std::string& str) {
  T val(defaultValue); FromString(str, &val); return val;
}



char make_char_safe_for_filename(char c);



}  

#endif  
