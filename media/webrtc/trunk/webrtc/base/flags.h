





















#ifndef WEBRTC_BASE_FLAGS_H__
#define WEBRTC_BASE_FLAGS_H__

#include <assert.h>

#include "webrtc/base/checks.h"
#include "webrtc/base/common.h"

namespace rtc {


union FlagValue {
  
  
  
  
  
  static FlagValue New_BOOL(int b) {
    FlagValue v;
    v.b = (b != 0);
    return v;
  }

  static FlagValue New_INT(int i) {
    FlagValue v;
    v.i = i;
    return v;
  }

  static FlagValue New_FLOAT(float f) {
    FlagValue v;
    v.f = f;
    return v;
  }

  static FlagValue New_STRING(const char* s) {
    FlagValue v;
    v.s = s;
    return v;
  }

  bool b;
  int i;
  double f;
  const char* s;
};



class Flag {
 public:
  enum Type { BOOL, INT, FLOAT, STRING };

  
  Flag(const char* file, const char* name, const char* comment,
       Type type, void* variable, FlagValue default_);

  
  const char* file() const  { return file_; }
  const char* name() const  { return name_; }
  const char* comment() const  { return comment_; }

  
  Type type() const  { return type_; }

  
  bool* bool_variable() const {
    assert(type_ == BOOL);
    return &variable_->b;
  }
  
  int* int_variable() const {
    assert(type_ == INT);
    return &variable_->i;
  }
  
  double* float_variable() const {
    assert(type_ == FLOAT);
    return &variable_->f;
  }
  
  const char** string_variable() const {
    assert(type_ == STRING);
    return &variable_->s;
  }

  
  bool bool_default() const {
    assert(type_ == BOOL);
    return default_.b;
  }
  
  int int_default() const {
    assert(type_ == INT);
    return default_.i;
  }
  
  double float_default() const {
    assert(type_ == FLOAT);
    return default_.f;
  }
  
  const char* string_default() const {
    assert(type_ == STRING);
    return default_.s;
  }

  
  void SetToDefault();

  
  Flag* next() const  { return next_; }

  
  
  void Print(bool print_current_value);

 private:
  const char* file_;
  const char* name_;
  const char* comment_;

  Type type_;
  FlagValue* variable_;
  FlagValue default_;

  Flag* next_;

  friend class FlagList;  
};



#define DEFINE_FLAG(type, c_type, name, default, comment) \
  /* define and initialize the flag */                    \
  c_type FLAG_##name = (default);                         \
  /* register the flag */                                 \
  static rtc::Flag Flag_##name(__FILE__, #name, (comment),      \
                               rtc::Flag::type, &FLAG_##name,   \
                               rtc::FlagValue::New_##type(default))



#define DECLARE_FLAG(c_type, name)              \
  /* declare the external flag */               \
  extern c_type FLAG_##name



#define DEFINE_bool(name, default, comment) \
  DEFINE_FLAG(BOOL, bool, name, default, comment)
#define DEFINE_int(name, default, comment) \
  DEFINE_FLAG(INT, int, name, default, comment)
#define DEFINE_float(name, default, comment) \
  DEFINE_FLAG(FLOAT, double, name, default, comment)
#define DEFINE_string(name, default, comment) \
  DEFINE_FLAG(STRING, const char*, name, default, comment)



#define DECLARE_bool(name)  DECLARE_FLAG(bool, name)
#define DECLARE_int(name)  DECLARE_FLAG(int, name)
#define DECLARE_float(name)  DECLARE_FLAG(double, name)
#define DECLARE_string(name)  DECLARE_FLAG(const char*, name)



class FlagList {
 public:
  FlagList();

  
  static Flag* list()  { return list_; }

  
  
  
  static void Print(const char* file, bool print_current_value);

  
  static Flag* Lookup(const char* name);

  
  
  
  
  static void SplitArgument(const char* arg,
                            char* buffer, int buffer_size,
                            const char** name, const char** value,
                            bool* is_bool);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int SetFlagsFromCommandLine(int* argc,
                                     const char** argv,
                                     bool remove_flags);
  static inline int SetFlagsFromCommandLine(int* argc,
                                            char** argv,
                                            bool remove_flags) {
    return SetFlagsFromCommandLine(argc, const_cast<const char**>(argv),
                                   remove_flags);
  }

  
  
  static void Register(Flag* flag);

 private:
  static Flag* list_;
};

#if defined(WEBRTC_WIN)





class WindowsCommandLineArguments {
 public:
  WindowsCommandLineArguments();
  ~WindowsCommandLineArguments();

  int argc() { return argc_; }
  char **argv() { return argv_; }
 private:
  int argc_;
  char **argv_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(WindowsCommandLineArguments);
};
#endif  

}  

#endif  
