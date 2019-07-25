






































#include "nsCOMPtr.h"
#include "nsPIListBoxObject.h"
#include "nsBoxObject.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsBindingManager.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsGkAtoms.h"
#include "nsIScrollableFrame.h"
#include "nsListBoxBodyFrame.h"

class nsListBoxObject : public nsPIListBoxObject, public nsBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSILISTBOXOBJECT

  
  virtual nsListBoxBodyFrame* GetListBoxBody(bool aFlush);

  nsListBoxObject();

  
  virtual void Clear();
  virtual void ClearCachedValues();
  
protected:
  nsListBoxBodyFrame *mListBoxBody;
};

NS_IMPL_ISUPPORTS_INHERITED2(nsListBoxObject, nsBoxObject, nsIListBoxObject,
                             nsPIListBoxObject)

nsListBoxObject::nsListBoxObject()
  : mListBoxBody(nsnull)
{
}




NS_IMETHODIMP
nsListBoxObject::GetRowCount(PRInt32 *aResult)
{
  nsListBoxBodyFrame* body = GetListBoxBody(true);
  if (body)
    return body->GetRowCount(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxObject::GetNumberOfVisibleRows(PRInt32 *aResult)
{
  nsListBoxBodyFrame* body = GetListBoxBody(true);
  if (body)
    return body->GetNumberOfVisibleRows(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxObject::GetIndexOfFirstVisibleRow(PRInt32 *aResult)
{
  nsListBoxBodyFrame* body = GetListBoxBody(true);
  if (body)
    return body->GetIndexOfFirstVisibleRow(aResult);
  return NS_OK;
}

NS_IMETHODIMP nsListBoxObject::EnsureIndexIsVisible(PRInt32 aRowIndex)
{
  nsListBoxBodyFrame* body = GetListBoxBody(true);
  if (body)
    return body->EnsureIndexIsVisible(aRowIndex);
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxObject::ScrollToIndex(PRInt32 aRowIndex)
{
  nsListBoxBodyFrame* body = GetListBoxBody(true);
  if (body)
    return body->ScrollToIndex(aRowIndex);
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxObject::ScrollByLines(PRInt32 aNumLines)
{
  nsListBoxBodyFrame* body = GetListBoxBody(true);
  if (body)
    return body->ScrollByLines(aNumLines);
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxObject::GetItemAtIndex(PRInt32 index, nsIDOMElement **_retval)
{
  nsListBoxBodyFrame* body = GetListBoxBody(true);
  if (body)
    return body->GetItemAtIndex(index, _retval);
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxObject::GetIndexOfItem(nsIDOMElement* aElement, PRInt32 *aResult)
{
  *aResult = 0;

  nsListBoxBodyFrame* body = GetListBoxBody(true);
  if (body)
    return body->GetIndexOfItem(aElement, aResult);
  return NS_OK;
}



static void
FindBodyContent(nsIContent* aParent, nsIContent** aResult)
{
  if (aParent->Tag() == nsGkAtoms::listboxbody) {
    *aResult = aParent;
    NS_IF_ADDREF(*aResult);
  }
  else {
    nsCOMPtr<nsIDOMNodeList> kids;
    aParent->GetOwnerDoc()->BindingManager()->GetXBLChildNodesFor(aParent, getter_AddRefs(kids));
    if (!kids) return;

    PRUint32 i;
    kids->GetLength(&i);
    
    while (i > 0) {
      nsCOMPtr<nsIDOMNode> childNode;
      kids->Item(--i, getter_AddRefs(childNode));
      nsCOMPtr<nsIContent> childContent(do_QueryInterface(childNode));
      FindBodyContent(childContent, aResult);
      if (*aResult)
        break;
    }
  }
}

nsListBoxBodyFrame*
nsListBoxObject::GetListBoxBody(bool aFlush)
{
  if (mListBoxBody) {
    return mListBoxBody;
  }

  nsIPresShell* shell = GetPresShell(false);
  if (!shell) {
    return nsnull;
  }

  nsIFrame* frame = aFlush ? 
                      GetFrame(false)  :
                      mContent->GetPrimaryFrame();
  if (!frame)
    return nsnull;

  
  nsCOMPtr<nsIContent> content;
  FindBodyContent(frame->GetContent(), getter_AddRefs(content));

  if (!content)
    return nsnull;

  
  frame = content->GetPrimaryFrame();
  if (!frame)
     return nsnull;
  nsIScrollableFrame* scrollFrame = do_QueryFrame(frame);
  if (!scrollFrame)
    return nsnull;

  
  nsIFrame* yeahBaby = scrollFrame->GetScrolledFrame();
  if (!yeahBaby)
     return nsnull;

  
  nsListBoxBodyFrame* listBoxBody = do_QueryFrame(yeahBaby);
  NS_ENSURE_TRUE(listBoxBody &&
                 listBoxBody->SetBoxObject(this),
                 nsnull);
  mListBoxBody = listBoxBody;
  return mListBoxBody;
}

void
nsListBoxObject::Clear()
{
  ClearCachedValues();

  nsBoxObject::Clear();
}

void
nsListBoxObject::ClearCachedValues()
{
  mListBoxBody = nsnull;
}



nsresult
NS_NewListBoxObject(nsIBoxObject** aResult)
{
  *aResult = new nsListBoxObject;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}
