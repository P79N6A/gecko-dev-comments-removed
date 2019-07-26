




























#include <string>
#include <map>

#include <stdlib.h>
#include <string.h>

#include "common/unique_string.h"

namespace google_breakpad {




class UniqueString {
 public:
  UniqueString(string str) { str_ = strdup(str.c_str()); }
  ~UniqueString() { free(reinterpret_cast<void*>(const_cast<char*>(str_))); }
  const char* str_;
};

class UniqueStringUniverse {
 public:
  UniqueStringUniverse() {};
  const UniqueString* FindOrCopy(string str) {
    std::map<string, UniqueString*>::iterator it = map_.find(str);
    if (it == map_.end()) {
      UniqueString* ustr = new UniqueString(str);
      map_[str] = ustr;
      return ustr;
    } else {
      return it->second;
    }
  }
 private:
  std::map<string, UniqueString*> map_;
};





static UniqueStringUniverse* sUSU = NULL;



const UniqueString* ToUniqueString(string str) {
  if (!sUSU) {
    sUSU = new UniqueStringUniverse();
  }
  return sUSU->FindOrCopy(str);
}


const UniqueString* ToUniqueString_n(const char* str, size_t n) {
  if (!sUSU) {
    sUSU = new UniqueStringUniverse();
  }
  string key(str, n);
  return sUSU->FindOrCopy(key);
}

const char Index(const UniqueString* us, int ix)
{
  return us->str_[ix];
}

const char* const FromUniqueString(const UniqueString* ustr)
{
  return ustr->str_;
}

int StrcmpUniqueString(const UniqueString* us1, const UniqueString* us2) {
  return strcmp(us1->str_, us2->str_);
}

bool LessThan_UniqueString(const UniqueString* us1, const UniqueString* us2) {
  int r = StrcmpUniqueString(us1, us2);
  return r < 0;
}

}  
