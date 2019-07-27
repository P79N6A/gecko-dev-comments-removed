




#ifndef mozilla_dom_SVGTests_h
#define mozilla_dom_SVGTests_h

#include "nsStringFwd.h"
#include "SVGStringList.h"
#include "nsCOMPtr.h"

class nsAttrValue;
class nsIAtom;
class nsString;

namespace mozilla {
class DOMSVGStringList;

#define MOZILLA_DOMSVGTESTS_IID \
   { 0x92370da8, 0xda28, 0x4895, \
     {0x9b, 0x1b, 0xe0, 0x06, 0x0d, 0xb7, 0x3f, 0xc3 } }

namespace dom {

class SVGTests : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGTESTS_IID)
  NS_DECL_ISUPPORTS

  SVGTests();

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

  
  already_AddRefed<DOMSVGStringList> RequiredFeatures();
  already_AddRefed<DOMSVGStringList> RequiredExtensions();
  already_AddRefed<DOMSVGStringList> SystemLanguage();
  bool HasExtension(const nsAString& aExtension);

protected:
  virtual ~SVGTests() {}

private:
  enum { FEATURES, EXTENSIONS, LANGUAGE };
  SVGStringList mStringListAttributes[3];
  static nsIAtom** sStringListNames[3];
};

NS_DEFINE_STATIC_IID_ACCESSOR(SVGTests, MOZILLA_DOMSVGTESTS_IID)

} 
} 

#endif
