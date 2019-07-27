





#ifndef COMPILER_DIRECTIVE_HANDLER_H_
#define COMPILER_DIRECTIVE_HANDLER_H_

#include "compiler/translator/ExtensionBehavior.h"
#include "compiler/translator/Pragma.h"
#include "compiler/preprocessor/DirectiveHandlerBase.h"

class TDiagnostics;

class TDirectiveHandler : public pp::DirectiveHandler
{
  public:
    TDirectiveHandler(TExtensionBehavior& extBehavior,
                      TDiagnostics& diagnostics,
                      int& shaderVersion);
    virtual ~TDirectiveHandler();

    const TPragma& pragma() const { return mPragma; }
    const TExtensionBehavior& extensionBehavior() const { return mExtensionBehavior; }

    virtual void handleError(const pp::SourceLocation& loc,
                             const std::string& msg);

    virtual void handlePragma(const pp::SourceLocation& loc,
                              const std::string& name,
                              const std::string& value,
                              bool stdgl);

    virtual void handleExtension(const pp::SourceLocation& loc,
                                 const std::string& name,
                                 const std::string& behavior);

    virtual void handleVersion(const pp::SourceLocation& loc,
                               int version);

  private:
    TPragma mPragma;
    TExtensionBehavior& mExtensionBehavior;
    TDiagnostics& mDiagnostics;
    int& mShaderVersion;
};

#endif  
