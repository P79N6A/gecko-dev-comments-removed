




































#include "nsAppShellWindowEnumerator.h"

#include "nsIContentViewer.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindow.h"
#include "nsIFactory.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIXULWindow.h"

#include "nsWindowMediator.h"





static nsresult GetDOMWindow(nsIXULWindow* inWindow,
                             nsCOMPtr<nsIDOMWindow> &outDOMWindow);
static nsCOMPtr<nsIDOMNode> GetDOMNodeFromDocShell(nsIDocShell *aShell);
static void GetAttribute(nsIXULWindow *inWindow, const nsAString &inAttribute,
                         nsAString &outValue);
static void GetWindowType(nsIXULWindow* inWindow, nsString &outType);


nsresult GetDOMWindow(nsIXULWindow *aWindow, nsCOMPtr<nsIDOMWindow> &aDOMWindow)
{
  nsCOMPtr<nsIDocShell> docShell;

  aWindow->GetDocShell(getter_AddRefs(docShell));
  aDOMWindow = do_GetInterface(docShell);
  return aDOMWindow ? NS_OK : NS_ERROR_FAILURE;
}

nsCOMPtr<nsIDOMNode> GetDOMNodeFromDocShell(nsIDocShell *aShell)
{
  nsCOMPtr<nsIDOMNode> node;

  nsCOMPtr<nsIContentViewer> cv;
  aShell->GetContentViewer(getter_AddRefs(cv));
  if (cv) {
    nsCOMPtr<nsIDOMDocument> domdoc(do_QueryInterface(cv->GetDocument()));
    if (domdoc) {
      nsCOMPtr<nsIDOMElement> element;
      domdoc->GetDocumentElement(getter_AddRefs(element));
      if (element)
        node = element;
    }
  }

  return node;
}


void GetAttribute(nsIXULWindow *inWindow, const nsAString &inAttribute,
                  nsAString &outValue)
{
  nsCOMPtr<nsIDocShell> shell;
  if (inWindow && NS_SUCCEEDED(inWindow->GetDocShell(getter_AddRefs(shell)))) {
    nsCOMPtr<nsIDOMNode> node(GetDOMNodeFromDocShell(shell));
    if (node) {
      nsCOMPtr<nsIDOMElement> webshellElement(do_QueryInterface(node));
      if (webshellElement)
        webshellElement->GetAttribute(inAttribute, outValue);
    }
  }
}



void GetWindowType(nsIXULWindow* aWindow, nsString &outType)
{
  GetAttribute(aWindow, NS_LITERAL_STRING("windowtype"), outType);
}





nsWindowInfo::nsWindowInfo(nsIXULWindow* inWindow, PRInt32 inTimeStamp) :
  mWindow(inWindow),mTimeStamp(inTimeStamp),mZLevel(nsIXULWindow::normalZ)
{
  ReferenceSelf(PR_TRUE, PR_TRUE);
}

nsWindowInfo::~nsWindowInfo()
{
}



bool nsWindowInfo::TypeEquals(const nsAString &aType)
{ 
  nsAutoString rtnString;
  GetWindowType(mWindow, rtnString);
  return rtnString == aType;
}



void nsWindowInfo::InsertAfter(nsWindowInfo *inOlder , nsWindowInfo *inHigher)
{
  if (inOlder) {
    mOlder = inOlder;
    mYounger = inOlder->mYounger;
    mOlder->mYounger = this;
    if (mOlder->mOlder == mOlder)
      mOlder->mOlder = this;
    mYounger->mOlder = this;
    if (mYounger->mYounger == mYounger)
      mYounger->mYounger = this;
  }
  if (inHigher) {
    mHigher = inHigher;
    mLower = inHigher->mLower;
    mHigher->mLower = this;
    if (mHigher->mHigher == mHigher)
      mHigher->mHigher = this;
    mLower->mHigher = this;
    if (mLower->mLower == mLower)
      mLower->mLower = this;
  }
}


void nsWindowInfo::Unlink(bool inAge, bool inZ)
{
  if (inAge) {
    mOlder->mYounger = mYounger;
    mYounger->mOlder = mOlder;
  }
  if (inZ) {
    mLower->mHigher = mHigher;
    mHigher->mLower = mLower;
  }
  ReferenceSelf(inAge, inZ);
}


void nsWindowInfo::ReferenceSelf(bool inAge, bool inZ)
{
  if (inAge) {
    mYounger = this;
    mOlder = this;
  }
  if (inZ) {
    mLower = this;
    mHigher = this;
  }
}





