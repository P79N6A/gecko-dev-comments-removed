





































#ifndef __NS_SVGFEATURES_H__
#define __NS_SVGFEATURES_H__

#include "nsString.h"

class nsIContent;
class nsIAtom;

class nsSVGFeatures
{
public:
  







  static PRBool
  HaveFeature(nsISupports* aObject, const nsAString& aFeature);

  










  static int
  GetBestLanguagePreferenceRank(const nsSubstring& aAttribute,
                                const nsSubstring& aAcceptLangs);

  



  static const nsString * const kIgnoreSystemLanguage;

  











  static PRBool
  PassesConditionalProcessingTests(nsIContent *aContent,
                                   const nsString *aAcceptLangs = nsnull);

private:
  







  static PRBool
  HaveFeatures(nsISupports* aObject, const nsSubstring& aFeatures);

  





  static PRBool
  HaveExtension(const nsAString& aExtension);

  





  static PRBool
  HaveExtensions(const nsSubstring& aExtensions);

  








  static PRBool
  MatchesLanguagePreferences(const nsSubstring& aAttribute,
                             const nsSubstring& aAcceptLangs); 

  








  static PRBool
  ElementSupportsAttributes(const nsIAtom *aTagName, PRUint16 aAttr);
};

#endif 
