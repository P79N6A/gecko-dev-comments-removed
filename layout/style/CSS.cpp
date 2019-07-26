






#include "CSS.h"

#include "mozilla/dom/BindingDeclarations.h"
#include "nsCSSParser.h"
#include "nsGlobalWindow.h"
#include "nsIDocument.h"
#include "nsIURI.h"

namespace mozilla {
namespace dom {

struct SupportsParsingInfo
{
  nsIURI* mDocURI;
  nsIURI* mBaseURI;
  nsIPrincipal* mPrincipal;
};

static nsresult
GetParsingInfo(nsISupports* aGlobal,
               SupportsParsingInfo& aInfo)
{
  if (!aGlobal) {
    return NS_ERROR_FAILURE;
  }

  nsGlobalWindow* win = nsGlobalWindow::FromSupports(aGlobal);
  nsCOMPtr<nsIDocument> doc = win->GetDoc();
  if (!doc) {
    return NS_ERROR_FAILURE;
  }

  aInfo.mDocURI = nsCOMPtr<nsIURI>(doc->GetDocumentURI());
  aInfo.mBaseURI = nsCOMPtr<nsIURI>(doc->GetBaseURI());
  aInfo.mPrincipal = win->GetPrincipal();
  return NS_OK;
}

 bool
CSS::Supports(const GlobalObject& aGlobal,
              const nsAString& aProperty,
              const nsAString& aValue,
              ErrorResult& aRv)
{
  nsCSSParser parser;
  SupportsParsingInfo info;

  nsresult rv = GetParsingInfo(aGlobal.Get(), info);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  return parser.EvaluateSupportsDeclaration(aProperty, aValue, info.mDocURI,
                                            info.mBaseURI, info.mPrincipal);
}

 bool
CSS::Supports(const GlobalObject& aGlobal,
              const nsAString& aCondition,
              ErrorResult& aRv)
{
  nsCSSParser parser;
  SupportsParsingInfo info;

  nsresult rv = GetParsingInfo(aGlobal.Get(), info);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  return parser.EvaluateSupportsCondition(aCondition, info.mDocURI,
                                          info.mBaseURI, info.mPrincipal);
}

} 
} 
