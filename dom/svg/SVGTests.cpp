




#include "mozilla/dom/SVGTests.h"
#include "DOMSVGStringList.h"
#include "nsSVGFeatures.h"
#include "mozilla/dom/SVGSwitchElement.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsStyleUtil.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS0(SVGTests)

nsIAtom** SVGTests::sStringListNames[3] =
{
  &nsGkAtoms::requiredFeatures,
  &nsGkAtoms::requiredExtensions,
  &nsGkAtoms::systemLanguage,
};

SVGTests::SVGTests()
{
  mStringListAttributes[LANGUAGE].SetIsCommaSeparated(true);
}

already_AddRefed<DOMSVGStringList>
SVGTests::RequiredFeatures()
{
  nsCOMPtr<nsIDOMSVGElement> elem = do_QueryInterface(this);
  nsSVGElement* element = static_cast<nsSVGElement*>(elem.get());
  return DOMSVGStringList::GetDOMWrapper(
           &mStringListAttributes[FEATURES], element, true, FEATURES);
}

already_AddRefed<DOMSVGStringList>
SVGTests::RequiredExtensions()
{
  nsCOMPtr<nsIDOMSVGElement> elem = do_QueryInterface(this);
  nsSVGElement* element = static_cast<nsSVGElement*>(elem.get());
  return DOMSVGStringList::GetDOMWrapper(
           &mStringListAttributes[EXTENSIONS], element, true, EXTENSIONS);
}

already_AddRefed<DOMSVGStringList>
SVGTests::SystemLanguage()
{
  nsCOMPtr<nsIDOMSVGElement> elem = do_QueryInterface(this);
  nsSVGElement* element = static_cast<nsSVGElement*>(elem.get());
  return DOMSVGStringList::GetDOMWrapper(
           &mStringListAttributes[LANGUAGE], element, true, LANGUAGE);
}

bool
SVGTests::HasExtension(const nsAString& aExtension)
{
  return nsSVGFeatures::HasExtension(aExtension);
}

bool
SVGTests::IsConditionalProcessingAttribute(const nsIAtom* aAttribute) const
{
  for (uint32_t i = 0; i < ArrayLength(sStringListNames); i++) {
    if (aAttribute == *sStringListNames[i]) {
      return true;
    }
  }
  return false;
}

int32_t
SVGTests::GetBestLanguagePreferenceRank(const nsSubstring& aAcceptLangs) const
{
  const nsDefaultStringComparator defaultComparator;

  if (!mStringListAttributes[LANGUAGE].IsExplicitlySet()) {
    return -2;
  }

  int32_t lowestRank = -1;

  for (uint32_t i = 0; i < mStringListAttributes[LANGUAGE].Length(); i++) {
    nsCharSeparatedTokenizer languageTokenizer(aAcceptLangs, ',');
    int32_t index = 0;
    while (languageTokenizer.hasMoreTokens()) {
      const nsSubstring &languageToken = languageTokenizer.nextToken();
      bool exactMatch = (languageToken == mStringListAttributes[LANGUAGE][i]);
      bool prefixOnlyMatch =
        !exactMatch &&
        nsStyleUtil::DashMatchCompare(mStringListAttributes[LANGUAGE][i],
                                      languageTokenizer.nextToken(),
                                      defaultComparator);
      if (index == 0 && exactMatch) {
        
        return 0;
      }
      if ((exactMatch || prefixOnlyMatch) &&
          (lowestRank == -1 || 2 * index + prefixOnlyMatch < lowestRank)) {
        lowestRank = 2 * index + prefixOnlyMatch;
      }
      ++index;
    }
  }
  return lowestRank;
}

const nsString * const SVGTests::kIgnoreSystemLanguage = (nsString *) 0x01;

