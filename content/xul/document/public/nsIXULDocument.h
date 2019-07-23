




































#ifndef nsIXULDocument_h___
#define nsIXULDocument_h___

#include "nsISupports.h"
#include "nsString.h"
#include "nsCOMArray.h"

class nsForwardReference;
class nsIAtom;
class nsIDOMElement;
class nsIPrincipal;
class nsIRDFResource;
class nsISupportsArray;
class nsIXULTemplateBuilder;
class nsIURI;
class nsIContent;
class nsIRDFDataSource;
class nsIScriptGlobalObjectOwner;


#define NS_IXULDOCUMENT_IID \
{ 0x01c4fe87, 0x961f, 0x4194, \
  { 0xa2, 0x1d, 0x04, 0xf4, 0x38, 0x7c, 0x4b, 0xb3 } }







class nsIXULDocument : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXULDOCUMENT_IID)

  
  

  


  NS_IMETHOD AddElementForID(const nsAString& aID, nsIContent* aElement) = 0;

  


  NS_IMETHOD RemoveElementForID(const nsAString& aID, nsIContent* aElement) = 0;

  




  NS_IMETHOD GetElementsForID(const nsAString& aID, nsCOMArray<nsIContent>& aElements) = 0;

  



  NS_IMETHOD AddForwardReference(nsForwardReference* aForwardReference) = 0;

  


  NS_IMETHOD ResolveForwardReferences() = 0;

  


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
