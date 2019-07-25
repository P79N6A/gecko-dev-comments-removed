






































#ifndef nsCSSParser_h___
#define nsCSSParser_h___

#include "mozilla/Attributes.h"

#include "nsAString.h"
#include "nsCSSProperty.h"
#include "nsColor.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"

class nsCSSStyleSheet;
class nsIPrincipal;
class nsIURI;
struct nsCSSSelectorList;
class nsMediaList;
class nsCSSKeyframeRule;

namespace mozilla {
namespace css {
class Rule;
class Declaration;
class Loader;
class StyleRule;
}
}



class NS_STACK_CLASS nsCSSParser {
public:
  nsCSSParser(mozilla::css::Loader* aLoader = nsnull,
              nsCSSStyleSheet* aSheet = nsnull);
  ~nsCSSParser();

  static void Shutdown();

private:
  nsCSSParser(nsCSSParser const&) MOZ_DELETE;
  nsCSSParser& operator=(nsCSSParser const&) MOZ_DELETE;

public:
  
  
  
  nsresult SetStyleSheet(nsCSSStyleSheet* aSheet);

  
  nsresult SetQuirkMode(bool aQuirkMode);

  
  nsresult SetSVGMode(bool aSVGMode);

  
  nsresult SetChildLoader(mozilla::css::Loader* aChildLoader);

  















  nsresult ParseSheet(const nsAString& aInput,
                      nsIURI*          aSheetURL,
                      nsIURI*          aBaseURI,
                      nsIPrincipal*    aSheetPrincipal,
                      PRUint32         aLineNumber,
                      bool             aAllowUnsafeRules);

  
  
  
  nsresult ParseStyleAttribute(const nsAString&  aAttributeValue,
                               nsIURI*           aDocURL,
                               nsIURI*           aBaseURL,
                               nsIPrincipal*     aNodePrincipal,
                               mozilla::css::StyleRule** aResult);

  
  
  
  
  
  nsresult ParseDeclarations(const nsAString&  aBuffer,
                             nsIURI*           aSheetURL,
                             nsIURI*           aBaseURL,
                             nsIPrincipal*     aSheetPrincipal,
                             mozilla::css::Declaration* aDeclaration,
                             bool*           aChanged);

  nsresult ParseRule(const nsAString&        aRule,
                     nsIURI*                 aSheetURL,
                     nsIURI*                 aBaseURL,
                     nsIPrincipal*           aSheetPrincipal,
                     nsCOMArray<mozilla::css::Rule>& aResult);

  nsresult ParseProperty(const nsCSSProperty aPropID,
                         const nsAString&    aPropValue,
                         nsIURI*             aSheetURL,
                         nsIURI*             aBaseURL,
                         nsIPrincipal*       aSheetPrincipal,
                         mozilla::css::Declaration* aDeclaration,
                         bool*             aChanged,
                         bool                aIsImportant);

  







  nsresult ParseMediaList(const nsSubstring& aBuffer,
                          nsIURI*            aURL,
                          PRUint32           aLineNumber,
                          nsMediaList*       aMediaList,
                          bool               aHTMLMode);

  








  nsresult ParseColorString(const nsSubstring& aBuffer,
                            nsIURI*            aURL,
                            PRUint32           aLineNumber,
                            nscolor*           aColor);

  



  nsresult ParseSelectorString(const nsSubstring&  aSelectorString,
                               nsIURI*             aURL,
                               PRUint32            aLineNumber,
                               nsCSSSelectorList** aSelectorList);

  



  already_AddRefed<nsCSSKeyframeRule>
  ParseKeyframeRule(const nsSubstring& aBuffer,
                    nsIURI*            aURL,
                    PRUint32           aLineNumber);

  



  bool ParseKeyframeSelectorString(const nsSubstring& aSelectorString,
                                   nsIURI*            aURL,
                                   PRUint32           aLineNumber,
                                   InfallibleTArray<float>& aSelectorList);

protected:
  
  
  
  void* mImpl;
};

#endif 