bool
SVGTests::PassesConditionalProcessingTests(const nsString *aAcceptLangs) const
{
  
  if (mStringListAttributes[FEATURES].IsExplicitlySet()) {
    if (mStringListAttributes[FEATURES].IsEmpty()) {
      return false;
    }
    nsCOMPtr<nsIContent> content(
      do_QueryInterface(const_cast<SVGTests*>(this)));

    for (uint32_t i = 0; i < mStringListAttributes[FEATURES].Length(); i++) {
      if (!nsSVGFeatures::HasFeature(content, mStringListAttributes[FEATURES][i])) {
        return false;
      }
    }
  }

  
  
  
  
  
  
  
  if (mStringListAttributes[EXTENSIONS].IsExplicitlySet()) {
    if (mStringListAttributes[EXTENSIONS].IsEmpty()) {
      return false;
    }
    for (uint32_t i = 0; i < mStringListAttributes[EXTENSIONS].Length(); i++) {
      if (!nsSVGFeatures::HasExtension(mStringListAttributes[EXTENSIONS][i])) {
        return false;
      }
    }
  }

  if (aAcceptLangs == kIgnoreSystemLanguage) {
    return true;
  }

  
  
  
  
  
  
  
  if (mStringListAttributes[LANGUAGE].IsExplicitlySet()) {
    if (mStringListAttributes[LANGUAGE].IsEmpty()) {
      return false;
    }

    
    const nsAutoString acceptLangs(aAcceptLangs ? *aAcceptLangs :
      Preferences::GetLocalizedString("intl.accept_languages"));

    if (acceptLangs.IsEmpty()) {
      NS_WARNING("no default language specified for systemLanguage conditional test");
      return false;
    }

    const nsDefaultStringComparator defaultComparator;

    for (uint32_t i = 0; i < mStringListAttributes[LANGUAGE].Length(); i++) {
      nsCharSeparatedTokenizer languageTokenizer(acceptLangs, ',');
      while (languageTokenizer.hasMoreTokens()) {
        if (nsStyleUtil::DashMatchCompare(mStringListAttributes[LANGUAGE][i],
                                          languageTokenizer.nextToken(),
                                          defaultComparator)) {
          return true;
        }
      }
    }
    return false;
  }

  return true;
}

bool
SVGTests::ParseConditionalProcessingAttribute(nsIAtom* aAttribute,
                                              const nsAString& aValue,
                                              nsAttrValue& aResult)
{
  for (uint32_t i = 0; i < ArrayLength(sStringListNames); i++) {
    if (aAttribute == *sStringListNames[i]) {
      nsresult rv = mStringListAttributes[i].SetValue(aValue);
      if (NS_FAILED(rv)) {
        mStringListAttributes[i].Clear();
      }
      MaybeInvalidate();
      return true;
    }
  }
  return false;
}

void
SVGTests::UnsetAttr(const nsIAtom* aAttribute)
{
  for (uint32_t i = 0; i < ArrayLength(sStringListNames); i++) {
    if (aAttribute == *sStringListNames[i]) {
      mStringListAttributes[i].Clear();
      MaybeInvalidate();
      return;
    }
  }
}

nsIAtom*
SVGTests::GetAttrName(uint8_t aAttrEnum) const
{
  return *sStringListNames[aAttrEnum];
}

void
SVGTests::GetAttrValue(uint8_t aAttrEnum, nsAttrValue& aValue) const
{
  MOZ_ASSERT(aAttrEnum < ArrayLength(sStringListNames),
             "aAttrEnum out of range");
  aValue.SetTo(mStringListAttributes[aAttrEnum], nullptr);
}

void
SVGTests::MaybeInvalidate()
{
  nsCOMPtr<nsIDOMSVGElement> elem = do_QueryInterface(this);
  nsSVGElement* element = static_cast<nsSVGElement*>(elem.get());

  nsIContent* parent = element->GetFlattenedTreeParent();

  if (parent &&
      parent->NodeInfo()->Equals(nsGkAtoms::svgSwitch, kNameSpaceID_SVG)) {
    static_cast<dom::SVGSwitchElement*>(parent)->MaybeInvalidate();
  }
}

} 
} 
