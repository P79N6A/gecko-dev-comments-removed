




































#ifndef nsIXULDocument_h___
#define nsIXULDocument_h___

#include "nsISupports.h"
#include "nsString.h"
#include "nsCOMArray.h"

class nsIXULTemplateBuilder;
class nsIContent;
class nsIScriptGlobalObjectOwner;



#define NS_IXULDOCUMENT_IID \
{ 0xe486ea2a, 0x5b37, 0x4107, \
  { 0x90, 0x5f, 0xee, 0x06, 0x2f, 0xb4, 0xff, 0x97 }}







class nsIXULDocument : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXULDOCUMENT_IID)

  
  

  


  NS_IMETHOD AddElementForID(nsIContent* aElement) = 0;

  




  NS_IMETHOD GetElementsForID(const nsAString& aID, nsCOMArray<nsIContent>& aElements) = 0;

  


  NS_IMETHOD GetScriptGlobalObjectOwner(nsIScriptGlobalObjectOwner** aGlobalOwner) = 0;

  


  NS_IMETHOD AddSubtreeToDocument(nsIContent* aElement) = 0;

  


  NS_IMETHOD RemoveSubtreeFromDocument(nsIContent* aElement) = 0;

  




  NS_IMETHOD SetTemplateBuilderFor(nsIContent* aContent, nsIXULTemplateBuilder* aBuilder) = 0;

  



  NS_IMETHOD GetTemplateBuilderFor(nsIContent* aContent, nsIXULTemplateBuilder** aResult) = 0;

  









  NS_IMETHOD OnPrototypeLoadDone(PRBool aResumeWalk) = 0;

  


  virtual PRBool OnDocumentParserError() = 0;

  


  virtual void ResetDocumentDirection() = 0;

  virtual void ResetDocumentLWTheme() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXULDocument, NS_IXULDOCUMENT_IID)


nsresult NS_NewXULDocument(nsIXULDocument** result);

#endif 
