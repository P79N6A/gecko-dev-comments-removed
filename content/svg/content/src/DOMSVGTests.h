




































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

  










  PRInt32 GetBestLanguagePreferenceRank(const nsSubstring& aAcceptLangs) const;

  



  static const nsString * const kIgnoreSystemLanguage;

  










  bool PassesConditionalProcessingTests(
         const nsString *aAcceptLangs = nsnull) const;

  



  bool IsConditionalProcessingAttribute(const nsIAtom* aAttribute) const;

  bool ParseConditionalProcessingAttribute(
         nsIAtom* aAttribute,
         const nsAString& aValue,
         nsAttrValue& aResult);

  


  void GetValue(PRUint8 aAttrEnum, nsAString& aValue) const;

  


  void UnsetAttr(const nsIAtom* aAttribute);

  void DidChangeStringList(PRUint8 aAttrEnum);

  void MaybeInvalidate();

private:

  struct StringListInfo {
    nsIAtom** mName;
    bool      mIsCommaSeparated;
  };

  enum { FEATURES, EXTENSIONS, LANGUAGE };
  SVGStringList mStringListAttributes[3];
  static StringListInfo sStringListInfo[3];
};

#endif 
