
































#ifndef PROCESSOR_SOURCE_LINE_RESOLVER_H__
#define PROCESSOR_SOURCE_LINE_RESOLVER_H__

#include <string>
#include <ext/hash_map>
#include "google_airbag/common/airbag_types.h"

namespace google_airbag {

using std::string;
using __gnu_cxx::hash_map;

struct StackFrame;
struct StackFrameInfo;

class SourceLineResolver {
 public:
  typedef u_int64_t MemAddr;

  SourceLineResolver();
  ~SourceLineResolver();

  
  
  
  
  
  
  bool LoadModule(const string &module_name, const string &map_file);

  
  bool HasModule(const string &module_name) const;

  
  
  
  
  
  
  
  StackFrameInfo* FillSourceLineInfo(StackFrame *frame) const;

 private:
  template<class T> class MemAddrMap;
  struct Line;
  struct Function;
  struct PublicSymbol;
  struct File;
  struct HashString {
    size_t operator()(const string &s) const;
  };
  class Module;

  
  typedef hash_map<string, Module*, HashString> ModuleMap;
  ModuleMap *modules_;

  
  SourceLineResolver(const SourceLineResolver&);
  void operator=(const SourceLineResolver&);
};

}  

#endif  
