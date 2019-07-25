








































 
#include "nsApplicationAccessible.h"

#include "States.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"

#include "nsIComponentManager.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIWindowMediator.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"

using namespace mozilla::a11y;

nsApplicationAccessible::nsApplicationAccessible() :
  nsAccessibleWrap(nsnull, nsnull)
{
  mFlags |= eApplicationAccessible;
}




NS_IMPL_ISUPPORTS_INHERITED1(nsApplicationAccessible, nsAccessible,
                             nsIAccessibleApplication)




NS_IMETHODIMP
nsApplicationAccessible::GetParent(nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetNextSibling(nsIAccessible **aNextSibling)
{
  NS_ENSURE_ARG_POINTER(aNextSibling);
  *aNextSibling = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetPreviousSibling(nsIAccessible **aPreviousSibling)
{
  NS_ENSURE_ARG_POINTER(aPreviousSibling);
  *aPreviousSibling = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetName(nsAString& aName)
{
  aName.Truncate();

  nsCOMPtr<nsIStringBundleService> bundleService =
    mozilla::services::GetStringBundleService();

  NS_ASSERTION(bundleService, "String bundle service must be present!");
  NS_ENSURE_STATE(bundleService);

  nsCOMPtr<nsIStringBundle> bundle;
  nsresult rv = bundleService->CreateBundle("chrome://branding/locale/brand.properties",
                                            getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLString appName;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                 getter_Copies(appName));
  if (NS_FAILED(rv) || appName.IsEmpty()) {
    NS_WARNING("brandShortName not found, using default app name");
    appName.AssignLiteral("Gecko based application");
  }

  aName.Assign(appName);
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetValue(nsAString &aValue)
{
  aValue.Truncate();
  return NS_OK;
}

void
nsApplicationAccessible::Description(nsString &aDescription)
{
  aDescription.Truncate();
}

PRUint64
nsApplicationAccessible::State()
{
  return IsDefunct() ? states::DEFUNCT : 0;
}

NS_IMETHODIMP
nsApplicationAccessible::GetAttributes(nsIPersistentProperties **aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  *aAttributes = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GroupPosition(PRInt32 *aGroupLevel,
                                       PRInt32 *aSimilarItemsInGroup,
                                       PRInt32 *aPositionInGroup)
{
  NS_ENSURE_ARG_POINTER(aGroupLevel);
  *aGroupLevel = 0;
  NS_ENSURE_ARG_POINTER(aSimilarItemsInGroup);
  *aSimilarItemsInGroup = 0;
  NS_ENSURE_ARG_POINTER(aPositionInGroup);
  *aPositionInGroup = 0;
  return NS_OK;
}

nsAccessible*
nsApplicationAccessible::ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                      EWhichChildAtPoint aWhichChild)
{
  return nsnull;
}

nsAccessible*
nsApplicationAccessible::FocusedChild()
{
  if (gLastFocusedNode) {
    nsAccessible* focusedChild =
      GetAccService()->GetAccessible(gLastFocusedNode);
    if (focusedChild && focusedChild->Parent() == this)
      return focusedChild;
  }
  return nsnull;
}

NS_IMETHODIMP
nsApplicationAccessible::GetRelationByType(PRUint32 aRelationType,
                                           nsIAccessibleRelation **aRelation)
{
  NS_ENSURE_ARG_POINTER(aRelation);
  *aRelation = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetRelationsCount(PRUint32 *aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetRelation(PRUint32 aIndex,
                                     nsIAccessibleRelation **aRelation)
{
  NS_ENSURE_ARG_POINTER(aRelation);
  *aRelation = nsnull;
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsApplicationAccessible::GetRelations(nsIArray **aRelations)
{
  NS_ENSURE_ARG_POINTER(aRelations);
  *aRelations = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetBounds(PRInt32 *aX, PRInt32 *aY,
                                   PRInt32 *aWidth, PRInt32 *aHeight)
{
  NS_ENSURE_ARG_POINTER(aX);
  *aX = 0;
  NS_ENSURE_ARG_POINTER(aY);
  *aY = 0;
  NS_ENSURE_ARG_POINTER(aWidth);
  *aWidth = 0;
  NS_ENSURE_ARG_POINTER(aHeight);
  *aHeight = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::SetSelected(PRBool aIsSelected)
{
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::TakeSelection()
{
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::TakeFocus()
{
  return NS_OK;
}

PRUint8
nsApplicationAccessible::ActionCount()
{
  return 0;
}

NS_IMETHODIMP
nsApplicationAccessible::GetActionName(PRUint8 aIndex, nsAString &aName)
{
  aName.Truncate();
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsApplicationAccessible::GetActionDescription(PRUint8 aIndex,
                                              nsAString &aDescription)
{
  aDescription.Truncate();
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsApplicationAccessible::DoAction(PRUint8 aIndex)
{
  return NS_OK;
}




NS_IMETHODIMP
nsApplicationAccessible::GetAppName(nsAString& aName)
{
  aName.Truncate();

  if (!mAppInfo)
    return NS_ERROR_FAILURE;

  nsCAutoString cname;
  nsresult rv = mAppInfo->GetName(cname);
  NS_ENSURE_SUCCESS(rv, rv);

  AppendUTF8toUTF16(cname, aName);
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetAppVersion(nsAString& aVersion)
{
  aVersion.Truncate();

  if (!mAppInfo)
    return NS_ERROR_FAILURE;

  nsCAutoString cversion;
  nsresult rv = mAppInfo->GetVersion(cversion);
  NS_ENSURE_SUCCESS(rv, rv);

  AppendUTF8toUTF16(cversion, aVersion);
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetPlatformName(nsAString& aName)
{
  aName.AssignLiteral("Gecko");
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetPlatformVersion(nsAString& aVersion)
{
  aVersion.Truncate();

  if (!mAppInfo)
    return NS_ERROR_FAILURE;

  nsCAutoString cversion;
  nsresult rv = mAppInfo->GetPlatformVersion(cversion);
  NS_ENSURE_SUCCESS(rv, rv);

  AppendUTF8toUTF16(cversion, aVersion);
  return NS_OK;
}




bool
nsApplicationAccessible::IsDefunct() const
{
  return nsAccessibilityService::IsShutdown();
}

PRBool
nsApplicationAccessible::Init()
{
  mAppInfo = do_GetService("@mozilla.org/xre/app-info;1");
  return PR_TRUE;
}

void
nsApplicationAccessible::Shutdown()
{
  mAppInfo = nsnull;
}

bool
nsApplicationAccessible::IsPrimaryForNode() const
{
  return false;
}




void
nsApplicationAccessible::ApplyARIAState(PRUint64* aState)
{
}

PRUint32
nsApplicationAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_APP_ROOT;
}

PRUint64
nsApplicationAccessible::NativeState()
{
  return 0;
}

void
nsApplicationAccessible::InvalidateChildren()
{
  
  
}

KeyBinding
nsApplicationAccessible::AccessKey() const
{
  return KeyBinding();
}




void
nsApplicationAccessible::CacheChildren()
{
  
  
  

  
  
  
  
  

  nsCOMPtr<nsIWindowMediator> windowMediator =
    do_GetService(NS_WINDOWMEDIATOR_CONTRACTID);

  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
  nsresult rv = windowMediator->GetEnumerator(nsnull,
                                              getter_AddRefs(windowEnumerator));
  if (NS_FAILED(rv))
    return;

  PRBool hasMore = PR_FALSE;
  windowEnumerator->HasMoreElements(&hasMore);
  while (hasMore) {
    nsCOMPtr<nsISupports> window;
    windowEnumerator->GetNext(getter_AddRefs(window));
    nsCOMPtr<nsIDOMWindow> DOMWindow = do_QueryInterface(window);
    if (DOMWindow) {
      nsCOMPtr<nsIDOMDocument> DOMDocument;
      DOMWindow->GetDocument(getter_AddRefs(DOMDocument));
      if (DOMDocument) {
        nsCOMPtr<nsIDocument> docNode(do_QueryInterface(DOMDocument));
        GetAccService()->GetDocAccessible(docNode); 
      }
    }
    windowEnumerator->HasMoreElements(&hasMore);
  }
}

nsAccessible*
nsApplicationAccessible::GetSiblingAtOffset(PRInt32 aOffset,
                                            nsresult* aError) const
{
  if (aError)
    *aError = NS_OK; 

  return nsnull;
}




NS_IMETHODIMP
nsApplicationAccessible::GetDOMNode(nsIDOMNode **aDOMNode)
{
  NS_ENSURE_ARG_POINTER(aDOMNode);
  *aDOMNode = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetDocument(nsIAccessibleDocument **aDocument)
{
  NS_ENSURE_ARG_POINTER(aDocument);
  *aDocument = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetRootDocument(nsIAccessibleDocument **aRootDocument)
{
  NS_ENSURE_ARG_POINTER(aRootDocument);
  *aRootDocument = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetInnerHTML(nsAString &aInnerHTML)
{
  aInnerHTML.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::ScrollTo(PRUint32 aScrollType)
{
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::ScrollToPoint(PRUint32 aCoordinateType,
                                       PRInt32 aX, PRInt32 aY)
{
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetComputedStyleValue(const nsAString &aPseudoElt,
                                               const nsAString &aPropertyName,
                                               nsAString &aValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetComputedStyleCSSValue(const nsAString &aPseudoElt,
                                                  const nsAString &aPropertyName,
                                                  nsIDOMCSSPrimitiveValue **aCSSPrimitiveValue)
{
  NS_ENSURE_ARG_POINTER(aCSSPrimitiveValue);
  *aCSSPrimitiveValue = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetLanguage(nsAString &aLanguage)
{
  aLanguage.Truncate();
  return NS_OK;
}

