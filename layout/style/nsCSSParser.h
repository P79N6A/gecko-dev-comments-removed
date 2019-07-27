






#ifndef nsCSSParser_h___
#define nsCSSParser_h___

#include "mozilla/Attributes.h"

#include "nsCSSProperty.h"
#include "nsCSSScanner.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsStringFwd.h"
#include "nsTArrayForwardDeclare.h"

class nsIPrincipal;
class nsIURI;
struct nsCSSSelectorList;
class nsMediaList;
class nsMediaQuery;
class nsCSSKeyframeRule;
class nsCSSValue;
struct nsRuleData;

namespace mozilla {
class CSSStyleSheet;
class CSSVariableValues;
namespace css {
class Rule;
class Declaration;
class Loader;
class StyleRule;
}
}



class MOZ_STACK_CLASS nsCSSParser {
public:
  explicit nsCSSParser(mozilla::css::Loader* aLoader = nullptr,
                       mozilla::CSSStyleSheet* aSheet = nullptr);
  ~nsCSSParser();

  static void Shutdown();

private:
  nsCSSParser(nsCSSParser const&) MOZ_DELETE;
  nsCSSParser& operator=(nsCSSParser const&) MOZ_DELETE;

public:
  
  
  
  nsresult SetStyleSheet(mozilla::CSSStyleSheet* aSheet);

  
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

  
  nsresult ParseVariable(const nsAString&    aVariableName,
                         const nsAString&    aPropValue,
                         nsIURI*             aSheetURL,
                         nsIURI*             aBaseURL,
                         nsIPrincipal*       aSheetPrincipal,
                         mozilla::css::Declaration* aDeclaration,
                         bool*               aChanged,
                         bool                aIsImportant);
  







  void ParseMediaList(const nsSubstring& aBuffer,
                      nsIURI*            aURL,
                      uint32_t           aLineNumber,
                      nsMediaList*       aMediaList,
                      bool               aHTMLMode);

  











  bool ParseSourceSizeList(const nsAString& aBuffer,
                           nsIURI* aURI, 
                           uint32_t aLineNumber, 
                           InfallibleTArray< nsAutoPtr<nsMediaQuery> >& aQueries,
                           InfallibleTArray<nsCSSValue>& aValues,
                           bool aHTMLMode);

  



  bool ParseFontFamilyListString(const nsSubstring& aBuffer,
                                 nsIURI*            aURL,
                                 uint32_t           aLineNumber,
                                 nsCSSValue&        aValue);

  





  bool ParseColorString(const nsSubstring& aBuffer,
                        nsIURI*            aURL,
                        uint32_t           aLineNumber,
                        nsCSSValue&        aValue,
                        bool               aSuppressErrors = false);

  



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

  typedef void (*VariableEnumFunc)(const nsAString&, void*);

  




  bool EnumerateVariableReferences(const nsAString& aPropertyValue,
                                   VariableEnumFunc aFunc,
                                   void* aData);

  



  bool ResolveVariableValue(const nsAString& aPropertyValue,
                            const mozilla::CSSVariableValues* aVariables,
                            nsString& aResult,
                            nsCSSTokenSerializationType& aFirstToken,
                            nsCSSTokenSerializationType& aLastToken);

  











  void ParsePropertyWithVariableReferences(
                                   nsCSSProperty aPropertyID,
                                   nsCSSProperty aShorthandPropertyID,
                                   const nsAString& aValue,
                                   const mozilla::CSSVariableValues* aVariables,
                                   nsRuleData* aRuleData,
                                   nsIURI* aDocURL,
                                   nsIURI* aBaseURL,
                                   nsIPrincipal* aDocPrincipal,
                                   mozilla::CSSStyleSheet* aSheet,
                                   uint32_t aLineNumber,
                                   uint32_t aLineOffset);

  bool ParseCounterStyleName(const nsAString& aBuffer,
                             nsIURI* aURL,
                             nsAString& aName);

  bool ParseCounterDescriptor(nsCSSCounterDesc aDescID,
                              const nsAString& aBuffer,
                              nsIURI* aSheetURL,
                              nsIURI* aBaseURL,
                              nsIPrincipal* aSheetPrincipal,
                              nsCSSValue& aValue);

  
  bool IsValueValidForProperty(const nsCSSProperty aPropID,
                               const nsAString&    aPropValue);

protected:
  
  
  
  void* mImpl;
};

#endif 
