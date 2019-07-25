

































#include "common/language.h"

namespace google_breakpad {


class CPPLanguage: public Language {
 public:
  string MakeQualifiedName(const string &parent_name,
                           const string &name) const {
    if (parent_name.empty())
      return name;
    else
      return parent_name + "::" + name;
  }
};

const CPPLanguage CPPLanguageSingleton;


class JavaLanguage: public Language {
 public:
  string MakeQualifiedName(const string &parent_name,
                           const string &name) const {
    if (parent_name.empty())
      return name;
    else
      return parent_name + "." + name;
  }
};

JavaLanguage JavaLanguageSingleton;


class AssemblerLanguage: public Language {
  bool HasFunctions() const { return false; }
  string MakeQualifiedName(const string &parent_name,
                           const string &name) const {
    return name;
  }
};

AssemblerLanguage AssemblerLanguageSingleton;

const Language * const Language::CPlusPlus = &CPPLanguageSingleton;
const Language * const Language::Java = &JavaLanguageSingleton;
const Language * const Language::Assembler = &AssemblerLanguageSingleton;

} 
