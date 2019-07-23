































#ifndef GOOGLE_BREAKPAD_PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_H__

#include <ext/hash_map>

#include "google_breakpad/processor/source_line_resolver_interface.h"

namespace google_breakpad {

using std::string;
using __gnu_cxx::hash_map;

class BasicSourceLineResolver : public SourceLineResolverInterface {
 public:
  BasicSourceLineResolver();
  virtual ~BasicSourceLineResolver();

  
  

  
  
  
  virtual bool LoadModule(const string &module_name, const string &map_file);

  virtual bool HasModule(const string &module_name) const;

  virtual StackFrameInfo* FillSourceLineInfo(StackFrame *frame) const;

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

  
  BasicSourceLineResolver(const BasicSourceLineResolver&);
  void operator=(const BasicSourceLineResolver&);
};

}  

#endif  
