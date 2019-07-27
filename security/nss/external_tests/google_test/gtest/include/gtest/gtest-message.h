












































#ifndef GTEST_INCLUDE_GTEST_GTEST_MESSAGE_H_
#define GTEST_INCLUDE_GTEST_GTEST_MESSAGE_H_

#include <limits>

#include "gtest/internal/gtest-port.h"



void operator<<(const testing::internal::Secret&, int);

namespace testing {



























class GTEST_API_ Message {
 private:
  
  
  typedef std::ostream& (*BasicNarrowIoManip)(std::ostream&);

 public:
  
  Message();

  
  Message(const Message& msg) : ss_(new ::std::stringstream) {  
    *ss_ << msg.GetString();
  }

  
  explicit Message(const char* str) : ss_(new ::std::stringstream) {
    *ss_ << str;
  }

#if GTEST_OS_SYMBIAN
  
  template <typename T>
  inline Message& operator <<(const T& value) {
    StreamHelper(typename internal::is_pointer<T>::type(), value);
    return *this;
  }
#else
  
  template <typename T>
  inline Message& operator <<(const T& val) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    using ::operator <<;
    *ss_ << val;
    return *this;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  template <typename T>
  inline Message& operator <<(T* const& pointer) {  
    if (pointer == NULL) {
      *ss_ << "(null)";
    } else {
      *ss_ << pointer;
    }
    return *this;
  }
#endif  

  
  
  
  
  
  
  Message& operator <<(BasicNarrowIoManip val) {
    *ss_ << val;
    return *this;
  }

  
  Message& operator <<(bool b) {
    return *this << (b ? "true" : "false");
  }

  
  
  Message& operator <<(const wchar_t* wide_c_str);
  Message& operator <<(wchar_t* wide_c_str);

#if GTEST_HAS_STD_WSTRING
  
  
  Message& operator <<(const ::std::wstring& wstr);
#endif  

#if GTEST_HAS_GLOBAL_WSTRING
  
  
  Message& operator <<(const ::wstring& wstr);
#endif  

  
  
  
  
  std::string GetString() const;

 private:

#if GTEST_OS_SYMBIAN
  
  
  
  
  template <typename T>
  inline void StreamHelper(internal::true_type , T* pointer) {
    if (pointer == NULL) {
      *ss_ << "(null)";
    } else {
      *ss_ << pointer;
    }
  }
  template <typename T>
  inline void StreamHelper(internal::false_type ,
                           const T& value) {
    
    
    using ::operator <<;
    *ss_ << value;
  }
#endif  

  
  const internal::scoped_ptr< ::std::stringstream> ss_;

  
  
  void operator=(const Message&);
};


inline std::ostream& operator <<(std::ostream& os, const Message& sb) {
  return os << sb.GetString();
}

namespace internal {





template <typename T>
std::string StreamableToString(const T& streamable) {
  return (Message() << streamable).GetString();
}

}  
}  

#endif  
