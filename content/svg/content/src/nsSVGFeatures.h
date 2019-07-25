





































#ifndef __NS_SVGFEATURES_H__
#define __NS_SVGFEATURES_H__

#include "nsString.h"

class nsIContent;
class nsIAtom;

class nsSVGFeatures
{
public:
  







  static bool
  HaveFeature(nsISupports* aObject, const nsAString& aFeature);

  










  static int
  GetBestLanguagePreferenceRank(const nsSubstring& aAttribute,
                                const nsSubstring& aAcceptLangs);

  



  static const nsString * const kIgnoreSystemLanguage;

  











  static bool
  PassesConditionalProcessingTests(nsIContent *aContent,
                                   const nsString *aAcceptLangs = nsnull);

private:
  







  static bool
  HaveFeatures(nsISupports* aObject, const nsSubstring& aFeatures);

  





  static bool
  HaveExtension(const nsAString& aExtension);

  





  static bool
  HaveExtensions(const nsSubstring& aExtensions);

  








  static bool
  MatchesLanguagePreferences(const nsSubstring& aAttribute,
                             const nsSubstring& aAcceptLangs); 

  








  static bool
  ElementSupportsAttributes(const nsIAtom *aTagName, PRUint16 aAttr);
};

#endif 
