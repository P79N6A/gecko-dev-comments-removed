





































#include "nsRootAccessibleWrap.h"

#include "nsWinUtils.h"

#include "nsIDOMEventTarget.h"
#include "nsEventListenerManager.h"




nsRootAccessibleWrap::
  nsRootAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                       nsIWeakReference* aShell) :
  nsRootAccessible(aDocument, aRootContent, aShell)
{
}

nsRootAccessibleWrap::~nsRootAccessibleWrap()
{
}




void
nsRootAccessibleWrap::DocumentActivated(nsDocAccessible* aDocument)
{
  if (nsWinUtils::IsWindowEmulationFor(kDolphinModuleHandle) &&
      nsWinUtils::IsTabDocument(aDocument->GetDocumentNode())) {
    PRUint32 count = mChildDocuments.Length();
    for (PRUint32 idx = 0; idx < count; idx++) {
      nsDocAccessible* childDoc = mChildDocuments[idx];
      HWND childDocHWND = static_cast<HWND>(childDoc->GetNativeWindow());
      if (childDoc != aDocument)
        nsWinUtils::HideNativeWindow(childDocHWND);
      else
        nsWinUtils::ShowNativeWindow(childDocHWND);
    }
  }
}
