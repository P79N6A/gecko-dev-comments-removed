






#ifndef mozilla_css_Rule_h___
#define mozilla_css_Rule_h___

#include "nsIStyleRule.h"
#include "nsIDOMCSSRule.h"
#include "nsCSSStyleSheet.h"

class nsIStyleSheet;
class nsIDocument;
struct nsRuleData;
template<class T> struct already_AddRefed;
class nsHTMLCSSStyleSheet;

namespace mozilla {
namespace css {
class GroupRule;

#define DECL_STYLE_RULE_INHERIT_NO_DOMRULE  \
virtual void MapRuleInfoInto(nsRuleData* aRuleData);

#define DECL_STYLE_RULE_INHERIT                   \
  DECL_STYLE_RULE_INHERIT_NO_DOMRULE              \
  virtual nsIDOMCSSRule* GetDOMRule();            \
  virtual nsIDOMCSSRule* GetExistingDOMRule();

class Rule : public nsIStyleRule {
protected:
  Rule()
    : mSheet(0),
      mParentRule(nullptr)
  {
  }

  Rule(const Rule& aCopy)
    : mSheet(aCopy.mSheet),
      mParentRule(aCopy.mParentRule)
  {
  }

  virtual ~Rule() {}

public:

  
  
  
  
  
  enum {
    UNKNOWN_RULE = 0,
    CHARSET_RULE,
    IMPORT_RULE,
    NAMESPACE_RULE,
    STYLE_RULE,
    MEDIA_RULE,
    FONT_FACE_RULE,
    PAGE_RULE,
    KEYFRAME_RULE,
    KEYFRAMES_RULE,
    DOCUMENT_RULE,
    SUPPORTS_RULE
  };

  virtual int32_t GetType() const = 0;

  nsCSSStyleSheet* GetStyleSheet() const;
  nsHTMLCSSStyleSheet* GetHTMLCSSStyleSheet() const;

  
  nsIDocument* GetDocument() const
  {
    nsCSSStyleSheet* sheet = GetStyleSheet();
    return sheet ? sheet->GetDocument() : nullptr;
  }

  virtual void SetStyleSheet(nsCSSStyleSheet* aSheet);
  
  
  void SetHTMLCSSStyleSheet(nsHTMLCSSStyleSheet* aSheet);

  void SetParentRule(GroupRule* aRule) {
    
    
    
    mParentRule = aRule;
  }

  


  virtual already_AddRefed<Rule> Clone() const = 0;

  
  
  virtual nsIDOMCSSRule* GetDOMRule() = 0;

  
  virtual nsIDOMCSSRule* GetExistingDOMRule() = 0;

  
  nsresult GetParentRule(nsIDOMCSSRule** aParentRule);
  nsresult GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet);

  
  
  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)
    const MOZ_MUST_OVERRIDE = 0;

  
  static size_t SizeOfCOMArrayElementIncludingThis(css::Rule* aElement,
                                                   nsMallocSizeOfFun aMallocSizeOf,
                                                   void* aData);

protected:
  
  
  uintptr_t         mSheet;
  GroupRule*        mParentRule;
};

} 
} 

#endif
