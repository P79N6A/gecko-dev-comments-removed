


















































#ifndef THIRD_PARTY_BREAKPAD_SRC_COMMON_USING_STD_STRING_H_
#define THIRD_PARTY_BREAKPAD_SRC_COMMON_USING_STD_STRING_H_

#ifdef HAS_GLOBAL_STRING
  typedef ::string google_breakpad_string;
#else
  using std::string;
  typedef std::string google_breakpad_string;
#endif


#define HAS_GOOGLE_BREAKPAD_STRING

#endif  
