











































#ifndef nsIXBLDocumentInfo_h__
#define nsIXBLDocumentInfo_h__

#include "nsISupports.h"

class nsIContent;
class nsIDocument;
class nsIScriptContext;
class nsXBLPrototypeBinding;
class nsIURI;
class nsACString;


#define NS_IXBLDOCUMENTINFO_IID \
{ 0x2d8334b0, 0xe0b8, 0x4a23, { 0x91, 0x52, 0xdc, 0x89, 0xac, 0x27, 0x58, 0x94 } }

class nsIXBLDocumentInfo : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXBLDOCUMENTINFO_IID)

  NS_IMETHOD GetDocument(nsIDocument** aResult)=0;
  
  NS_IMETHOD GetScriptAccess(PRBool* aResult)=0;

  
  NS_IMETHOD_(nsIURI*) DocumentURI()=0;

  NS_IMETHOD GetPrototypeBinding(const nsACString& aRef, nsXBLPrototypeBinding** aResult)=0;
  NS_IMETHOD SetPrototypeBinding(const nsACString& aRef, nsXBLPrototypeBinding* aBinding)=0;

  NS_IMETHOD SetFirstPrototypeBinding(nsXBLPrototypeBinding* aBinding)=0;

  NS_IMETHOD FlushSkinStylesheets()=0;

  
  NS_IMETHOD_(PRBool) IsChrome()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXBLDocumentInfo, NS_IXBLDOCUMENTINFO_IID)

nsresult
NS_NewXBLDocumentInfo(nsIDocument* aDocument, nsIXBLDocumentInfo** aResult);

#endif 
