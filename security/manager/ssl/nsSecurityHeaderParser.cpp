



#include "nsSecurityHeaderParser.h"
#include "mozilla/Logging.h"











bool
IsTokenSymbol(signed char chr) {
  if (chr < 33 || chr == 127 ||
      chr == '(' || chr == ')' || chr == '<' || chr == '>' ||
      chr == '@' || chr == ',' || chr == ';' || chr == ':' ||
      chr == '"' || chr == '/' || chr == '[' || chr == ']' ||
      chr == '?' || chr == '=' || chr == '{' || chr == '}' || chr == '\\') {
    return false;
  }
  return true;
}








bool
IsQuotedTextSymbol(signed char chr) {
  return ((chr >= 32 && chr != '"' && chr != '\\' && chr != 127) ||
          chr == 0x9 || chr == 0xa || chr == 0xd);
}


bool
IsQuotedPairSymbol(signed char chr) {
  return (chr >= 0);
}

static PRLogModuleInfo *
GetSHParserLog()
{
  static PRLogModuleInfo *sSHParserLog;
  if (!sSHParserLog) {
    sSHParserLog = PR_NewLogModule("nsSecurityHeaderParser");
  }
  return sSHParserLog;
}

#define SHPARSERLOG(args) MOZ_LOG(GetSHParserLog(), mozilla::LogLevel::Debug, args)

nsSecurityHeaderParser::nsSecurityHeaderParser(const char *aHeader)
  : mCursor(aHeader)
  , mError(false)
{
}

nsSecurityHeaderParser::~nsSecurityHeaderParser() {
  nsSecurityHeaderDirective *directive;
  while ((directive = mDirectives.popFirst())) {
    delete directive;
  }
}

mozilla::LinkedList<nsSecurityHeaderDirective> *
nsSecurityHeaderParser::GetDirectives() {
  return &mDirectives;
}

nsresult
nsSecurityHeaderParser::Parse() {
  MOZ_ASSERT(mDirectives.isEmpty());
  SHPARSERLOG(("trying to parse '%s'", mCursor));

  Header();

  
  if (mError || *mCursor) {
    return NS_ERROR_FAILURE;
  } else {
    return NS_OK;
  }
}

bool
nsSecurityHeaderParser::Accept(char aChr)
{
  if (*mCursor == aChr) {
    Advance();
    return true;
  }

  return false;
}

bool
nsSecurityHeaderParser::Accept(bool (*aClassifier) (signed char))
{
  if (aClassifier(*mCursor)) {
    Advance();
    return true;
  }

  return false;
}

void
nsSecurityHeaderParser::Expect(char aChr)
{
  if (*mCursor != aChr) {
    mError = true;
  } else {
    Advance();
  }
}

void
nsSecurityHeaderParser::Advance()
{
  
  
  if (*mCursor) {
    mOutput.Append(*mCursor);
    mCursor++;
  } else {
    mError = true;
  }
}

void
nsSecurityHeaderParser::Header()
{
  Directive();
  while (Accept(';')) {
    Directive();
  }
}

void
nsSecurityHeaderParser::Directive()
{
  mDirective = new nsSecurityHeaderDirective();
  LWSMultiple();
  DirectiveName();
  LWSMultiple();
  if (Accept('=')) {
    LWSMultiple();
    DirectiveValue();
    LWSMultiple();
  }
  mDirectives.insertBack(mDirective);
  SHPARSERLOG(("read directive name '%s', value '%s'",
               mDirective->mName.Data(), mDirective->mValue.Data()));
}

void
nsSecurityHeaderParser::DirectiveName()
{
  mOutput.Truncate(0);
  Token();
  mDirective->mName.Assign(mOutput);
}

void
nsSecurityHeaderParser::DirectiveValue()
{
  mOutput.Truncate(0);
  if (Accept(IsTokenSymbol)) {
    Token();
    mDirective->mValue.Assign(mOutput);
  } else if (Accept('"')) {
    
    
    
    mOutput.Truncate(0);
    QuotedString();
    mDirective->mValue.Assign(mOutput);
    Expect('"');
  }
}

void
nsSecurityHeaderParser::Token()
{
  while (Accept(IsTokenSymbol));
}

void
nsSecurityHeaderParser::QuotedString()
{
  while (true) {
    if (Accept(IsQuotedTextSymbol)) {
      QuotedText();
    } else if (Accept('\\')) {
      QuotedPair();
    } else {
      break;
    }
  }
}

void
nsSecurityHeaderParser::QuotedText()
{
  while (Accept(IsQuotedTextSymbol));
}

void
nsSecurityHeaderParser::QuotedPair()
{
  Accept(IsQuotedPairSymbol);
}

void
nsSecurityHeaderParser::LWSMultiple()
{
  while (true) {
    if (Accept('\r')) {
      LWSCRLF();
    } else if (Accept(' ') || Accept('\t')) {
      LWS();
    } else {
      break;
    }
  }
}

void
nsSecurityHeaderParser::LWSCRLF() {
  Expect('\n');
  if (!(Accept(' ') || Accept('\t'))) {
    mError = true;
  }
  LWS();
}

void
nsSecurityHeaderParser::LWS()
{
  
  
  while (Accept(' ') || Accept('\t'));
}
