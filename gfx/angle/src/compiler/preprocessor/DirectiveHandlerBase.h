





#ifndef COMPILER_PREPROCESSOR_DIRECTIVE_HANDLER_H_
#define COMPILER_PREPROCESSOR_DIRECTIVE_HANDLER_H_

#include <string>

namespace pp
{

struct SourceLocation;





class DirectiveHandler
{
  public:
    virtual ~DirectiveHandler();

    virtual void handleError(const SourceLocation &loc,
                             const std::string &msg) = 0;

    
    virtual void handlePragma(const SourceLocation &loc,
                              const std::string &name,
                              const std::string &value,
                              bool stdgl) = 0;

    virtual void handleExtension(const SourceLocation &loc,
                                 const std::string &name,
                                 const std::string &behavior) = 0;

    virtual void handleVersion(const SourceLocation &loc,
                               int version) = 0;
};

}  
#endif  
