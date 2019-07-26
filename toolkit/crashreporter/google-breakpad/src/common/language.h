




































#ifndef COMMON_LINUX_LANGUAGE_H__
#define COMMON_LINUX_LANGUAGE_H__

#include <string>

#include "common/using_std_string.h"

namespace google_breakpad {





class Language {
 public:
  
  
  virtual ~Language() {}

  
  
  
  
  virtual bool HasFunctions() const { return true; }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual string MakeQualifiedName (const string &parent_name,
                                    const string &name) const = 0;

  
  static const Language * const CPlusPlus,
                        * const Java,
                        * const Assembler;
};

} 

#endif  
