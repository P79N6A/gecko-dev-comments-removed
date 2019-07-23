































#ifndef GOOGLE_BREAKPAD_PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_H__



#ifdef __SUNPRO_CC
#define BSLR_NO_HASH_MAP
#endif  

#ifdef BSLR_NO_HASH_MAP
#include <map>
#else  
#include <ext/hash_map>
#endif  

#include "google_breakpad/processor/source_line_resolver_interface.h"

namespace google_breakpad {

using std::string;
#ifdef BSLR_NO_HASH_MAP
using std::map;
#else  
using __gnu_cxx::hash_map;
#endif  

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
#ifdef BSLR_NO_HASH_MAP
  struct CompareString {
    bool operator()(const string &s1, const string &s2) const;
  };
#else  
  struct HashString {
    size_t operator()(const string &s) const;
  };
#endif  
  class Module;

  
#ifdef BSLR_NO_HASH_MAP
  typedef map<string, Module*, CompareString> ModuleMap;
#else  
  typedef hash_map<string, Module*, HashString> ModuleMap;
#endif  
  ModuleMap *modules_;

  
  BasicSourceLineResolver(const BasicSourceLineResolver&);
  void operator=(const BasicSourceLineResolver&);
};

}  

#endif  
