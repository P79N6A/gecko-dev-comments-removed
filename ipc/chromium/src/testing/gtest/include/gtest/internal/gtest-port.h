


































#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PORT_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PORT_H_
















































































































#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <iostream>  

#define GTEST_DEV_EMAIL_ "googletestframework@@googlegroups.com"
#define GTEST_FLAG_PREFIX_ "gtest_"
#define GTEST_FLAG_PREFIX_UPPER_ "GTEST_"
#define GTEST_NAME_ "Google Test"
#define GTEST_PROJECT_URL_ "http://code.google.com/p/googletest/"


#ifdef __GNUC__

#define GTEST_GCC_VER_ \
    (__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__)
#endif  


#ifdef __CYGWIN__
#define GTEST_OS_CYGWIN 1
#elif __SYMBIAN32__
#define GTEST_OS_SYMBIAN 1
#elif defined _WIN32
#define GTEST_OS_WINDOWS 1
#elif defined __APPLE__
#define GTEST_OS_MAC 1
#elif defined __linux__
#define GTEST_OS_LINUX 1
#elif defined __MVS__
#define GTEST_OS_ZOS 1
#elif defined(__sun) && defined(__SVR4)
#define GTEST_OS_SOLARIS 1
#endif  

#if GTEST_OS_CYGWIN || GTEST_OS_LINUX || GTEST_OS_MAC





#include <regex.h>  
#include <strings.h>  
#include <sys/types.h>  
#include <unistd.h>  

#define GTEST_USES_POSIX_RE 1

#elif GTEST_OS_WINDOWS

#include <direct.h>  
#include <io.h>  



#define GTEST_USES_SIMPLE_RE 1

#else



#define GTEST_USES_SIMPLE_RE 1

#endif  




#ifdef _MSC_VER  

#ifndef _HAS_EXCEPTIONS  
#define _HAS_EXCEPTIONS 1
#endif  
#define GTEST_HAS_EXCEPTIONS _HAS_EXCEPTIONS
#else  



#if defined(__GNUC__) && __EXCEPTIONS
#define GTEST_HAS_EXCEPTIONS 1
#else
#define GTEST_HAS_EXCEPTIONS 0
#endif  
#endif  



#ifndef GTEST_HAS_STD_STRING




#if defined(_MSC_VER) && (_MSC_VER < 1400) && !GTEST_HAS_EXCEPTIONS
#define GTEST_HAS_STD_STRING 0
#else
#define GTEST_HAS_STD_STRING 1
#endif
#endif  

#ifndef GTEST_HAS_GLOBAL_STRING



#define GTEST_HAS_GLOBAL_STRING 0

#endif  

#ifndef GTEST_HAS_STD_WSTRING





#if GTEST_OS_CYGWIN || GTEST_OS_SOLARIS



#define GTEST_HAS_STD_WSTRING 0
#else
#define GTEST_HAS_STD_WSTRING GTEST_HAS_STD_STRING
#endif  

#endif  

#ifndef GTEST_HAS_GLOBAL_WSTRING


#define GTEST_HAS_GLOBAL_WSTRING \
    (GTEST_HAS_STD_WSTRING && GTEST_HAS_GLOBAL_STRING)
#endif  

#if GTEST_HAS_STD_STRING || GTEST_HAS_GLOBAL_STRING || \
    GTEST_HAS_STD_WSTRING || GTEST_HAS_GLOBAL_WSTRING
#include <string>  
#endif  
        

#if GTEST_HAS_STD_STRING
#include <sstream>  
#else
#include <strstream>  
#endif  


#ifndef GTEST_HAS_RTTI



#ifdef _MSC_VER

#ifdef _CPPRTTI  
#define GTEST_HAS_RTTI 1
#else
#define GTEST_HAS_RTTI 0
#endif  

#elif defined(__GNUC__)


#if GTEST_GCC_VER_ >= 40302
#ifdef __GXX_RTTI
#define GTEST_HAS_RTTI 1
#else
#define GTEST_HAS_RTTI 0
#endif  
#else

#define GTEST_HAS_RTTI 1
#endif  

#else


#define GTEST_HAS_RTTI 1

#endif  

#endif  


#ifndef GTEST_HAS_PTHREAD

#define GTEST_HAS_PTHREAD (GTEST_OS_LINUX || GTEST_OS_MAC)
#endif  






#ifndef GTEST_HAS_TR1_TUPLE



