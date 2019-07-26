




#include "OuterDocAccessible.h"

#include "Accessible-inl.h"
#include "nsAccUtils.h"
#include "DocAccessible.h"
#include "Role.h"
#include "States.h"

#ifdef A11Y_LOG
#include "Logging.h"
#endif

using namespace mozilla;
using namespace mozilla::a11y;





OuterDocAccessible::
  OuterDocAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}

OuterDocAccessible::~OuterDocAccessible()
{
}




NS_IMPL_ISUPPORTS_INHERITED0(OuterDocAccessible,
                             Accessible)




role
OuterDocAccessible::NativeRole()
{
  return roles::INTERNAL_FRAME;
}

Accessible*
OuterDocAccessible::ChildAtPoint(int32_t aX, int32_t aY,
                                 EWhichChildAtPoint aWhichChild)
{
  int32_t docX = 0, docY = 0, docWidth = 0, docHeight = 0;
  nsresult rv = GetBounds(&docX, &docY, &docWidth, &docHeight);
  NS_ENSURE_SUCCESS(rv, nullptr);

  if (aX < docX || aX >= docX + docWidth || aY < docY || aY >= docY + docHeight)
    return nullptr;

  
  
  Accessible* child = GetChildAt(0);
  NS_ENSURE_TRUE(child, nullptr);

  if (aWhichChild == eDeepestChild)
    return child->ChildAtPoint(aX, aY, eDeepestChild);
  return child;
}




uint8_t
OuterDocAccessible::ActionCount()
{
  
  return 0;
}

NS_IMETHODIMP
OuterDocAccessible::GetActionName(uint8_t aIndex, nsAString& aName)
{
  aName.Truncate();

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
OuterDocAccessible::GetActionDescription(uint8_t aIndex,
                                         nsAString& aDescription)
{
  aDescription.Truncate();

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
OuterDocAccessible::DoAction(uint8_t aIndex)
{
  return NS_ERROR_INVALID_ARG;
}




void
OuterDocAccessible::Shutdown()
{
  
  
  
  
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocDestroy))
    logging::OuterDocDestroy(this);
#endif

  Accessible* childAcc = mChildren.SafeElementAt(0, nullptr);
  if (childAcc) {
#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eDocDestroy)) {
      logging::DocDestroy("outerdoc's child document shutdown",
                          childAcc->AsDoc()->DocumentNode());
    }
#endif
    childAcc->Shutdown();
  }

  AccessibleWrap::Shutdown();
}




void
OuterDocAccessible::InvalidateChildren()
{
  
  
  
  
  
  
  
  

  SetChildrenFlag(eChildrenUninitialized);
}

bool
OuterDocAccessible::InsertChildAt(uint32_t aIdx, Accessible* aAccessible)
{
  NS_ASSERTION(aAccessible->IsDoc(),
               "OuterDocAccessible should only have document child!");
  
  
  
  
  
  if (mChildren.Length())
    mChildren[0]->Shutdown();

  if (!AccessibleWrap::InsertChildAt(0, aAccessible))
    return false;

#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocCreate)) {
    logging::DocCreate("append document to outerdoc",
                       aAccessible->AsDoc()->DocumentNode());
    logging::Address("outerdoc", this);
  }
#endif

  return true;
}

bool
OuterDocAccessible::RemoveChild(Accessible* aAccessible)
{
  Accessible* child = mChildren.SafeElementAt(0, nullptr);
  if (child != aAccessible) {
    NS_ERROR("Wrong child to remove!");
    return false;
  }

#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocDestroy)) {
    logging::DocDestroy("remove document from outerdoc",
                        child->AsDoc()->DocumentNode(), child->AsDoc());
    logging::Address("outerdoc", this);
  }
#endif

  bool wasRemoved = AccessibleWrap::RemoveChild(child);

  NS_ASSERTION(!mChildren.Length(),
               "This child document of outerdoc accessible wasn't removed!");

  return wasRemoved;
}





void
OuterDocAccessible::CacheChildren()
{
  
  
  nsIDocument* outerDoc = mContent->GetCurrentDoc();
  if (outerDoc) {
    nsIDocument* innerDoc = outerDoc->GetSubDocumentFor(mContent);
    if (innerDoc)
      GetAccService()->GetDocAccessible(innerDoc);
  }
}
