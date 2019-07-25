




































#include "DOMSVGTests.h"
#include "DOMSVGStringList.h"
#include "nsSVGFeatures.h"
#include "nsSVGSwitchElement.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsStyleUtil.h"
#include "nsSVGUtils.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS1(DOMSVGTests, nsIDOMSVGTests)

DOMSVGTests::StringListInfo DOMSVGTests::sStringListInfo[3] =
{
  { &nsGkAtoms::requiredFeatures, false },
  { &nsGkAtoms::requiredExtensions, false },
  { &nsGkAtoms::systemLanguage, true }
};


NS_IMETHODIMP
DOMSVGTests::GetRequiredFeatures(nsIDOMSVGStringList * *aRequiredFeatures)
{
  nsCOMPtr<nsSVGElement> element = do_QueryInterface(this);
  *aRequiredFeatures = DOMSVGStringList::GetDOMWrapper(
                         &mStringListAttributes[FEATURES], element, true, FEATURES).get();
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTests::GetRequiredExtensions(nsIDOMSVGStringList * *aRequiredExtensions)
{
  nsCOMPtr<nsSVGElement> element = do_QueryInterface(this);
  *aRequiredExtensions = DOMSVGStringList::GetDOMWrapper(
                           &mStringListAttributes[EXTENSIONS], element, true, EXTENSIONS).get();
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTests::GetSystemLanguage(nsIDOMSVGStringList * *aSystemLanguage)
{
  nsCOMPtr<nsSVGElement> element = do_QueryInterface(this);
  *aSystemLanguage = DOMSVGStringList::GetDOMWrapper(
                       &mStringListAttributes[LANGUAGE], element, true, LANGUAGE).get();
  return NS_OK;
}


NS_IMETHODIMP
DOMSVGTests::HasExtension(const nsAString & extension, bool *_retval)
{
  *_retval = nsSVGFeatures::HasExtension(extension);
  return NS_OK;
}

bool
DOMSVGTests::IsConditionalProcessingAttribute(const nsIAtom* aAttribute) const
{
  for (PRUint32 i = 0; i < ArrayLength(sStringListInfo); i++) {
    if (aAttribute == *sStringListInfo[i].mName) {
      return true;
    }
  }
  return false;
}

PRInt32
DOMSVGTests::GetBestLanguagePreferenceRank(const nsSubstring& aAcceptLangs) const
{
  const nsDefaultStringComparator defaultComparator;

  PRInt32 lowestRank = -1;

  for (PRUint32 i = 0; i < mStringListAttributes[LANGUAGE].Length(); i++) {
    nsCharSeparatedTokenizer languageTokenizer(aAcceptLangs, ',');
    PRInt32 index = 0;
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

const nsString * const DOMSVGTests::kIgnoreSystemLanguage = (nsString *) 0x01;

bool
DOMSVGTests::PassesConditionalProcessingTests(const nsString *aAcceptLangs) const
{
  
  if (mStringListAttributes[FEATURES].IsExplicitlySet()) {
    if (mStringListAttributes[FEATURES].IsEmpty()) {
      return false;
    }
    nsCOMPtr<nsIContent> content(
      do_QueryInterface(const_cast<DOMSVGTests*>(this)));

    for (PRUint32 i = 0; i < mStringListAttributes[FEATURES].Length(); i++) {
      if (!nsSVGFeatures::HasFeature(content, mStringListAttributes[FEATURES][i])) {
        return false;
      }
    }
  }

  
  
  
  
  
  
  
  if (mStringListAttributes[EXTENSIONS].IsExplicitlySet()) {
    if (mStringListAttributes[EXTENSIONS].IsEmpty()) {
      return false;
    }
    for (PRUint32 i = 0; i < mStringListAttributes[EXTENSIONS].Length(); i++) {
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

    for (PRUint32 i = 0; i < mStringListAttributes[LANGUAGE].Length(); i++) {
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
DOMSVGTests::ParseConditionalProcessingAttribute(nsIAtom* aAttribute,
                                                 const nsAString& aValue,
                                                 nsAttrValue& aResult)
{
  for (PRUint32 i = 0; i < ArrayLength(sStringListInfo); i++) {
    if (aAttribute == *sStringListInfo[i].mName) {
      nsresult rv = mStringListAttributes[i].SetValue(
                      aValue, sStringListInfo[i].mIsCommaSeparated);
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
DOMSVGTests::GetValue(PRUint8 aAttrEnum, nsAString& aValue) const
{
  NS_ABORT_IF_FALSE(aAttrEnum >= 0 && aAttrEnum < ArrayLength(sStringListInfo),
                    "aAttrEnum out of range");
  mStringListAttributes[aAttrEnum].GetValue(
    aValue, sStringListInfo[aAttrEnum].mIsCommaSeparated);
}

void
DOMSVGTests::UnsetAttr(const nsIAtom* aAttribute)
{
  for (PRUint32 i = 0; i < ArrayLength(sStringListInfo); i++) {
    if (aAttribute == *sStringListInfo[i].mName) {
      mStringListAttributes[i].Clear();
      MaybeInvalidate();
      return;
    }
  }
}

void
DOMSVGTests::DidChangeStringList(PRUint8 aAttrEnum)
{
  NS_ASSERTION(aAttrEnum < ArrayLength(sStringListInfo), "aAttrEnum out of range");

  nsCOMPtr<nsSVGElement> element = do_QueryInterface(this);

  nsAutoString serializedValue;
  GetValue(aAttrEnum, serializedValue);

  nsAttrValue attrValue(serializedValue);
  element->SetParsedAttr(kNameSpaceID_None,
                         *sStringListInfo[aAttrEnum].mName,
                         nsnull, attrValue, true);

  MaybeInvalidate();
}

void
DOMSVGTests::MaybeInvalidate()
{
  nsCOMPtr<nsSVGElement> element = do_QueryInterface(this);

  nsIContent* parent = element->GetFlattenedTreeParent();
  
  if (parent &&
      parent->NodeInfo()->Equals(nsGkAtoms::svgSwitch, kNameSpaceID_SVG)) {
    static_cast<nsSVGSwitchElement*>(parent)->MaybeInvalidate();
  }
}
