




#include "OuterDocAccessible.h"

#include "Accessible-inl.h"
#include "nsAccUtils.h"
#include "DocAccessible-inl.h"
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
  nsIntRect docRect = Bounds();
  if (aX < docRect.x || aX >= docRect.x + docRect.width ||
      aY < docRect.y || aY >= docRect.y + docRect.height)
    return nullptr;

  
  
  Accessible* child = GetChildAt(0);
  NS_ENSURE_TRUE(child, nullptr);

  if (aWhichChild == eDeepestChild)
    return child->ChildAtPoint(aX, aY, eDeepestChild);
  return child;
}




void
OuterDocAccessible::Shutdown()
{
  
  
  
  
  

#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocDestroy))
    logging::OuterDocDestroy(this);
#endif

  Accessible* child = mChildren.SafeElementAt(0, nullptr);
  if (child) {
#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eDocDestroy)) {
      logging::DocDestroy("outerdoc's child document rebind is scheduled",
                          child->AsDoc()->DocumentNode());
    }
#endif
    RemoveChild(child);
    mDoc->BindChildDocument(child->AsDoc());
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