NS_IMPL_ISUPPORTS1(nsAppShellWindowEnumerator, nsISimpleEnumerator)

nsAppShellWindowEnumerator::nsAppShellWindowEnumerator(
    const PRUnichar* aTypeString,
    nsWindowMediator& aMediator) :
      mWindowMediator(&aMediator), mType(aTypeString), mCurrentPosition(nsnull)
{
  mWindowMediator->AddEnumerator(this);
  NS_ADDREF(mWindowMediator);
}

nsAppShellWindowEnumerator::~nsAppShellWindowEnumerator()
{
  mWindowMediator->RemoveEnumerator(this);
  NS_RELEASE(mWindowMediator);
}



void nsAppShellWindowEnumerator::AdjustInitialPosition()
{
  if (!mType.IsEmpty() && mCurrentPosition && !mCurrentPosition->TypeEquals(mType))
    mCurrentPosition = FindNext();
}

NS_IMETHODIMP nsAppShellWindowEnumerator::HasMoreElements(bool *retval)
{
  if (!retval)
    return NS_ERROR_INVALID_ARG;

  *retval = mCurrentPosition ? PR_TRUE : PR_FALSE;
  return NS_OK;
}


void nsAppShellWindowEnumerator::WindowRemoved(nsWindowInfo *inInfo)
{
  if (mCurrentPosition == inInfo)
    mCurrentPosition = FindNext();
}





nsASDOMWindowEnumerator::nsASDOMWindowEnumerator(
    const PRUnichar* aTypeString,
    nsWindowMediator& aMediator) :
      nsAppShellWindowEnumerator(aTypeString, aMediator)
{
}

nsASDOMWindowEnumerator::~nsASDOMWindowEnumerator()
{
}

NS_IMETHODIMP nsASDOMWindowEnumerator::GetNext(nsISupports **retval)
{
  if (!retval)
    return NS_ERROR_INVALID_ARG;

  *retval = nsnull;
  if (mCurrentPosition) {
    nsCOMPtr<nsIDOMWindow> domWindow;
    GetDOMWindow(mCurrentPosition->mWindow, domWindow);
    CallQueryInterface(domWindow, retval);
    mCurrentPosition = FindNext();
  }
  return NS_OK;
}





nsASXULWindowEnumerator::nsASXULWindowEnumerator(
    const PRUnichar* aTypeString,
    nsWindowMediator& aMediator) :
      nsAppShellWindowEnumerator(aTypeString, aMediator)
{
}

nsASXULWindowEnumerator::~nsASXULWindowEnumerator()
{
}

NS_IMETHODIMP nsASXULWindowEnumerator::GetNext(nsISupports **retval)
{
  if (!retval)
    return NS_ERROR_INVALID_ARG;

  *retval = nsnull;
  if (mCurrentPosition) {
    CallQueryInterface(mCurrentPosition->mWindow, retval);
    mCurrentPosition = FindNext();
  }
  return NS_OK;
}





nsASDOMWindowEarlyToLateEnumerator::nsASDOMWindowEarlyToLateEnumerator(
    const PRUnichar *aTypeString,
    nsWindowMediator &aMediator) :
      nsASDOMWindowEnumerator(aTypeString, aMediator)
{
  mCurrentPosition = aMediator.mOldestWindow;
  AdjustInitialPosition();
}

nsASDOMWindowEarlyToLateEnumerator::~nsASDOMWindowEarlyToLateEnumerator()
{
}

nsWindowInfo *nsASDOMWindowEarlyToLateEnumerator::FindNext()
{
  nsWindowInfo *info,
               *listEnd;
  bool          allWindows = mType.IsEmpty();

  
  if (!mCurrentPosition)
    return nsnull;

  info = mCurrentPosition->mYounger;
  listEnd = mWindowMediator->mOldestWindow;

  while (info != listEnd) {
    if (allWindows || info->TypeEquals(mType))
      return info;
    info = info->mYounger;
  }

  return nsnull;
}





nsASXULWindowEarlyToLateEnumerator::nsASXULWindowEarlyToLateEnumerator(
    const PRUnichar *aTypeString,
    nsWindowMediator &aMediator) :
      nsASXULWindowEnumerator(aTypeString, aMediator)
{
  mCurrentPosition = aMediator.mOldestWindow;
  AdjustInitialPosition();
}

nsASXULWindowEarlyToLateEnumerator::~nsASXULWindowEarlyToLateEnumerator()
{
}

