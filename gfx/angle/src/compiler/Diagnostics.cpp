





#include "compiler/Diagnostics.h"

#include "compiler/compiler_debug.h"
#include "compiler/InfoSink.h"
#include "compiler/preprocessor/SourceLocation.h"

TDiagnostics::TDiagnostics(TInfoSink& infoSink) :
    mInfoSink(infoSink),
    mNumErrors(0),
    mNumWarnings(0)
{
}

TDiagnostics::~TDiagnostics()
{
}

void TDiagnostics::writeInfo(Severity severity,
                             const pp::SourceLocation& loc,
                             const std::string& reason,
                             const std::string& token,
                             const std::string& extra)
{
    TPrefixType prefix = EPrefixNone;
    switch (severity)
    {
      case ERROR:
        ++mNumErrors;
        prefix = EPrefixError;
        break;
      case WARNING:
        ++mNumWarnings;
        prefix = EPrefixWarning;
        break;
      default:
        UNREACHABLE();
        break;
    }

    TInfoSinkBase& sink = mInfoSink.info;
    
    sink.prefix(prefix);
    sink.location(EncodeSourceLoc(loc.file, loc.line));
    sink << "'" << token <<  "' : " << reason << " " << extra << "\n";
}

void TDiagnostics::writeDebug(const std::string& str)
{
    mInfoSink.debug << str;
}

void TDiagnostics::print(ID id,
                         const pp::SourceLocation& loc,
                         const std::string& text)
{
    writeInfo(severity(id), loc, message(id), text, "");
}
