









#ifndef WEBRTC_BASE_CHECKS_H_
#define WEBRTC_BASE_CHECKS_H_

#include <sstream>
#include <string>

#ifdef WEBRTC_CHROMIUM_BUILD




#include "webrtc/base/logging.h"
#endif
#include "webrtc/typedefs.h"





































namespace rtc {




#ifndef WEBRTC_CHROMIUM_BUILD



#define LAZY_STREAM(stream, condition)                                        \
  !(condition) ? static_cast<void>(0) : rtc::FatalMessageVoidify() & (stream)


#define EAT_STREAM_PARAMETERS                                           \
  true ? static_cast<void>(0)                                           \
       : rtc::FatalMessageVoidify() & rtc::FatalMessage("", 0).stream()







#define CHECK(condition)                                                    \
  LAZY_STREAM(rtc::FatalMessage(__FILE__, __LINE__).stream(), !(condition)) \
  << "Check failed: " #condition << std::endl << "# "






#define CHECK_OP(name, op, val1, val2)                      \
  if (std::string* _result =                                \
      rtc::Check##name##Impl((val1), (val2),                \
                             #val1 " " #op " " #val2))      \
    rtc::FatalMessage(__FILE__, __LINE__, _result).stream()





template<class t1, class t2>
std::string* MakeCheckOpString(const t1& v1, const t2& v2, const char* names) {
  std::ostringstream ss;
  ss << names << " (" << v1 << " vs. " << v2 << ")";
  std::string* msg = new std::string(ss.str());
  return msg;
}


#if !defined(COMPILER_MSVC)


extern template std::string* MakeCheckOpString<int, int>(
    const int&, const int&, const char* names);
extern template
std::string* MakeCheckOpString<unsigned long, unsigned long>(
    const unsigned long&, const unsigned long&, const char* names);
extern template
std::string* MakeCheckOpString<unsigned long, unsigned int>(
    const unsigned long&, const unsigned int&, const char* names);
extern template
std::string* MakeCheckOpString<unsigned int, unsigned long>(
    const unsigned int&, const unsigned long&, const char* names);
extern template
std::string* MakeCheckOpString<std::string, std::string>(
    const std::string&, const std::string&, const char* name);
#endif





#define DEFINE_CHECK_OP_IMPL(name, op) \
  template <class t1, class t2> \
  inline std::string* Check##name##Impl(const t1& v1, const t2& v2, \
                                        const char* names) { \
    if (v1 op v2) return NULL; \
    else return rtc::MakeCheckOpString(v1, v2, names); \
  } \
  inline std::string* Check##name##Impl(int v1, int v2, const char* names) { \
    if (v1 op v2) return NULL; \
    else return rtc::MakeCheckOpString(v1, v2, names); \
  }
DEFINE_CHECK_OP_IMPL(EQ, ==)
DEFINE_CHECK_OP_IMPL(NE, !=)
DEFINE_CHECK_OP_IMPL(LE, <=)
DEFINE_CHECK_OP_IMPL(LT, < )
DEFINE_CHECK_OP_IMPL(GE, >=)
DEFINE_CHECK_OP_IMPL(GT, > )
#undef DEFINE_CHECK_OP_IMPL

#define CHECK_EQ(val1, val2) CHECK_OP(EQ, ==, val1, val2)
#define CHECK_NE(val1, val2) CHECK_OP(NE, !=, val1, val2)
#define CHECK_LE(val1, val2) CHECK_OP(LE, <=, val1, val2)
#define CHECK_LT(val1, val2) CHECK_OP(LT, < , val1, val2)
#define CHECK_GE(val1, val2) CHECK_OP(GE, >=, val1, val2)
#define CHECK_GT(val1, val2) CHECK_OP(GT, > , val1, val2)



#if (!defined(NDEBUG) || defined(DCHECK_ALWAYS_ON))
#define DCHECK(condition) CHECK(condition)
#define DCHECK_EQ(v1, v2) CHECK_EQ(v1, v2)
#define DCHECK_NE(v1, v2) CHECK_NE(v1, v2)
#define DCHECK_LE(v1, v2) CHECK_LE(v1, v2)
#define DCHECK_LT(v1, v2) CHECK_LT(v1, v2)
#define DCHECK_GE(v1, v2) CHECK_GE(v1, v2)
#define DCHECK_GT(v1, v2) CHECK_GT(v1, v2)
#else
#define DCHECK(condition) EAT_STREAM_PARAMETERS
#define DCHECK_EQ(v1, v2) EAT_STREAM_PARAMETERS
#define DCHECK_NE(v1, v2) EAT_STREAM_PARAMETERS
#define DCHECK_LE(v1, v2) EAT_STREAM_PARAMETERS
#define DCHECK_LT(v1, v2) EAT_STREAM_PARAMETERS
#define DCHECK_GE(v1, v2) EAT_STREAM_PARAMETERS
#define DCHECK_GT(v1, v2) EAT_STREAM_PARAMETERS
#endif


class FatalMessageVoidify {
 public:
  FatalMessageVoidify() { }
  
  
  void operator&(std::ostream&) { }
};

#endif  

#define FATAL() rtc::FatalMessage(__FILE__, __LINE__).stream()





class FatalMessage {
 public:
  FatalMessage(const char* file, int line);
  
  FatalMessage(const char* file, int line, std::string* result);
  NO_RETURN ~FatalMessage();

  std::ostream& stream() { return stream_; }

 private:
  void Init(const char* file, int line);

  std::ostringstream stream_;
};

}  

#endif  
