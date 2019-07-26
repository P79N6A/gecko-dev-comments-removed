




































#ifndef COMMON_LINUX_LANGUAGE_H__
#define COMMON_LINUX_LANGUAGE_H__

#include <string>

namespace google_breakpad {

using std::string;





class Language {
 public:
  
  
  
  
  virtual bool HasFunctions() const { return true; }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual string MakeQualifiedName (const string &parent_name,
                                    const string &name) const = 0;

  
  static const Language * const CPlusPlus,
                        * const Java,
                        * const Assembler;
};

} 

#endif  
