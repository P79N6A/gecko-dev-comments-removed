















































#include "nsSVGFeatures.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsContentUtils.h"
#include "nsWhitespaceTokenizer.h"
#include "nsCommaSeparatedTokenizer.h"
#include "nsStyleUtil.h"
#include "nsSVGUtils.h"

 PRBool
nsSVGFeatures::HaveFeature(const nsAString& aFeature)
{
  if (!NS_SVGEnabled()) {
    return PR_FALSE;
  }

#define SVG_SUPPORTED_FEATURE(str) if (aFeature.Equals(NS_LITERAL_STRING(str).get())) return PR_TRUE;
#define SVG_UNSUPPORTED_FEATURE(str)
#include "nsSVGFeaturesList.h"
#undef SVG_SUPPORTED_FEATURE
#undef SVG_UNSUPPORTED_FEATURE
  return PR_FALSE;
}

 PRBool
nsSVGFeatures::HaveFeatures(const nsSubstring& aFeatures)
{
  nsWhitespaceTokenizer tokenizer(aFeatures);
  while (tokenizer.hasMoreTokens()) {
    if (!HaveFeature(tokenizer.nextToken())) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

 PRBool
nsSVGFeatures::HaveExtension(const nsAString& aExtension)
{
#define SVG_SUPPORTED_EXTENSION(str) if (aExtension.Equals(NS_LITERAL_STRING(str).get())) return PR_TRUE;
  SVG_SUPPORTED_EXTENSION("http://www.w3.org/1999/xhtml")
#ifdef MOZ_MATHML
  SVG_SUPPORTED_EXTENSION("http://www.w3.org/1998/Math/MathML")
#endif
#undef SVG_SUPPORTED_EXTENSION

  return PR_FALSE;
}

 PRBool
nsSVGFeatures::HaveExtensions(const nsSubstring& aExtensions)
{
  nsWhitespaceTokenizer tokenizer(aExtensions);
  while (tokenizer.hasMoreTokens()) {
    if (!HaveExtension(tokenizer.nextToken())) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

 PRBool
nsSVGFeatures::MatchesLanguagePreferences(const nsSubstring& aAttribute,
                                          const nsSubstring& aAcceptLangs) 
{
  const nsDefaultStringComparator defaultComparator;

  nsCommaSeparatedTokenizer attributeTokenizer(aAttribute);

  while (attributeTokenizer.hasMoreTokens()) {
    const nsSubstring &attributeToken = attributeTokenizer.nextToken();
    nsCommaSeparatedTokenizer languageTokenizer(aAcceptLangs);
    while (languageTokenizer.hasMoreTokens()) {
      if (nsStyleUtil::DashMatchCompare(attributeToken,
                                        languageTokenizer.nextToken(),
                                        defaultComparator)) {
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

 PRInt32
nsSVGFeatures::GetBestLanguagePreferenceRank(const nsSubstring& aAttribute,
                                             const nsSubstring& aAcceptLangs) 
{
  const nsDefaultStringComparator defaultComparator;

  nsCommaSeparatedTokenizer attributeTokenizer(aAttribute);
  PRInt32 lowestRank = -1;

  while (attributeTokenizer.hasMoreTokens()) {
    const nsSubstring &attributeToken = attributeTokenizer.nextToken();
    nsCommaSeparatedTokenizer languageTokenizer(aAcceptLangs);
    PRInt32 index = 0;
    while (languageTokenizer.hasMoreTokens()) {
      const nsSubstring &languageToken = languageTokenizer.nextToken();
      PRBool exactMatch = (languageToken == attributeToken);
      PRBool prefixOnlyMatch =
        !exactMatch &&
        nsStyleUtil::DashMatchCompare(attributeToken,
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

 PRBool
nsSVGFeatures::ElementSupportsAttributes(const nsIAtom *aTagName, PRUint16 aAttr)
{
#define SVG_ELEMENT(_atom, _supports) if (aTagName == nsGkAtoms::_atom) return (_supports & aAttr) != 0;
#include "nsSVGElementList.h"
#undef SVG_ELEMENT
  return PR_FALSE;
}

const nsString * const nsSVGFeatures::kIgnoreSystemLanguage = (nsString *) 0x01;

 PRBool
nsSVGFeatures::PassesConditionalProcessingTests(nsIContent *aContent,
                                                const nsString *aAcceptLangs)
{
  if (!aContent->IsNodeOfType(nsINode::eELEMENT)) {
    return PR_FALSE;
  }

  if (!ElementSupportsAttributes(aContent->Tag(), ATTRS_CONDITIONAL)) {
    return PR_TRUE;
  }

  
  nsAutoString value;
  if (aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::requiredFeatures, value)) {
    if (value.IsEmpty() || !HaveFeatures(value)) {
      return PR_FALSE;
    }
  }

  
  
  
  
  
  
  
  if (aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::requiredExtensions, value)) {
    if (value.IsEmpty() || !HaveExtensions(value)) {
      return PR_FALSE;
    }
  }

  if (aAcceptLangs == kIgnoreSystemLanguage) {
    return PR_TRUE;
  }

  const nsAutoString acceptLangs(aAcceptLangs ? *aAcceptLangs :
    nsContentUtils::GetLocalizedStringPref("intl.accept_languages"));

  
  
  
  
  
  
  
  if (aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::systemLanguage,
                        value)) {
    
    if (!acceptLangs.IsEmpty()) {
      return MatchesLanguagePreferences(value, acceptLangs);
    } else {
      
      NS_WARNING("no default language specified for systemLanguage conditional test");
      return !value.IsEmpty();
    }
  }

  return PR_TRUE;
}
