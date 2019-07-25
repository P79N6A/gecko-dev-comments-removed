





































#include "RootAccessibleWrap.h"

#include "Compatibility.h"
#include "nsWinUtils.h"

#include "nsIDOMEventTarget.h"
#include "nsEventListenerManager.h"

using namespace mozilla::a11y;




RootAccessibleWrap::
  RootAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                     nsIPresShell* aPresShell) :
  RootAccessible(aDocument, aRootContent, aPresShell)
{
}

RootAccessibleWrap::~RootAccessibleWrap()
{
}




void
RootAccessibleWrap::DocumentActivated(nsDocAccessible* aDocument)
{
  if (Compatibility::IsDolphin() &&
      nsCoreUtils::IsTabDocument(aDocument->GetDocumentNode())) {
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
