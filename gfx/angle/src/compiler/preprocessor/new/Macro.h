





#ifndef COMPILER_PREPROCESSOR_MACRO_H_
#define COMPILER_PREPROCESSOR_MACRO_H_

#include <string>
#include <vector>

#include "common/angleutils.h"
#include "Token.h"

namespace pp
{

class Macro
{
  public:
    enum Type
    {
        kTypeObj,
        kTypeFunc
    };

    
    Macro(Type type,
          std::string* name,
          TokenVector* parameters,
          TokenVector* replacements);
    ~Macro();

    Type type() const { return mType; }
    const std::string* identifier() const { return mName; }
    const TokenVector* parameters() const { return mParameters; }
    const TokenVector* replacements() const { return mReplacements; }

  private:
    DISALLOW_COPY_AND_ASSIGN(Macro);

    Type mType;
    std::string* mName;
    TokenVector* mParameters;
    TokenVector* mReplacements;
};

}  
#endif COMPILER_PREPROCESSOR_MACRO_H_