nsWindowInfo *nsASXULWindowEarlyToLateEnumerator::FindNext()
{
  nsWindowInfo *info,
               *listEnd;
  bool          allWindows = mType.IsEmpty();

  






  if (!mCurrentPosition)
    return nsnull;

  info = mCurrentPosition->mYounger;
  listEnd = mWindowMediator->mOldestWindow;

  while (info != listEnd) {
    if (allWindows || info->TypeEquals(mType))
      return info;
    info = info->mYounger;
  }

  return nsnull;
}





nsASDOMWindowFrontToBackEnumerator::nsASDOMWindowFrontToBackEnumerator(
    const PRUnichar *aTypeString,
    nsWindowMediator &aMediator) :
      nsASDOMWindowEnumerator(aTypeString, aMediator)
{
  mCurrentPosition = aMediator.mTopmostWindow;
  AdjustInitialPosition();
}

nsASDOMWindowFrontToBackEnumerator::~nsASDOMWindowFrontToBackEnumerator()
{
}

nsWindowInfo *nsASDOMWindowFrontToBackEnumerator::FindNext()
{
  nsWindowInfo *info,
               *listEnd;
  bool          allWindows = mType.IsEmpty();

  
  if (!mCurrentPosition)
    return nsnull;

  info = mCurrentPosition->mLower;
  listEnd = mWindowMediator->mTopmostWindow;

  while (info != listEnd) {
    if (allWindows || info->TypeEquals(mType))
      return info;
    info = info->mLower;
  }

  return nsnull;
}





nsASXULWindowFrontToBackEnumerator::nsASXULWindowFrontToBackEnumerator(
    const PRUnichar *aTypeString,
    nsWindowMediator &aMediator) :
      nsASXULWindowEnumerator(aTypeString, aMediator)
{
  mCurrentPosition = aMediator.mTopmostWindow;
  AdjustInitialPosition();
}

nsASXULWindowFrontToBackEnumerator::~nsASXULWindowFrontToBackEnumerator()
{
}

nsWindowInfo *nsASXULWindowFrontToBackEnumerator::FindNext()
{
  nsWindowInfo *info,
               *listEnd;
  bool          allWindows = mType.IsEmpty();

  
  if (!mCurrentPosition)
    return nsnull;

  info = mCurrentPosition->mLower;
  listEnd = mWindowMediator->mTopmostWindow;

  while (info != listEnd) {
    if (allWindows || info->TypeEquals(mType))
      return info;
    info = info->mLower;
  }

  return nsnull;
}





nsASDOMWindowBackToFrontEnumerator::nsASDOMWindowBackToFrontEnumerator(
    const PRUnichar *aTypeString,
    nsWindowMediator &aMediator) :
  nsASDOMWindowEnumerator(aTypeString, aMediator)
{
  mCurrentPosition = aMediator.mTopmostWindow ?
                     aMediator.mTopmostWindow->mHigher : nsnull;
  AdjustInitialPosition();
}

nsASDOMWindowBackToFrontEnumerator::~nsASDOMWindowBackToFrontEnumerator()
{
}

nsWindowInfo *nsASDOMWindowBackToFrontEnumerator::FindNext()
{
  nsWindowInfo *info,
               *listEnd;
  bool          allWindows = mType.IsEmpty();

  
  if (!mCurrentPosition)
    return nsnull;

  info = mCurrentPosition->mHigher;
  listEnd = mWindowMediator->mTopmostWindow;
  if (listEnd)
    listEnd = listEnd->mHigher;

  while (info != listEnd) {
    if (allWindows || info->TypeEquals(mType))
      return info;
    info = info->mHigher;
  }

  return nsnull;
}





nsASXULWindowBackToFrontEnumerator::nsASXULWindowBackToFrontEnumerator(
    const PRUnichar *aTypeString,
    nsWindowMediator &aMediator) :
      nsASXULWindowEnumerator(aTypeString, aMediator)
{
  mCurrentPosition = aMediator.mTopmostWindow ?
                     aMediator.mTopmostWindow->mHigher : nsnull;
  AdjustInitialPosition();
}

nsASXULWindowBackToFrontEnumerator::~nsASXULWindowBackToFrontEnumerator()
{
}

nsWindowInfo *nsASXULWindowBackToFrontEnumerator::FindNext()
{
  nsWindowInfo *info,
               *listEnd;
  bool          allWindows = mType.IsEmpty();

  
  if (!mCurrentPosition)
    return nsnull;

  info = mCurrentPosition->mHigher;
  listEnd = mWindowMediator->mTopmostWindow;
  if (listEnd)
    listEnd = listEnd->mHigher;

  while (info != listEnd) {
    if (allWindows || info->TypeEquals(mType))
      return info;
    info = info->mHigher;
  }

  return nsnull;
}