#if defined(__GNUC__) && (GTEST_GCC_VER_ >= 40000)
#define GTEST_HAS_TR1_TUPLE 1
#else
#define GTEST_HAS_TR1_TUPLE 0
#endif  
#endif  




#if GTEST_HAS_TR1_TUPLE
#if defined(__GNUC__)


#include <tr1/tuple>
#else


#include <tuple>
#endif  
#endif  





#ifndef GTEST_HAS_CLONE


#if GTEST_OS_LINUX && !defined(__ia64__)
#define GTEST_HAS_CLONE 1
#else
#define GTEST_HAS_CLONE 0
#endif  

#endif  









#if GTEST_HAS_STD_STRING && (GTEST_OS_LINUX || \
                             GTEST_OS_MAC || \
                             GTEST_OS_CYGWIN || \
                             (GTEST_OS_WINDOWS && _MSC_VER >= 1400))
#define GTEST_HAS_DEATH_TEST 1
#include <vector>  
#endif



#if defined(__GNUC__) || (_MSC_VER >= 1400)


#define GTEST_HAS_PARAM_TEST 1
#endif  





#if defined(__GNUC__) || (_MSC_VER >= 1400)
#define GTEST_HAS_TYPED_TEST 1
#define GTEST_HAS_TYPED_TEST_P 1
#endif  



#if GTEST_HAS_PARAM_TEST && GTEST_HAS_TR1_TUPLE
#define GTEST_HAS_COMBINE 1
#endif  


#define GTEST_WIDE_STRING_USES_UTF16_ \
    (GTEST_OS_WINDOWS || GTEST_OS_CYGWIN || GTEST_OS_SYMBIAN)











#ifdef __INTEL_COMPILER
#define GTEST_AMBIGUOUS_ELSE_BLOCKER_
#else
#define GTEST_AMBIGUOUS_ELSE_BLOCKER_ switch (0) case 0:  // NOLINT
#endif












#if defined(__GNUC__) && !defined(COMPILER_ICC)
#define GTEST_ATTRIBUTE_UNUSED_ __attribute__ ((unused))
#else
#define GTEST_ATTRIBUTE_UNUSED_
#endif



#define GTEST_DISALLOW_COPY_AND_ASSIGN_(type)\
  type(const type &);\
  void operator=(const type &)






#if defined(__GNUC__) && (GTEST_GCC_VER_ >= 30400) && !defined(COMPILER_ICC)
#define GTEST_MUST_USE_RESULT_ __attribute__ ((warn_unused_result))
#else
#define GTEST_MUST_USE_RESULT_
#endif  

namespace testing {

class Message;

namespace internal {

class String;





#if GTEST_HAS_STD_STRING
typedef ::std::stringstream StrStream;
#else
typedef ::std::strstream StrStream;
#endif  





template <typename T>
class scoped_ptr {
 public:
  explicit scoped_ptr(T* p = NULL) : ptr_(p) {}
  ~scoped_ptr() { reset(); }

  T& operator*() const { return *ptr_; }
  T* operator->() const { return ptr_; }
  T* get() const { return ptr_; }

  T* release() {
    T* const ptr = ptr_;
    ptr_ = NULL;
    return ptr;
  }

  void reset(T* p = NULL) {
    if (p != ptr_) {
      if (sizeof(T) > 0) {  
        delete ptr_;
      }
      ptr_ = p;
    }
  }
 private:
  T* ptr_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(scoped_ptr);
};





class RE {
 public:
  
#if GTEST_HAS_STD_STRING
  RE(const ::std::string& regex) { Init(regex.c_str()); }  
#endif  

#if GTEST_HAS_GLOBAL_STRING
  RE(const ::string& regex) { Init(regex.c_str()); }  
#endif  

  RE(const char* regex) { Init(regex); }  
  ~RE();

  
  const char* pattern() const { return pattern_; }

  
  
  
  
  
  
  
#if GTEST_HAS_STD_STRING
  static bool FullMatch(const ::std::string& str, const RE& re) {
    return FullMatch(str.c_str(), re);
  }
  static bool PartialMatch(const ::std::string& str, const RE& re) {
    return PartialMatch(str.c_str(), re);
  }
#endif  

#if GTEST_HAS_GLOBAL_STRING
  static bool FullMatch(const ::string& str, const RE& re) {
    return FullMatch(str.c_str(), re);
  }
  static bool PartialMatch(const ::string& str, const RE& re) {
    return PartialMatch(str.c_str(), re);
  }
#endif  

