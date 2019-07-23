




































#ifndef nsHTMLFormatConverter_h__
#define nsHTMLFormatConverter_h__

#include "nsIFormatConverter.h"
#include "nsString.h"

class nsHTMLFormatConverter : public nsIFormatConverter
{
public:

  nsHTMLFormatConverter();
  virtual ~nsHTMLFormatConverter();

    
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFORMATCONVERTER

protected:

  nsresult AddFlavorToList ( nsISupportsArray* inList, const char* inFlavor ) ;

  NS_IMETHOD ConvertFromHTMLToUnicode(const nsAutoString & aFromStr, nsAutoString & aToStr);
  NS_IMETHOD ConvertFromHTMLToAOLMail(const nsAutoString & aFromStr, nsAutoString & aToStr);

};

#endif 
