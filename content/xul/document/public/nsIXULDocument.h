




































#ifndef nsIXULDocument_h___
#define nsIXULDocument_h___

#include "nsISupports.h"
#include "nsString.h"
#include "nsCOMArray.h"

class nsIXULTemplateBuilder;
class nsIContent;
class nsIScriptGlobalObjectOwner;



#define NS_IXULDOCUMENT_IID \
{ 0x57314526, 0xf749, 0x4cf0, \
  { 0xb6, 0xb6, 0x37, 0x23, 0xeb, 0xa2, 0x14, 0x80 } }







class nsIXULDocument : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXULDOCUMENT_IID)

  
  

  


  NS_IMETHOD AddElementForID(const nsAString& aID, nsIContent* aElement) = 0;

  


  NS_IMETHOD RemoveElementForID(const nsAString& aID, nsIContent* aElement) = 0;

  




  NS_IMETHOD GetElementsForID(const nsAString& aID, nsCOMArray<nsIContent>& aElements) = 0;

  


  NS_IMETHOD GetScriptGlobalObjectOwner(nsIScriptGlobalObjectOwner** aGlobalOwner) = 0;

  


  NS_IMETHOD AddSubtreeToDocument(nsIContent* aElement) = 0;

  


  NS_IMETHOD RemoveSubtreeFromDocument(nsIContent* aElement) = 0;

  




  NS_IMETHOD SetTemplateBuilderFor(nsIContent* aContent, nsIXULTemplateBuilder* aBuilder) = 0;

  



  NS_IMETHOD GetTemplateBuilderFor(nsIContent* aContent, nsIXULTemplateBuilder** aResult) = 0;

  









  NS_IMETHOD OnPrototypeLoadDone(PRBool aResumeWalk) = 0;

  


  virtual PRBool OnDocumentParserError() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXULDocument, NS_IXULDOCUMENT_IID)


nsresult NS_NewXULDocument(nsIXULDocument** result);

#endif 
