




































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

  friend class mozilla::DOMSVGStringList;
  typedef mozilla::SVGStringList SVGStringList;

  DOMSVGTests();

  










  PRInt32 GetBestLanguagePreferenceRank(const nsSubstring& aAcceptLangs) const;

  



  static const nsString * const kIgnoreSystemLanguage;

  










  bool PassesConditionalProcessingTests(
         const nsString *aAcceptLangs = nsnull) const;

  



  bool IsConditionalProcessingAttribute(const nsIAtom* aAttribute) const;

  bool ParseConditionalProcessingAttribute(
         nsIAtom* aAttribute,
         const nsAString& aValue,
         nsAttrValue& aResult);

  


  void UnsetAttr(const nsIAtom* aAttribute);

  nsIAtom* GetAttrName(PRUint8 aAttrEnum) const;
  void GetAttrValue(PRUint8 aAttrEnum, nsAttrValue &aValue) const;

  void MaybeInvalidate();

private:
  enum { FEATURES, EXTENSIONS, LANGUAGE };
  SVGStringList mStringListAttributes[3];
  static nsIAtom** sStringListNames[3];
};

#endif
