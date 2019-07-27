




#include "mozilla/dom/ContainerBoxObject.h"
#include "mozilla/dom/ContainerBoxObjectBinding.h"
#include "nsCOMPtr.h"
#include "nsIDocShell.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsSubDocumentFrame.h"

namespace mozilla {
namespace dom {
  
ContainerBoxObject::ContainerBoxObject()
{
}

ContainerBoxObject::~ContainerBoxObject()
{
}

JSObject*
ContainerBoxObject::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return ContainerBoxObjectBinding::Wrap(aCx, this, aGivenProto);
}

already_AddRefed<nsIDocShell>
ContainerBoxObject::GetDocShell()
{
  nsSubDocumentFrame *subDocFrame = do_QueryFrame(GetFrame(false));
  if (subDocFrame) {
    
    
    nsCOMPtr<nsIDocShell> ret;
    subDocFrame->GetDocShell(getter_AddRefs(ret));
    return ret.forget();
  }

  if (!mContent) {
    return nullptr;
  }

  
  

  nsIDocument *doc = mContent->GetComposedDoc();

  if (!doc) {
    return nullptr;
  }

  nsIDocument *sub_doc = doc->GetSubDocumentFor(mContent);

  if (!sub_doc) {
    return nullptr;
  }

  nsCOMPtr<nsIDocShell> result = sub_doc->GetDocShell();
  return result.forget();
}

} 
} 

nsresult
NS_NewContainerBoxObject(nsIBoxObject** aResult)
{
  NS_ADDREF(*aResult = new mozilla::dom::ContainerBoxObject());
  return NS_OK;
}
