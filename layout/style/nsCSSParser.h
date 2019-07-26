






#ifndef nsCSSParser_h___
#define nsCSSParser_h___

#include "mozilla/Attributes.h"

#include "nsCSSProperty.h"
#include "nsCOMPtr.h"
#include "nsStringFwd.h"
#include "nsTArrayForwardDeclare.h"

class nsCSSStyleSheet;
class nsIPrincipal;
class nsIURI;
struct nsCSSSelectorList;
class nsMediaList;
class nsCSSKeyframeRule;
class nsCSSValue;

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
  nsCSSParser(mozilla::css::Loader* aLoader = nullptr,
              nsCSSStyleSheet* aSheet = nullptr);
  ~nsCSSParser();

  static void Shutdown();

private:
  nsCSSParser(nsCSSParser const&) MOZ_DELETE;
  nsCSSParser& operator=(nsCSSParser const&) MOZ_DELETE;

public:
  
  
  
  nsresult SetStyleSheet(nsCSSStyleSheet* aSheet);

  
  nsresult SetQuirkMode(bool aQuirkMode);

  
  nsresult SetChildLoader(mozilla::css::Loader* aChildLoader);

  















  nsresult ParseSheet(const nsAString& aInput,
                      nsIURI*          aSheetURL,
                      nsIURI*          aBaseURI,
                      nsIPrincipal*    aSheetPrincipal,
                      uint32_t         aLineNumber,
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
                     mozilla::css::Rule**    aResult);

  
  
  
  
  
  
  
  
  nsresult ParseProperty(const nsCSSProperty aPropID,
                         const nsAString&    aPropValue,
                         nsIURI*             aSheetURL,
                         nsIURI*             aBaseURL,
                         nsIPrincipal*       aSheetPrincipal,
                         mozilla::css::Declaration* aDeclaration,
                         bool*               aChanged,
                         bool                aIsImportant,
                         bool                aIsSVGMode = false);

  







  nsresult ParseMediaList(const nsSubstring& aBuffer,
                          nsIURI*            aURL,
                          uint32_t           aLineNumber,
                          nsMediaList*       aMediaList,
                          bool               aHTMLMode);

  





  bool ParseColorString(const nsSubstring& aBuffer,
                        nsIURI*            aURL,
                        uint32_t           aLineNumber,
                        nsCSSValue&        aValue);

  



  nsresult ParseSelectorString(const nsSubstring&  aSelectorString,
                               nsIURI*             aURL,
                               uint32_t            aLineNumber,
                               nsCSSSelectorList** aSelectorList);

  



  already_AddRefed<nsCSSKeyframeRule>
  ParseKeyframeRule(const nsSubstring& aBuffer,
                    nsIURI*            aURL,
                    uint32_t           aLineNumber);

  



  bool ParseKeyframeSelectorString(const nsSubstring& aSelectorString,
                                   nsIURI*            aURL,
                                   uint32_t           aLineNumber,
                                   InfallibleTArray<float>& aSelectorList);

  



  bool EvaluateSupportsDeclaration(const nsAString& aProperty,
                                   const nsAString& aValue,
                                   nsIURI* aDocURL,
                                   nsIURI* aBaseURL,
                                   nsIPrincipal* aDocPrincipal);

  



  bool EvaluateSupportsCondition(const nsAString& aCondition,
                                 nsIURI* aDocURL,
                                 nsIURI* aBaseURL,
                                 nsIPrincipal* aDocPrincipal);

protected:
  
  
  
  void* mImpl;
};

#endif 
