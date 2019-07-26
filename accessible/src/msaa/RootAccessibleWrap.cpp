




#include "RootAccessibleWrap.h"

#include "Compatibility.h"
#include "nsCoreUtils.h"
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
RootAccessibleWrap::DocumentActivated(DocAccessible* aDocument)
{
  if (Compatibility::IsDolphin() &&
      nsCoreUtils::IsTabDocument(aDocument->DocumentNode())) {
    uint32_t count = mChildDocuments.Length();
    for (uint32_t idx = 0; idx < count; idx++) {
      DocAccessible* childDoc = mChildDocuments[idx];
      HWND childDocHWND = static_cast<HWND>(childDoc->GetNativeWindow());
      if (childDoc != aDocument)
        nsWinUtils::HideNativeWindow(childDocHWND);
      else
        nsWinUtils::ShowNativeWindow(childDocHWND);
    }
  }
}
