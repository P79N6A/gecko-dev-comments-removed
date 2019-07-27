









































#include "LulCommonExt.h"

#include <stdlib.h>
#include <string.h>

#include <string>
#include <map>


namespace lul {

using std::string;




Module::Module(const string &name, const string &os,
               const string &architecture, const string &id) :
    name_(name),
    os_(os),
    architecture_(architecture),
    id_(id) { }

Module::~Module() {
}





class UniqueString {
 public:
  explicit UniqueString(string str) { str_ = strdup(str.c_str()); }
  ~UniqueString() { free(reinterpret_cast<void*>(const_cast<char*>(str_))); }
  const char* str_;
};

const char* const FromUniqueString(const UniqueString* ustr)
{
  return ustr->str_;
}

bool IsEmptyUniqueString(const UniqueString* ustr)
{
  return (ustr->str_)[0] == '\0';
}





UniqueStringUniverse::~UniqueStringUniverse()
{
  for (std::map<string, UniqueString*>::iterator it = map_.begin();
       it != map_.end(); it++) {
    delete it->second;
  }
}

const UniqueString* UniqueStringUniverse::ToUniqueString(string str)
{
  std::map<string, UniqueString*>::iterator it = map_.find(str);
  if (it == map_.end()) {
    UniqueString* ustr = new UniqueString(str);
    map_[str] = ustr;
    return ustr;
  } else {
    return it->second;
  }
}

}  
