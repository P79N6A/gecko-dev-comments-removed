




#ifndef nsIXULDocument_h___
#define nsIXULDocument_h___

#include "nsISupports.h"
#include "nsString.h"
#include "nsCOMArray.h"

class nsIXULTemplateBuilder;
class nsIContent;



#define NS_IXULDOCUMENT_IID \
  {0x81ba4be5, 0x6cc5, 0x478a, {0x9b, 0x08, 0xb3, 0xe7, 0xed, 0x52, 0x44, 0x55}}







class nsIXULDocument : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXULDOCUMENT_IID)

  




  virtual void GetElementsForID(const nsAString& aID, nsCOMArray<nsIContent>& aElements) = 0;

  


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
