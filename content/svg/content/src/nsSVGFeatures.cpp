















































#include "nsString.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsContentUtils.h"
#include "nsWhitespaceTokenizer.h"
#include "nsCommaSeparatedTokenizer.h"
#include "nsStyleUtil.h"
#include "nsSVGUtils.h"







PRBool
NS_SVG_HaveFeature(const nsAString& aFeature)
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







static PRBool
HaveFeatures(const nsSubstring& aFeatures)
{
  nsWhitespaceTokenizer tokenizer(aFeatures);
  while (tokenizer.hasMoreTokens()) {
    if (!NS_SVG_HaveFeature(tokenizer.nextToken())) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}







static PRBool
HaveExtension(const nsAString& aExtension)
{
#define SVG_SUPPORTED_EXTENSION(str) if (aExtension.Equals(NS_LITERAL_STRING(str).get())) return PR_TRUE;
  SVG_SUPPORTED_EXTENSION("http://www.w3.org/1999/xhtml")
#ifdef MOZ_MATHML
  SVG_SUPPORTED_EXTENSION("http://www.w3.org/1998/Math/MathML")
#endif
#undef SVG_SUPPORTED_EXTENSION

  return PR_FALSE;
}







static PRBool
HaveExtensions(const nsSubstring& aExtensions)
{
  nsWhitespaceTokenizer tokenizer(aExtensions);
  while (tokenizer.hasMoreTokens()) {
    if (!HaveExtension(tokenizer.nextToken())) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}










static PRBool
MatchesLanguagePreferences(const nsSubstring& aAttribute, const nsSubstring& aLanguagePreferences) 
{
  const nsDefaultStringComparator defaultComparator;

  nsCommaSeparatedTokenizer attributeTokenizer(aAttribute);

  while (attributeTokenizer.hasMoreTokens()) {
    const nsSubstring &attributeToken = attributeTokenizer.nextToken();
    nsCommaSeparatedTokenizer languageTokenizer(aLanguagePreferences);
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










static PRBool
ElementSupportsAttributes(const nsIAtom *aTagName, PRUint16 aAttr)
{
#define SVG_ELEMENT(_atom, _supports) if (aTagName == nsGkAtoms::_atom) return (_supports & aAttr) != 0;
#include "nsSVGElementList.h"
#undef SVG_ELEMENT
  return PR_FALSE;
}









PRBool
NS_SVG_PassesConditionalProcessingTests(nsIContent *aContent)
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

  
  
  
  
  
  
  
  if (aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::systemLanguage,
                        value)) {
    
    nsAutoString langPrefs(nsContentUtils::GetLocalizedStringPref("intl.accept_languages"));
    if (!langPrefs.IsEmpty()) {
      langPrefs.StripWhitespace();
      value.StripWhitespace();
      return MatchesLanguagePreferences(value, langPrefs);
    } else {
      
      NS_WARNING("no default language specified for systemLanguage conditional test");
      return !value.IsEmpty();
    }
  }

  return PR_TRUE;
}
