




#ifndef CSSLexer_h___
#define CSSLexer_h___

#include "mozilla/UniquePtr.h"
#include "nsCSSScanner.h"
#include "mozilla/dom/CSSLexerBinding.h"

namespace mozilla {
namespace dom {

class CSSLexer : public NonRefcountedDOMObject
{
public:
  explicit CSSLexer(const nsAString&);
  ~CSSLexer();

  bool WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto,
                  JS::MutableHandle<JSObject*> aReflector);

  uint32_t LineNumber();
  uint32_t ColumnNumber();
  void NextToken(Nullable<CSSToken>& aResult);

private:
  nsString mInput;
  nsCSSScanner mScanner;
};

} 
} 

#endif 
