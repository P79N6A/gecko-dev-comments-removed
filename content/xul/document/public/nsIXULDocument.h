




































#ifndef nsIXULDocument_h___
#define nsIXULDocument_h___

#include "nsISupports.h"
#include "nsString.h"
#include "nsCOMArray.h"

class nsIXULTemplateBuilder;
class nsIContent;
class nsIScriptGlobalObjectOwner;



#define NS_IXULDOCUMENT_IID \
{ 0x3e872e97, 0xb678, 0x418e, \
  { 0xa7, 0xe3, 0x41, 0xb8, 0x30, 0x5d, 0x4e, 0x75 } }







class nsIXULDocument : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXULDOCUMENT_IID)

  




  virtual void GetElementsForID(const nsAString& aID, nsCOMArray<nsIContent>& aElements) = 0;

  


  NS_IMETHOD GetScriptGlobalObjectOwner(nsIScriptGlobalObjectOwner** aGlobalOwner) = 0;

  


  NS_IMETHOD AddSubtreeToDocument(nsIContent* aElement) = 0;

  


  NS_IMETHOD RemoveSubtreeFromDocument(nsIContent* aElement) = 0;

  




  NS_IMETHOD SetTemplateBuilderFor(nsIContent* aContent, nsIXULTemplateBuilder* aBuilder) = 0;

  



  NS_IMETHOD GetTemplateBuilderFor(nsIContent* aContent, nsIXULTemplateBuilder** aResult) = 0;

  









  NS_IMETHOD OnPrototypeLoadDone(bool aResumeWalk) = 0;

  


  virtual bool OnDocumentParserError() = 0;

  


  virtual void ResetDocumentDirection() = 0;

  virtual void ResetDocumentLWTheme() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXULDocument, NS_IXULDOCUMENT_IID)


nsresult NS_NewXULDocument(nsIXULDocument** result);

#endif 
