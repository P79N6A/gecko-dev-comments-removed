




#ifndef MOZILLA_DOMSVGTESTS_H__
#define MOZILLA_DOMSVGTESTS_H__

#include "nsIDOMSVGTests.h"
#include "nsStringFwd.h"
#include "SVGStringList.h"

class nsAttrValue;
class nsIAtom;
class nsString;

namespace mozilla {
class DOMSVGStringList;
}

class DOMSVGTests : public nsIDOMSVGTests
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMSVGTESTS

  DOMSVGTests();
  virtual ~DOMSVGTests() {}

  friend class mozilla::DOMSVGStringList;
  typedef mozilla::SVGStringList SVGStringList;

  










  int32_t GetBestLanguagePreferenceRank(const nsSubstring& aAcceptLangs) const;

  



  static const nsString * const kIgnoreSystemLanguage;

  










  bool PassesConditionalProcessingTests(
         const nsString *aAcceptLangs = nullptr) const;

  



  bool IsConditionalProcessingAttribute(const nsIAtom* aAttribute) const;

  bool ParseConditionalProcessingAttribute(
         nsIAtom* aAttribute,
         const nsAString& aValue,
         nsAttrValue& aResult);

  


  void UnsetAttr(const nsIAtom* aAttribute);

  nsIAtom* GetAttrName(uint8_t aAttrEnum) const;
  void GetAttrValue(uint8_t aAttrEnum, nsAttrValue &aValue) const;

  void MaybeInvalidate();

private:
  enum { FEATURES, EXTENSIONS, LANGUAGE };
  SVGStringList mStringListAttributes[3];
  static nsIAtom** sStringListNames[3];
};

#endif
