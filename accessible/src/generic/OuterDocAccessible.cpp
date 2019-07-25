




#include "OuterDocAccessible.h"

#include "nsAccUtils.h"
#include "DocAccessible.h"
#include "Role.h"
#include "States.h"

#ifdef DEBUG
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
OuterDocAccessible::ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                 EWhichChildAtPoint aWhichChild)
{
  PRInt32 docX = 0, docY = 0, docWidth = 0, docHeight = 0;
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

nsresult
OuterDocAccessible::GetAttributesInternal(nsIPersistentProperties* aAttributes)
{
  nsAutoString tag;
  aAttributes->GetStringProperty(NS_LITERAL_CSTRING("tag"), tag);
  if (!tag.IsEmpty()) {
    
    
    return NS_OK;
  }
  return Accessible::GetAttributesInternal(aAttributes);
}




PRUint8
OuterDocAccessible::ActionCount()
{
  
  return 0;
}

NS_IMETHODIMP
OuterDocAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
OuterDocAccessible::GetActionDescription(PRUint8 aIndex,
                                         nsAString& aDescription)
{
  aDescription.Truncate();

  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
OuterDocAccessible::DoAction(PRUint8 aIndex)
{
  return NS_ERROR_INVALID_ARG;
}




void
OuterDocAccessible::Shutdown()
{
  
  
  
  
#ifdef DEBUG
  if (logging::IsEnabled(logging::eDocDestroy))
    logging::OuterDocDestroy(this);
#endif

  Accessible* childAcc = mChildren.SafeElementAt(0, nullptr);
  if (childAcc) {
#ifdef DEBUG
    if (logging::IsEnabled(logging::eDocDestroy)) {
      logging::DocDestroy("outerdoc's child document shutdown",
                          childAcc->GetDocumentNode());
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
OuterDocAccessible::AppendChild(Accessible* aAccessible)
{
  
  
  
  
  
  if (mChildren.Length())
    mChildren[0]->Shutdown();

  if (!AccessibleWrap::AppendChild(aAccessible))
    return false;

#ifdef DEBUG
  if (logging::IsEnabled(logging::eDocCreate)) {
    logging::DocCreate("append document to outerdoc",
                       aAccessible->GetDocumentNode());
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

#ifdef DEBUG
  if (logging::IsEnabled(logging::eDocDestroy)) {
    logging::DocDestroy("remove document from outerdoc", child->GetDocumentNode(),
                        child->AsDoc());
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
