






#ifndef mozilla_css_Rule_h___
#define mozilla_css_Rule_h___

#include "mozilla/CSSStyleSheet.h"
#include "mozilla/MemoryReporting.h"
#include "nsIStyleRule.h"
#include "nsIDOMCSSRule.h"

class nsIDocument;
struct nsRuleData;
template<class T> struct already_AddRefed;
class nsHTMLCSSStyleSheet;

namespace mozilla {
namespace css {
class GroupRule;

#define DECL_STYLE_RULE_INHERIT_NO_DOMRULE  \
virtual void MapRuleInfoInto(nsRuleData* aRuleData) override;

#define DECL_STYLE_RULE_INHERIT                            \
  DECL_STYLE_RULE_INHERIT_NO_DOMRULE                       \
  virtual nsIDOMCSSRule* GetDOMRule() override;        \
  virtual nsIDOMCSSRule* GetExistingDOMRule() override;

class Rule : public nsIStyleRule {
protected:
  Rule(uint32_t aLineNumber, uint32_t aColumnNumber)
    : mSheet(0),
      mParentRule(nullptr),
      mLineNumber(aLineNumber),
      mColumnNumber(aColumnNumber),
      mWasMatched(false)
  {
  }

  Rule(const Rule& aCopy)
    : mSheet(aCopy.mSheet),
      mParentRule(aCopy.mParentRule),
      mLineNumber(aCopy.mLineNumber),
      mColumnNumber(aCopy.mColumnNumber),
      mWasMatched(false)
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
    SUPPORTS_RULE,
    FONT_FEATURE_VALUES_RULE,
    COUNTER_STYLE_RULE
  };

  virtual int32_t GetType() const = 0;

  CSSStyleSheet* GetStyleSheet() const;
  nsHTMLCSSStyleSheet* GetHTMLCSSStyleSheet() const;

  
  nsIDocument* GetDocument() const
  {
    CSSStyleSheet* sheet = GetStyleSheet();
    return sheet ? sheet->GetDocument() : nullptr;
  }

  virtual void SetStyleSheet(CSSStyleSheet* aSheet);
  
  
  void SetHTMLCSSStyleSheet(nsHTMLCSSStyleSheet* aSheet);

  void SetParentRule(GroupRule* aRule) {
    
    
    
    mParentRule = aRule;
  }

  uint32_t GetLineNumber() const { return mLineNumber; }
  uint32_t GetColumnNumber() const { return mColumnNumber; }

  


  virtual already_AddRefed<Rule> Clone() const = 0;

  
  
  virtual nsIDOMCSSRule* GetDOMRule() = 0;

  
  virtual nsIDOMCSSRule* GetExistingDOMRule() = 0;

  
  nsresult GetParentRule(nsIDOMCSSRule** aParentRule);
  nsresult GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet);
  Rule* GetCSSRule();

  
  
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
    const MOZ_MUST_OVERRIDE = 0;

  
  static size_t SizeOfCOMArrayElementIncludingThis(css::Rule* aElement,
                                                   mozilla::MallocSizeOf aMallocSizeOf,
                                                   void* aData);

protected:
  
  
  uintptr_t         mSheet;
  GroupRule*        mParentRule;

  
  uint32_t          mLineNumber;
  uint32_t          mColumnNumber : 31;
  uint32_t          mWasMatched : 1;
};

} 
} 

#endif