  static bool FullMatch(const char* str, const RE& re);
  static bool PartialMatch(const char* str, const RE& re);

 private:
  void Init(const char* regex);

  
  
  
  
  const char* pattern_;
  bool is_valid_;
#if GTEST_USES_POSIX_RE
  regex_t full_regex_;     
  regex_t partial_regex_;  
#else  
  const char* full_pattern_;  
#endif

  GTEST_DISALLOW_COPY_AND_ASSIGN_(RE);
};






enum GTestLogSeverity {
  GTEST_INFO,
  GTEST_WARNING,
  GTEST_ERROR,
  GTEST_FATAL
};

void GTestLog(GTestLogSeverity severity, const char* file,
              int line, const char* msg);

#define GTEST_LOG_(severity, msg)\
    ::testing::internal::GTestLog(\
        ::testing::internal::GTEST_##severity, __FILE__, __LINE__, \
        (::testing::Message() << (msg)).GetString().c_str())

inline void LogToStderr() {}
inline void FlushInfoLog() { fflush(NULL); }





#if GTEST_HAS_STD_STRING
void CaptureStderr();
::std::string GetCapturedStderr();
#endif  

#if GTEST_HAS_DEATH_TEST


extern ::std::vector<String> g_argvs;


const ::std::vector<String>& GetArgvs();

#endif  








class Mutex {
 public:
  Mutex() {}
  explicit Mutex(int ) {}
  void AssertHeld() const {}
  enum { NO_CONSTRUCTOR_NEEDED_FOR_STATIC_MUTEX = 0 };
};




class GTestMutexLock {
 public:
  explicit GTestMutexLock(Mutex*) {}  
};

typedef GTestMutexLock MutexLock;

template <typename T>
class ThreadLocal {
 public:
  ThreadLocal() : value_() {}
  explicit ThreadLocal(const T& value) : value_(value) {}
  T* pointer() { return &value_; }
  const T* pointer() const { return &value_; }
  const T& get() const { return value_; }
  void set(const T& value) { value_ = value; }
 private:
  T value_;
};



size_t GetThreadCount();



#define GTEST_IS_THREADSAFE 0

#if defined(__SYMBIAN32__) || defined(__IBMCPP__)






#define GTEST_ELLIPSIS_NEEDS_COPY_ 1





#define GTEST_NEEDS_IS_POINTER_ 1

#endif  

template <bool bool_value>
struct bool_constant {
  typedef bool_constant<bool_value> type;
  static const bool value = bool_value;
};
template <bool bool_value> const bool bool_constant<bool_value>::value;

typedef bool_constant<false> false_type;
typedef bool_constant<true> true_type;

template <typename T>
struct is_pointer : public false_type {};

template <typename T>
struct is_pointer<T*> : public true_type {};

#if GTEST_OS_WINDOWS
#define GTEST_PATH_SEP_ "\\"

typedef __int64 BiggestInt;
#else
#define GTEST_PATH_SEP_ "/"
typedef long long BiggestInt;  
#endif  







namespace posix {



#if GTEST_OS_WINDOWS

typedef struct _stat StatStruct;

inline int FileNo(FILE* file) { return _fileno(file); }
inline int IsATTY(int fd) { return _isatty(fd); }
inline int Stat(const char* path, StatStruct* buf) { return _stat(path, buf); }
inline int StrCaseCmp(const char* s1, const char* s2) {
  return _stricmp(s1, s2);
}
inline char* StrDup(const char* src) { return _strdup(src); }
inline int RmDir(const char* dir) { return _rmdir(dir); }
inline bool IsDir(const StatStruct& st) {
  return (_S_IFDIR & st.st_mode) != 0;
}

#else

typedef struct stat StatStruct;

inline int FileNo(FILE* file) { return fileno(file); }
inline int IsATTY(int fd) { return isatty(fd); }
inline int Stat(const char* path, StatStruct* buf) { return stat(path, buf); }
inline int StrCaseCmp(const char* s1, const char* s2) {
  return strcasecmp(s1, s2);
}
inline char* StrDup(const char* src) { return ::strdup(src); }
inline int RmDir(const char* dir) { return rmdir(dir); }
inline bool IsDir(const StatStruct& st) { return S_ISDIR(st.st_mode); }

#endif  



#ifdef _MSC_VER

#pragma warning(push)
#pragma warning(disable:4996)
#endif

inline const char* StrNCpy(char* dest, const char* src, size_t n) {
  return strncpy(dest, src, n);
}
inline int ChDir(const char* dir) { return chdir(dir); }
inline FILE* FOpen(const char* path, const char* mode) {
  return fopen(path, mode);
}
inline FILE *FReopen(const char* path, const char* mode, FILE* stream) {
  return freopen(path, mode, stream);
}
inline FILE* FDOpen(int fd, const char* mode) { return fdopen(fd, mode); }
inline int FClose(FILE* fp) { return fclose(fp); }
inline int Read(int fd, void* buf, unsigned int count) {
  return static_cast<int>(read(fd, buf, count));
}
inline int Write(int fd, const void* buf, unsigned int count) {
  return static_cast<int>(write(fd, buf, count));
}
inline int Close(int fd) { return close(fd); }
inline const char* StrError(int errnum) { return strerror(errnum); }
inline const char* GetEnv(const char* name) {
#ifdef _WIN32_WCE  
  return NULL;
#else
  return getenv(name);
#endif
}

#ifdef _MSC_VER
#pragma warning(pop)  // Restores the warning state.
#endif

#ifdef _WIN32_WCE



void Abort();
#else
inline void Abort() { abort(); }
#endif  

}  








const BiggestInt kMaxBiggestInt =
    ~(static_cast<BiggestInt>(1) << (8*sizeof(BiggestInt) - 1));



















template <size_t size>
class TypeWithSize {
 public:
  
  
  typedef void UInt;
};


template <>
class TypeWithSize<4> {
 public:
  
  
  
  
  typedef int Int;
  typedef unsigned int UInt;
};


template <>
class TypeWithSize<8> {
 public:
#if GTEST_OS_WINDOWS
  typedef __int64 Int;
  typedef unsigned __int64 UInt;
#else
  typedef long long Int;  
  typedef unsigned long long UInt;  
#endif  
};


typedef TypeWithSize<4>::Int Int32;
typedef TypeWithSize<4>::UInt UInt32;
typedef TypeWithSize<8>::Int Int64;
typedef TypeWithSize<8>::UInt UInt64;
typedef TypeWithSize<8>::Int TimeInMillis;  

















class GTestCheckProvider {
 public:
  GTestCheckProvider(const char* condition, const char* file, int line) {
    FormatFileLocation(file, line);
    ::std::cerr << " ERROR: Condition " << condition << " failed. ";
  }
  ~GTestCheckProvider() {
    ::std::cerr << ::std::endl;
    abort();
  }
  void FormatFileLocation(const char* file, int line) {
    if (file == NULL)
      file = "unknown file";
    if (line < 0) {
      ::std::cerr << file << ":";
    } else {
#if _MSC_VER
      ::std::cerr << file << "(" << line << "):";
#else
      ::std::cerr << file << ":" << line << ":";
#endif
    }
  }
  ::std::ostream& GetStream() { return ::std::cerr; }
};
#define GTEST_CHECK_(condition) \
    GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
    if (condition) \
      ; \
    else \
      ::testing::internal::GTestCheckProvider(\
          #condition, __FILE__, __LINE__).GetStream()


#define GTEST_FLAG(name) FLAGS_gtest_##name


#define GTEST_DECLARE_bool_(name) extern bool GTEST_FLAG(name)
#define GTEST_DECLARE_int32_(name) \
    extern ::testing::internal::Int32 GTEST_FLAG(name)
#define GTEST_DECLARE_string_(name) \
    extern ::testing::internal::String GTEST_FLAG(name)


#define GTEST_DEFINE_bool_(name, default_val, doc) \
    bool GTEST_FLAG(name) = (default_val)
#define GTEST_DEFINE_int32_(name, default_val, doc) \
    ::testing::internal::Int32 GTEST_FLAG(name) = (default_val)
#define GTEST_DEFINE_string_(name, default_val, doc) \
    ::testing::internal::String GTEST_FLAG(name) = (default_val)







bool ParseInt32(const Message& src_text, const char* str, Int32* value);



bool BoolFromGTestEnv(const char* flag, bool default_val);
Int32 Int32FromGTestEnv(const char* flag, Int32 default_val);
const char* StringFromGTestEnv(const char* flag, const char* default_val);

}  
}  

#endif  
