





































#include "nsAccessibleWrap.h"
#include "nsAccessibilityAtoms.h"

#include "nsIAccessibleDocument.h"
#include "nsIAccessibleSelectable.h"
#include "nsIAccessibleEvent.h"
#include "nsIAccessibleWin32Object.h"

#include "Accessible2_i.c"
#include "AccessibleStates.h"

#include "nsIMutableArray.h"
#include "nsIDOMDocument.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIPrefService.h"
#include "nsRootAccessible.h"
#include "nsIServiceManager.h"
#include "nsTextFormatter.h"
#include "nsIView.h"
#include "nsRoleMap.h"
#include "nsEventMap.h"
#include "nsArrayUtils.h"







#ifdef DEBUG_LEAKS
static gAccessibles = 0;
#endif

EXTERN_C GUID CDECL CLSID_Accessible =
{ 0x61044601, 0xa811, 0x4e2b, { 0xbb, 0xba, 0x17, 0xbf, 0xab, 0xd3, 0x29, 0xd7 } };








nsAccessibleWrap::nsAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference *aShell):
  nsAccessible(aNode, aShell), mEnumVARIANTPosition(0)
{
}




nsAccessibleWrap::~nsAccessibleWrap()
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsAccessibleWrap, nsAccessible);






STDMETHODIMP nsAccessibleWrap::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IUnknown == iid || IID_IDispatch == iid || IID_IAccessible == iid)
    *ppv = static_cast<IAccessible*>(this);
  else if (IID_IEnumVARIANT == iid && !gIsEnumVariantSupportDisabled) {
    long numChildren;
    get_accChildCount(&numChildren);
    if (numChildren > 0)  
      *ppv = static_cast<IEnumVARIANT*>(this);
  } else if (IID_IServiceProvider == iid)
    *ppv = static_cast<IServiceProvider*>(this);
  else if (IID_IAccessible2 == iid)
    *ppv = static_cast<IAccessible2*>(this);

  if (NULL == *ppv) {
    HRESULT hr = CAccessibleComponent::QueryInterface(iid, ppv);
    if (SUCCEEDED(hr))
      return hr;
  }

  if (NULL == *ppv) {
    HRESULT hr = CAccessibleHyperlink::QueryInterface(iid, ppv);
    if (SUCCEEDED(hr))
      return hr;
  }

  if (NULL == *ppv) {
    HRESULT hr = CAccessibleValue::QueryInterface(iid, ppv);
    if (SUCCEEDED(hr))
      return hr;
  }

  if (NULL == *ppv)
    return nsAccessNodeWrap::QueryInterface(iid, ppv);

  (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
  return S_OK;
}






STDMETHODIMP nsAccessibleWrap::AccessibleObjectFromWindow(HWND hwnd,
                                                          DWORD dwObjectID,
                                                          REFIID riid,
                                                          void **ppvObject)
{
  
  if (!gmAccLib)
    gmAccLib =::LoadLibrary("OLEACC.DLL");

  if (gmAccLib) {
    if (!gmAccessibleObjectFromWindow)
      gmAccessibleObjectFromWindow = (LPFNACCESSIBLEOBJECTFROMWINDOW)GetProcAddress(gmAccLib,"AccessibleObjectFromWindow");

    if (gmAccessibleObjectFromWindow)
      return gmAccessibleObjectFromWindow(hwnd, dwObjectID, riid, ppvObject);
  }

  return E_FAIL;
}

STDMETHODIMP nsAccessibleWrap::NotifyWinEvent(DWORD event,
                                              HWND hwnd,
                                              LONG idObjectType,
                                              LONG idObject)
{
  if (gmNotifyWinEvent)
    return gmNotifyWinEvent(event, hwnd, idObjectType, idObject);

  return E_FAIL;
}

STDMETHODIMP nsAccessibleWrap::get_accParent( IDispatch __RPC_FAR *__RPC_FAR *ppdispParent)
{
  *ppdispParent = NULL;
  if (!mWeakShell)
    return E_FAIL;  

  nsIFrame *frame = GetFrame();
  HWND hwnd = 0;
  if (frame) {
    nsIView *view = frame->GetViewExternal();
    if (view) {
      
      
      
      
      
      nsIWidget *widget = view->GetWidget();
      if (widget) {
        hwnd = (HWND)widget->GetNativeData(NS_NATIVE_WINDOW);
        NS_ASSERTION(hwnd, "No window handle for window");
        nsIView *rootView;
        view->GetViewManager()->GetRootView(rootView);
        if (rootView == view) {
          
          
          
          hwnd = ::GetParent(hwnd);
          NS_ASSERTION(hwnd, "No window handle for window");
        }
      }
      else {
        
        
        nsIScrollableFrame *scrollFrame = nsnull;
        CallQueryInterface(frame, &scrollFrame);
        if (scrollFrame) {
          hwnd = (HWND)scrollFrame->GetScrolledFrame()->GetWindow()->GetNativeData(NS_NATIVE_WINDOW);
          NS_ASSERTION(hwnd, "No window handle for window");
        }
      }
    }

    if (hwnd && SUCCEEDED(AccessibleObjectFromWindow(hwnd, OBJID_WINDOW, IID_IAccessible,
                                              (void**)ppdispParent))) {
      return S_OK;
    }
  }

  nsCOMPtr<nsIAccessible> xpParentAccessible(GetParent());
  NS_ASSERTION(xpParentAccessible, "No parent accessible where we're not direct child of window");
  if (!xpParentAccessible) {
    return E_UNEXPECTED;
  }
  *ppdispParent = NativeAccessible(xpParentAccessible);

  return S_OK;
}

STDMETHODIMP nsAccessibleWrap::get_accChildCount( long __RPC_FAR *pcountChildren)
{
  *pcountChildren = 0;
  if (MustPrune(this)) {
    return NS_OK;
  }

  PRInt32 numChildren;
  GetChildCount(&numChildren);
  *pcountChildren = numChildren;

  return S_OK;
}

STDMETHODIMP nsAccessibleWrap::get_accChild(
       VARIANT varChild,
       IDispatch __RPC_FAR *__RPC_FAR *ppdispChild)
{
  *ppdispChild = NULL;

  if (!mWeakShell || varChild.vt != VT_I4)
    return E_FAIL;

  if (varChild.lVal == CHILDID_SELF) {
    *ppdispChild = static_cast<IDispatch*>(this);
    AddRef();
    return S_OK;
  }

  nsCOMPtr<nsIAccessible> childAccessible;
  if (!MustPrune(this)) {
    GetChildAt(varChild.lVal - 1, getter_AddRefs(childAccessible));
    if (childAccessible) {
      *ppdispChild = NativeAccessible(childAccessible);
    }
  }

  return (*ppdispChild)? S_OK: E_FAIL;
}

STDMETHODIMP nsAccessibleWrap::get_accName(
       VARIANT varChild,
       BSTR __RPC_FAR *pszName)
{
  *pszName = NULL;
  nsCOMPtr<nsIAccessible> xpAccessible;
  GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));
  if (xpAccessible) {
    nsAutoString name;
    if (NS_FAILED(xpAccessible->GetName(name)))
      return S_FALSE;
    if (!name.IsVoid()) {
      *pszName = ::SysAllocString(name.get());
    }
#ifdef DEBUG_A11Y
    NS_ASSERTION(mIsInitialized, "Access node was not initialized");
#endif
  }

  return S_OK;
}


STDMETHODIMP nsAccessibleWrap::get_accValue(
       VARIANT varChild,
       BSTR __RPC_FAR *pszValue)
{
  *pszValue = NULL;
  nsCOMPtr<nsIAccessible> xpAccessible;
  GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));
  if (xpAccessible) {
    nsAutoString value;
    if (NS_FAILED(xpAccessible->GetValue(value)))
      return S_FALSE;

    *pszValue = ::SysAllocString(value.get());
  }

  return S_OK;
}

STDMETHODIMP
nsAccessibleWrap::get_accDescription(VARIANT varChild,
                                     BSTR __RPC_FAR *pszDescription)
{
  *pszDescription = NULL;
  nsCOMPtr<nsIAccessible> xpAccessible;
  GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));
  if (!xpAccessible)
    return E_FAIL;

  
  
  

  nsAutoString description;

  
  PRInt32 groupLevel;
  PRInt32 similarItemsInGroup;
  PRInt32 positionInGroup;

  nsresult rv = xpAccessible->GroupPosition(&groupLevel, &similarItemsInGroup,
                                            &positionInGroup);
  if (NS_SUCCEEDED(rv)) {
    if (positionInGroup > 0) {
      if (groupLevel > 0) {
        
        
        
        PRInt32 numChildren = 0;

        PRUint32 currentRole = 0;
        rv = xpAccessible->GetFinalRole(&currentRole);
        if (NS_SUCCEEDED(rv) &&
            currentRole == nsIAccessibleRole::ROLE_OUTLINEITEM) {
          nsCOMPtr<nsIAccessible> child;
          xpAccessible->GetFirstChild(getter_AddRefs(child));
          while (child) {
            child->GetFinalRole(&currentRole);
            if (currentRole == nsIAccessibleRole::ROLE_GROUPING) {
              nsCOMPtr<nsIAccessible> groupChild;
              child->GetFirstChild(getter_AddRefs(groupChild));
              while (groupChild) {
                groupChild->GetFinalRole(&currentRole);
                numChildren +=
                  (currentRole == nsIAccessibleRole::ROLE_OUTLINEITEM);
                nsCOMPtr<nsIAccessible> nextGroupChild;
                groupChild->GetNextSibling(getter_AddRefs(nextGroupChild));
                groupChild.swap(nextGroupChild);
              }
              break;
            }
            nsCOMPtr<nsIAccessible> nextChild;
            child->GetNextSibling(getter_AddRefs(nextChild));
            child.swap(nextChild);
          }
        }

        if (numChildren) {
          nsTextFormatter::ssprintf(description,
                                    NS_LITERAL_STRING("L%d, %d of %d with %d").get(),
                                    groupLevel, positionInGroup,
                                    similarItemsInGroup + 1, numChildren);
        } else {
          nsTextFormatter::ssprintf(description,
                                    NS_LITERAL_STRING("L%d, %d of %d").get(),
                                    groupLevel, positionInGroup,
                                    similarItemsInGroup + 1);
        }
      } else { 
        nsTextFormatter::ssprintf(description,
                                  NS_LITERAL_STRING("%d of %d").get(),
                                  positionInGroup, similarItemsInGroup + 1);
      }

      *pszDescription = ::SysAllocString(description.get());
      return S_OK;
    }
  }

  xpAccessible->GetDescription(description);
  if (!description.IsEmpty()) {
    
    
    
    
    description = NS_LITERAL_STRING("Description: ") + description;
  }

  *pszDescription = ::SysAllocString(description.get());
  return S_OK;
}

STDMETHODIMP nsAccessibleWrap::get_accRole(
       VARIANT varChild,
       VARIANT __RPC_FAR *pvarRole)
{
  VariantInit(pvarRole);

  nsCOMPtr<nsIAccessible> xpAccessible;
  GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));

  if (!xpAccessible)
    return E_FAIL;

#ifdef DEBUG_A11Y
  NS_ASSERTION(nsAccessible::IsTextInterfaceSupportCorrect(xpAccessible), "Does not support nsIAccessibleText when it should");
#endif

  PRUint32 xpRole = 0, msaaRole = 0;
  if (NS_FAILED(xpAccessible->GetFinalRole(&xpRole)))
    return E_FAIL;

  msaaRole = gWindowsRoleMap[xpRole].msaaRole;
  NS_ASSERTION(gWindowsRoleMap[nsIAccessibleRole::ROLE_LAST_ENTRY].msaaRole == ROLE_WINDOWS_LAST_ENTRY,
               "MSAA role map skewed");

  
  
  
  if (xpRole == nsIAccessibleRole::ROLE_ROW) {
    nsCOMPtr<nsIAccessible> parent = GetParent();
    if (parent && Role(parent) == nsIAccessibleRole::ROLE_TREE_TABLE) {
      msaaRole = ROLE_SYSTEM_OUTLINEITEM;
    }
  }
  
  
  if (msaaRole != USE_ROLE_STRING) {
    pvarRole->vt = VT_I4;
    pvarRole->lVal = msaaRole;  
    return S_OK;
  }

  
  
  
  nsCOMPtr<nsIDOMNode> domNode;
  nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(xpAccessible));
  if (!accessNode)
    return E_FAIL;

  accessNode->GetDOMNode(getter_AddRefs(domNode));
  nsIContent *content = GetRoleContent(domNode);
  if (!content)
    return E_FAIL;

  if (content->IsNodeOfType(nsINode::eELEMENT)) {
    nsAutoString roleString;
    if (msaaRole != ROLE_SYSTEM_CLIENT && !GetRoleAttribute(content, roleString)) {
      nsINodeInfo *nodeInfo = content->NodeInfo();
      nodeInfo->GetName(roleString);
      nsAutoString nameSpaceURI;
      nodeInfo->GetNamespaceURI(nameSpaceURI);
      if (!nameSpaceURI.IsEmpty()) {
        
        roleString += NS_LITERAL_STRING(", ") + nameSpaceURI;
      }
    }
    if (!roleString.IsEmpty()) {
      pvarRole->vt = VT_BSTR;
      pvarRole->bstrVal = ::SysAllocString(roleString.get());
      return S_OK;
    }
  }
  return E_FAIL;
}

STDMETHODIMP nsAccessibleWrap::get_accState(
       VARIANT varChild,
       VARIANT __RPC_FAR *pvarState)
{
  VariantInit(pvarState);
  pvarState->vt = VT_I4;
  pvarState->lVal = 0;

  nsCOMPtr<nsIAccessible> xpAccessible;
  GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));
  if (!xpAccessible)
    return E_FAIL;

  PRUint32 state = 0, extraState;
  if (NS_FAILED(xpAccessible->GetFinalState(&state, &extraState)))
    return E_FAIL;

  pvarState->lVal = state;

  return S_OK;
}


STDMETHODIMP nsAccessibleWrap::get_accHelp(
       VARIANT varChild,
       BSTR __RPC_FAR *pszHelp)
{
  *pszHelp = NULL;
  return S_FALSE;
}

STDMETHODIMP nsAccessibleWrap::get_accHelpTopic(
       BSTR __RPC_FAR *pszHelpFile,
       VARIANT varChild,
       long __RPC_FAR *pidTopic)
{
  *pszHelpFile = NULL;
  *pidTopic = 0;
  return E_NOTIMPL;
}

STDMETHODIMP nsAccessibleWrap::get_accKeyboardShortcut(
       VARIANT varChild,
       BSTR __RPC_FAR *pszKeyboardShortcut)
{
  *pszKeyboardShortcut = NULL;
  nsCOMPtr<nsIAccessible> xpAccessible;
  GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));
  if (xpAccessible) {
    nsAutoString shortcut;
    nsresult rv = xpAccessible->GetKeyboardShortcut(shortcut);
    if (NS_FAILED(rv))
      return S_FALSE;

    *pszKeyboardShortcut = ::SysAllocString(shortcut.get());
    return S_OK;
  }
  return S_FALSE;
}

STDMETHODIMP nsAccessibleWrap::get_accFocus(
       VARIANT __RPC_FAR *pvarChild)
{
  
  
  
  
  
  

  if (!mDOMNode) {
    return E_FAIL; 
  }

  VariantInit(pvarChild);

  
  nsCOMPtr<nsIAccessible> focusedAccessible;
  GetFocusedChild(getter_AddRefs(focusedAccessible));
  if (focusedAccessible == this) {
    pvarChild->vt = VT_I4;
    pvarChild->lVal = CHILDID_SELF;
  }
  else if (focusedAccessible) {
    pvarChild->vt = VT_DISPATCH;
    pvarChild->pdispVal = NativeAccessible(focusedAccessible);
  }
  else {
    pvarChild->vt = VT_EMPTY;   
  }

  return S_OK;
}



class AccessibleEnumerator : public IEnumVARIANT
{
public:
  AccessibleEnumerator(nsIArray* aArray) : mArray(aArray), mCurIndex(0) { }
  AccessibleEnumerator(const AccessibleEnumerator& toCopy) :
    mArray(toCopy.mArray), mCurIndex(toCopy.mCurIndex) { }
  ~AccessibleEnumerator() { }

  
  STDMETHODIMP QueryInterface(REFIID iid, void ** ppvObject);
  STDMETHODIMP_(ULONG) AddRef(void);
  STDMETHODIMP_(ULONG) Release(void);

  
  STDMETHODIMP Next(unsigned long celt, VARIANT FAR* rgvar, unsigned long FAR* pceltFetched);
  STDMETHODIMP Skip(unsigned long celt);
  STDMETHODIMP Reset()
  {
    mCurIndex = 0;
    return S_OK;
  }
  STDMETHODIMP Clone(IEnumVARIANT FAR* FAR* ppenum);

private:
  nsCOMPtr<nsIArray> mArray;
  PRUint32 mCurIndex;
  nsAutoRefCnt mRefCnt;
};

HRESULT
AccessibleEnumerator::QueryInterface(REFIID iid, void ** ppvObject)
{
  if (iid == IID_IEnumVARIANT) {
    *ppvObject = static_cast<IEnumVARIANT*>(this);
    AddRef();
    return S_OK;
  }
  if (iid == IID_IUnknown) {
    *ppvObject = static_cast<IUnknown*>(this);
    AddRef();
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
AccessibleEnumerator::AddRef(void)
{
  return ++mRefCnt;
}

STDMETHODIMP_(ULONG)
AccessibleEnumerator::Release(void)
{
  ULONG r = --mRefCnt;
  if (r == 0)
    delete this;
  return r;
}

STDMETHODIMP
AccessibleEnumerator::Next(unsigned long celt, VARIANT FAR* rgvar, unsigned long FAR* pceltFetched)
{
  PRUint32 length = 0;
  mArray->GetLength(&length);

  HRESULT hr = S_OK;

  
  if (celt > length - mCurIndex) {
    hr = S_FALSE;
    celt = length - mCurIndex;
  }

  for (PRUint32 i = 0; i < celt; ++i, ++mCurIndex) {
    
    nsCOMPtr<nsIAccessible> accel(do_QueryElementAt(mArray, mCurIndex));
    NS_ASSERTION(accel, "Invalid pointer in mArray");

    if (accel) {
      rgvar[i].vt = VT_DISPATCH;
      rgvar[i].pdispVal = nsAccessibleWrap::NativeAccessible(accel);
    }
  }

  if (pceltFetched)
    *pceltFetched = celt;

  return hr;
}

STDMETHODIMP
AccessibleEnumerator::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
  *ppenum = new AccessibleEnumerator(*this);
  if (!*ppenum)
    return E_OUTOFMEMORY;
  NS_ADDREF(*ppenum);
  return S_OK;
}

STDMETHODIMP
AccessibleEnumerator::Skip(unsigned long celt)
{
  PRUint32 length = 0;
  mArray->GetLength(&length);
  
  if (celt > length - mCurIndex) {
    mCurIndex = length;
    return S_FALSE;
  }
  mCurIndex += celt;
  return S_OK;
}


















STDMETHODIMP nsAccessibleWrap::get_accSelection(VARIANT __RPC_FAR *pvarChildren)
{
  VariantInit(pvarChildren);
  pvarChildren->vt = VT_EMPTY;

  nsCOMPtr<nsIAccessibleSelectable> 
    select(do_QueryInterface(static_cast<nsIAccessible*>(this)));

  if (select) {  
    
    nsCOMPtr<nsIArray> selectedOptions;
    
    select->GetSelectedChildren(getter_AddRefs(selectedOptions));
    if (selectedOptions) { 
      
      nsRefPtr<AccessibleEnumerator> pEnum = new AccessibleEnumerator(selectedOptions);

      
      if (!pEnum)
        return E_OUTOFMEMORY;
      pvarChildren->vt = VT_UNKNOWN;    
      NS_ADDREF(pvarChildren->punkVal = pEnum);
    }
  }
  return S_OK;
}

STDMETHODIMP nsAccessibleWrap::get_accDefaultAction(
       VARIANT varChild,
       BSTR __RPC_FAR *pszDefaultAction)
{
  *pszDefaultAction = NULL;
  nsCOMPtr<nsIAccessible> xpAccessible;
  GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));
  if (xpAccessible) {
    nsAutoString defaultAction;
    if (NS_FAILED(xpAccessible->GetActionName(0, defaultAction)))
      return S_FALSE;

    *pszDefaultAction = ::SysAllocString(defaultAction.get());
  }

  return S_OK;
}

STDMETHODIMP nsAccessibleWrap::accSelect(
       long flagsSelect,
       VARIANT varChild)
{
  
  nsCOMPtr<nsIAccessible> xpAccessible;
  GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));
  NS_ENSURE_TRUE(xpAccessible, E_FAIL);

  if (flagsSelect & (SELFLAG_TAKEFOCUS|SELFLAG_TAKESELECTION|SELFLAG_REMOVESELECTION))
  {
    if (flagsSelect & SELFLAG_TAKEFOCUS)
      xpAccessible->TakeFocus();

    if (flagsSelect & SELFLAG_TAKESELECTION)
      xpAccessible->TakeSelection();

    if (flagsSelect & SELFLAG_ADDSELECTION)
      xpAccessible->SetSelected(PR_TRUE);

    if (flagsSelect & SELFLAG_REMOVESELECTION)
      xpAccessible->SetSelected(PR_FALSE);

    if (flagsSelect & SELFLAG_EXTENDSELECTION)
      xpAccessible->ExtendSelection();

    return S_OK;
  }

  return E_FAIL;
}

STDMETHODIMP nsAccessibleWrap::accLocation(
       long __RPC_FAR *pxLeft,
       long __RPC_FAR *pyTop,
       long __RPC_FAR *pcxWidth,
       long __RPC_FAR *pcyHeight,
       VARIANT varChild)
{
  nsCOMPtr<nsIAccessible> xpAccessible;
  GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));

  if (xpAccessible) {
    PRInt32 x, y, width, height;
    if (NS_FAILED(xpAccessible->GetBounds(&x, &y, &width, &height)))
      return E_FAIL;

    *pxLeft = x;
    *pyTop = y;
    *pcxWidth = width;
    *pcyHeight = height;
    return S_OK;
  }

  return E_FAIL;
}

STDMETHODIMP nsAccessibleWrap::accNavigate(
       long navDir,
       VARIANT varStart,
       VARIANT __RPC_FAR *pvarEndUpAt)
{
  nsCOMPtr<nsIAccessible> xpAccessibleStart, xpAccessibleResult;
  GetXPAccessibleFor(varStart, getter_AddRefs(xpAccessibleStart));
  if (!xpAccessibleStart)
    return E_FAIL;

  VariantInit(pvarEndUpAt);
  PRUint32 xpRelation = 0;

  switch(navDir) {
    case NAVDIR_DOWN:
      xpAccessibleStart->GetAccessibleBelow(getter_AddRefs(xpAccessibleResult));
      break;
    case NAVDIR_FIRSTCHILD:
      if (!MustPrune(xpAccessibleStart)) {
        xpAccessibleStart->GetFirstChild(getter_AddRefs(xpAccessibleResult));
      }
      break;
    case NAVDIR_LASTCHILD:
      if (!MustPrune(xpAccessibleStart)) {
        xpAccessibleStart->GetLastChild(getter_AddRefs(xpAccessibleResult));
      }
      break;
    case NAVDIR_LEFT:
      xpAccessibleStart->GetAccessibleToLeft(getter_AddRefs(xpAccessibleResult));
      break;
    case NAVDIR_NEXT:
      xpAccessibleStart->GetNextSibling(getter_AddRefs(xpAccessibleResult));
      break;
    case NAVDIR_PREVIOUS:
      xpAccessibleStart->GetPreviousSibling(getter_AddRefs(xpAccessibleResult));
      break;
    case NAVDIR_RIGHT:
      xpAccessibleStart->GetAccessibleToRight(getter_AddRefs(xpAccessibleResult));
      break;
    case NAVDIR_UP:
      xpAccessibleStart->GetAccessibleAbove(getter_AddRefs(xpAccessibleResult));
      break;

    
    case NAVRELATION_CONTROLLED_BY:
      xpRelation = nsIAccessibleRelation::RELATION_CONTROLLED_BY;
      break;
    case NAVRELATION_CONTROLLER_FOR:
      xpRelation = nsIAccessibleRelation::RELATION_CONTROLLER_FOR;
      break;
    case NAVRELATION_LABEL_FOR:
      xpRelation = nsIAccessibleRelation::RELATION_LABEL_FOR;
      break;
    case NAVRELATION_LABELLED_BY:
      xpRelation = nsIAccessibleRelation::RELATION_LABELLED_BY;
      break;
    case NAVRELATION_MEMBER_OF:
      xpRelation = nsIAccessibleRelation::RELATION_MEMBER_OF;
      break;
    case NAVRELATION_NODE_CHILD_OF:
      xpRelation = nsIAccessibleRelation::RELATION_NODE_CHILD_OF;
      break;
    case NAVRELATION_FLOWS_TO:
      xpRelation = nsIAccessibleRelation::RELATION_FLOWS_TO;
      break;
    case NAVRELATION_FLOWS_FROM:
      xpRelation = nsIAccessibleRelation::RELATION_FLOWS_FROM;
      break;
    case NAVRELATION_SUBWINDOW_OF:
      xpRelation = nsIAccessibleRelation::RELATION_SUBWINDOW_OF;
      break;
    case NAVRELATION_EMBEDS:
      xpRelation = nsIAccessibleRelation::RELATION_EMBEDS;
      break;
    case NAVRELATION_EMBEDDED_BY:
      xpRelation = nsIAccessibleRelation::RELATION_EMBEDDED_BY;
      break;
    case NAVRELATION_POPUP_FOR:
      xpRelation = nsIAccessibleRelation::RELATION_POPUP_FOR;
      break;
    case NAVRELATION_PARENT_WINDOW_OF:
      xpRelation = nsIAccessibleRelation::RELATION_PARENT_WINDOW_OF;
      break;
    case NAVRELATION_DEFAULT_BUTTON:
      xpRelation = nsIAccessibleRelation::RELATION_DEFAULT_BUTTON;
      break;
    case NAVRELATION_DESCRIBED_BY:
      xpRelation = nsIAccessibleRelation::RELATION_DESCRIBED_BY;
      break;
    case NAVRELATION_DESCRIPTION_FOR:
      xpRelation = nsIAccessibleRelation::RELATION_DESCRIPTION_FOR;
      break;
  }

  pvarEndUpAt->vt = VT_EMPTY;

  if (xpRelation) {
    nsresult rv = GetAccessibleRelated(xpRelation,
                                       getter_AddRefs(xpAccessibleResult));
    if (rv == NS_ERROR_NOT_IMPLEMENTED) {
      return E_NOTIMPL;
    }
  }

  if (xpAccessibleResult) {
    pvarEndUpAt->pdispVal = NativeAccessible(xpAccessibleResult);
    pvarEndUpAt->vt = VT_DISPATCH;
    return NS_OK;
  }
  return E_FAIL;
}

STDMETHODIMP nsAccessibleWrap::accHitTest(
       long xLeft,
       long yTop,
       VARIANT __RPC_FAR *pvarChild)
{
  VariantInit(pvarChild);

  
  nsCOMPtr<nsIAccessible> xpAccessible;

  xLeft = xLeft;
  yTop = yTop;

  if (MustPrune(this)) {
    xpAccessible = this;
  }
  else {
    GetChildAtPoint(xLeft, yTop, getter_AddRefs(xpAccessible));
  }

  
  if (xpAccessible) {
    
    if (xpAccessible == static_cast<nsIAccessible*>(this)) {
      pvarChild->vt = VT_I4;
      pvarChild->lVal = CHILDID_SELF;
    } else { 
      pvarChild->vt = VT_DISPATCH;
      pvarChild->pdispVal = NativeAccessible(xpAccessible);
      nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(xpAccessible));
      NS_ASSERTION(accessNode, "Unable to QI to nsIAccessNode");
      nsCOMPtr<nsIDOMNode> domNode;
      accessNode->GetDOMNode(getter_AddRefs(domNode));
      if (!domNode) {
        
        pvarChild->vt = VT_EMPTY;
        return E_FAIL;
      }
    }
  } else {
    
    pvarChild->vt = VT_EMPTY;
    return E_FAIL;
  }

  return S_OK;
}

STDMETHODIMP nsAccessibleWrap::accDoDefaultAction(
       VARIANT varChild)
{
  nsCOMPtr<nsIAccessible> xpAccessible;
  GetXPAccessibleFor(varChild, getter_AddRefs(xpAccessible));

  if (!xpAccessible || FAILED(xpAccessible->DoAction(0))) {
    return E_FAIL;
  }
  return S_OK;
}

STDMETHODIMP nsAccessibleWrap::put_accName(
       VARIANT varChild,
       BSTR szName)
{
  return E_NOTIMPL;
}

STDMETHODIMP nsAccessibleWrap::put_accValue(
       VARIANT varChild,
       BSTR szValue)
{
  return E_NOTIMPL;
}

#include "mshtml.h"

STDMETHODIMP
nsAccessibleWrap::QueryService(REFGUID guidService, REFIID iid, void** ppv)
{
  













  return QueryInterface(iid, ppv);
}


STDMETHODIMP
nsAccessibleWrap::Next(ULONG aNumElementsRequested, VARIANT FAR* pvar, ULONG FAR* aNumElementsFetched)
{
  
  
  

  
  *aNumElementsFetched = 0;

  PRInt32 numChildren;
  GetChildCount(&numChildren);

  if (aNumElementsRequested <= 0 || !pvar ||
      mEnumVARIANTPosition >= numChildren) {
    return E_FAIL;
  }

  VARIANT varStart;
  VariantInit(&varStart);
  varStart.lVal = CHILDID_SELF;
  varStart.vt = VT_I4;

  accNavigate(NAVDIR_FIRSTCHILD, varStart, &pvar[0]);

  for (long childIndex = 0; pvar[*aNumElementsFetched].vt == VT_DISPATCH; ++childIndex) {
    PRBool wasAccessibleFetched = PR_FALSE;
    nsAccessibleWrap *msaaAccessible =
      static_cast<nsAccessibleWrap*>(pvar[*aNumElementsFetched].pdispVal);
    if (!msaaAccessible)
      break;
    if (childIndex >= mEnumVARIANTPosition) {
      if (++*aNumElementsFetched >= aNumElementsRequested)
        break;
      wasAccessibleFetched = PR_TRUE;
    }
    msaaAccessible->accNavigate(NAVDIR_NEXT, varStart, &pvar[*aNumElementsFetched] );
    if (!wasAccessibleFetched)
      msaaAccessible->nsAccessNode::Release(); 
  }

  mEnumVARIANTPosition += static_cast<PRUint16>(*aNumElementsFetched);
  return NOERROR;
}

STDMETHODIMP
nsAccessibleWrap::Skip(ULONG aNumElements)
{
  mEnumVARIANTPosition += static_cast<PRUint16>(aNumElements);

  PRInt32 numChildren;
  GetChildCount(&numChildren);

  if (mEnumVARIANTPosition > numChildren)
  {
    mEnumVARIANTPosition = numChildren;
    return S_FALSE;
  }
  return NOERROR;
}

STDMETHODIMP
nsAccessibleWrap::Reset(void)
{
  mEnumVARIANTPosition = 0;
  return NOERROR;
}




STDMETHODIMP
nsAccessibleWrap::get_nRelations(long *aNRelations)
{
  PRUint32 count = 0;
  nsresult rv = GetRelationsCount(&count);
  *aNRelations = count;

  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
nsAccessibleWrap::get_relation(long aRelationIndex,
                               IAccessibleRelation **aRelation)
{
  nsCOMPtr<nsIAccessibleRelation> relation;
  nsresult rv = GetRelation(aRelationIndex, getter_AddRefs(relation));
  if (NS_FAILED(rv))
    return E_FAIL;

  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryInterface(relation));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  rv =  winAccessNode->QueryNativeInterface(IID_IAccessibleRelation,
                                            &instancePtr);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aRelation = static_cast<IAccessibleRelation*>(instancePtr);
  return S_OK;
}

STDMETHODIMP
nsAccessibleWrap::get_relations(long aMaxRelations,
                                IAccessibleRelation **aRelation,
                                long *aNRelations)
{
  *aNRelations = 0;

  nsCOMPtr<nsIArray> relations;
  nsresult rv = GetRelations(getter_AddRefs(relations));
  if (NS_FAILED(rv))
    return E_FAIL;

  PRUint32 length = 0;
  rv = relations->GetLength(&length);
  if (NS_FAILED(rv))
    return E_FAIL;

  PRUint32 count = length < (PRUint32)aMaxRelations ? length : aMaxRelations;

  PRUint32 index = 0;
  for (; index < count; index++) {
    nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryElementAt(relations, index, &rv));
    if (NS_FAILED(rv) || !winAccessNode)
      break;

    void *instancePtr = NULL;
    nsresult rv =  winAccessNode->QueryNativeInterface(IID_IAccessibleRelation,
                                                       &instancePtr);
    if (NS_FAILED(rv))
      break;

    aRelation[index] = static_cast<IAccessibleRelation*>(instancePtr);
  }

  if (NS_FAILED(rv)) {
    for (PRUint32 index2 = 0; index2 < index; index2++) {
      aRelation[index2]->Release();
      aRelation[index2] = NULL;
    }
    return E_FAIL;
  }

  *aNRelations = count;
  return S_OK;
}

STDMETHODIMP
nsAccessibleWrap::role(long *role)
{
  PRUint32 xpRole = 0;
  if (NS_FAILED(GetFinalRole(&xpRole)))
    return E_FAIL;

  NS_ASSERTION(gWindowsRoleMap[nsIAccessibleRole::ROLE_LAST_ENTRY].ia2Role == ROLE_WINDOWS_LAST_ENTRY,
               "MSAA role map skewed");

  *role = gWindowsRoleMap[xpRole].ia2Role;

  return S_OK;
}

STDMETHODIMP
nsAccessibleWrap::scrollTo(enum IA2ScrollType aScrollType)
{
  if (NS_SUCCEEDED(ScrollTo(aScrollType)))
    return S_OK;
  return E_FAIL;
}

STDMETHODIMP
nsAccessibleWrap::scrollToPoint(enum IA2CoordinateType coordinateType,
                                long x, long y)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsAccessibleWrap::get_groupPosition(long *aGroupLevel,
                                    long *aSimilarItemsInGroup,
                                    long *aPositionInGroup)
{
  PRInt32 groupLevel = 0;
  PRInt32 similarItemsInGroup = 0;
  PRInt32 positionInGroup = 0;
  nsresult rv = GroupPosition(&groupLevel, &similarItemsInGroup,
                              &positionInGroup);

  if (NS_SUCCEEDED(rv)) {
   *aGroupLevel = groupLevel;
   *aSimilarItemsInGroup = similarItemsInGroup;
   *aPositionInGroup = positionInGroup;
    return S_OK;
  }

  return E_FAIL;
}

STDMETHODIMP
nsAccessibleWrap::get_states(AccessibleStates *aStates)
{
  *aStates = 0;

  

  PRUint32 states = 0, extraStates = 0;
  nsresult rv = GetFinalState(&states, &extraStates);
  if (NS_FAILED(rv))
    return E_FAIL;

  if (states & nsIAccessibleStates::STATE_INVALID)
    *aStates |= IA2_STATE_INVALID_ENTRY;
  else if (states & nsIAccessibleStates::STATE_REQUIRED)
    *aStates |= IA2_STATE_REQUIRED;

  
  
  
  
  

  if (extraStates & nsIAccessibleStates::EXT_STATE_ACTIVE)
    *aStates |= IA2_STATE_ACTIVE;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_DEFUNCT)
    *aStates |= IA2_STATE_DEFUNCT;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_EDITABLE)
    *aStates |= IA2_STATE_EDITABLE;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_HORIZONTAL)
    *aStates |= IA2_STATE_HORIZONTAL;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_MODAL)
    *aStates |= IA2_STATE_MODAL;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_MULTI_LINE)
    *aStates |= IA2_STATE_MULTI_LINE;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_OPAQUE)
    *aStates |= IA2_STATE_OPAQUE;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_SELECTABLE_TEXT)
    *aStates |= IA2_STATE_SELECTABLE_TEXT;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_SINGLE_LINE)
    *aStates |= IA2_STATE_SINGLE_LINE;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_STALE)
    *aStates |= IA2_STATE_STALE;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_SUPPORTS_AUTOCOMPLETION)
    *aStates |= IA2_STATE_SUPPORTS_AUTOCOMPLETION;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_TRANSIENT)
    *aStates |= IA2_STATE_TRANSIENT;
  else if (extraStates & nsIAccessibleStates::EXT_STATE_VERTICAL)
    *aStates |= IA2_STATE_VERTICAL;

  return S_OK;
}

STDMETHODIMP
nsAccessibleWrap::get_extendedRole(BSTR *extendedRole)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsAccessibleWrap::get_localizedExtendedRole(BSTR *localizedExtendedRole)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsAccessibleWrap::get_nExtendedStates(long *nExtendedStates)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsAccessibleWrap::get_extendedStates(long maxExtendedStates,
                                     BSTR **extendedStates,
                                     long *nExtendedStates)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsAccessibleWrap::get_localizedExtendedStates(long maxLocalizedExtendedStates,
                                              BSTR **localizedExtendedStates,
                                              long *nLocalizedExtendedStates)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsAccessibleWrap::get_uniqueID(long *uniqueID)
{
  void *id;
  if (NS_SUCCEEDED(GetUniqueID(&id))) {
    *uniqueID = reinterpret_cast<long>(id);
    return S_OK;
  }
  return E_FAIL;
}

STDMETHODIMP
nsAccessibleWrap::get_windowHandle(HWND *windowHandle)
{
  void **handle = nsnull;
  if (NS_SUCCEEDED(GetOwnerWindow(handle))) {
    *windowHandle = reinterpret_cast<HWND>(*handle);
    return S_OK;
  }
  return E_FAIL;
}

STDMETHODIMP
nsAccessibleWrap::get_indexInParent(long *indexInParent)
{
  PRInt32 index;
  if (NS_SUCCEEDED(GetIndexInParent(&index))) {
    *indexInParent = index;
    return S_OK;
  }
  return E_FAIL;
}

STDMETHODIMP
nsAccessibleWrap::get_locale(IA2Locale *locale)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsAccessibleWrap::get_attributes(BSTR *aAttributes)
{
  
  

  *aAttributes = NULL;

  nsCOMPtr<nsIPersistentProperties> attributes;
  if (NS_FAILED(GetAttributes(getter_AddRefs(attributes))))
    return E_FAIL;

  if (!attributes)
    return S_OK;

  nsCOMPtr<nsISimpleEnumerator> propEnum;
  attributes->Enumerate(getter_AddRefs(propEnum));
  if (!propEnum)
    return E_FAIL;

  nsAutoString strAttrs;

  const char kCharsToEscape[] = ":;=,\\";

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(propEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> propSupports;
    propEnum->GetNext(getter_AddRefs(propSupports));

    nsCOMPtr<nsIPropertyElement> propElem(do_QueryInterface(propSupports));
    if (!propElem)
      return E_FAIL;

    nsCAutoString name;
    if (NS_FAILED(propElem->GetKey(name)))
      return E_FAIL;

    PRUint32 offset = 0;
    while ((offset = name.FindCharInSet(kCharsToEscape, offset)) != kNotFound) {
      name.Insert('\\', offset);
      offset += 2;
    }

    nsAutoString value;
    if (NS_FAILED(propElem->GetValue(value)))
      return E_FAIL;

    offset = 0;
    while ((offset = value.FindCharInSet(kCharsToEscape, offset)) != kNotFound) {
      value.Insert('\\', offset);
      offset += 2;
    }

    AppendUTF8toUTF16(name, strAttrs);
    strAttrs.Append(':');
    strAttrs.Append(value);
    strAttrs.Append(';');
  }

  *aAttributes = ::SysAllocString(strAttrs.get());
  return S_OK;
}

STDMETHODIMP
nsAccessibleWrap::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
  
  
  *ppenum = nsnull;

  nsAccessibleWrap *accessibleWrap = new nsAccessibleWrap(mDOMNode, mWeakShell);
  if (!accessibleWrap)
    return E_FAIL;

  IAccessible *msaaAccessible = static_cast<IAccessible*>(accessibleWrap);
  msaaAccessible->AddRef();
  QueryInterface(IID_IEnumVARIANT, (void**)ppenum);
  if (*ppenum)
    (*ppenum)->Skip(mEnumVARIANTPosition); 
  msaaAccessible->Release();

  return NOERROR;
}



STDMETHODIMP
nsAccessibleWrap::GetTypeInfoCount(UINT *p)
{
  *p = 0;
  return E_NOTIMPL;
}


STDMETHODIMP nsAccessibleWrap::GetTypeInfo(UINT i, LCID lcid, ITypeInfo **ppti)
{
  *ppti = 0;
  return E_NOTIMPL;
}


STDMETHODIMP
nsAccessibleWrap::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                           UINT cNames, LCID lcid, DISPID *rgDispId)
{
  return E_NOTIMPL;
}


STDMETHODIMP nsAccessibleWrap::Invoke(DISPID dispIdMember, REFIID riid,
    LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
    VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
  return E_NOTIMPL;
}


NS_IMETHODIMP nsAccessibleWrap::GetNativeInterface(void **aOutAccessible)
{
  *aOutAccessible = static_cast<IAccessible*>(this);
  NS_ADDREF_THIS();
  return NS_OK;
}



NS_IMETHODIMP
nsAccessibleWrap::FireAccessibleEvent(nsIAccessibleEvent *aEvent)
{
  NS_ENSURE_ARG(aEvent);

  nsresult rv = nsAccessible::FireAccessibleEvent(aEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 eventType = 0;
  aEvent->GetEventType(&eventType);

  NS_ENSURE_TRUE(eventType > 0 &&
                 eventType < nsIAccessibleEvent::EVENT_LAST_ENTRY,
                 NS_ERROR_FAILURE);

  PRUint32 winLastEntry = gWinEventMap[nsIAccessibleEvent::EVENT_LAST_ENTRY];
  NS_ASSERTION(winLastEntry == kEVENT_LAST_ENTRY,
               "MSAA event map skewed");

  PRUint32 winEvent = gWinEventMap[eventType];
  if (!winEvent)
    return NS_OK;

  
  NS_ENSURE_TRUE(mWeakShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIAccessible> accessible;
  aEvent->GetAccessible(getter_AddRefs(accessible));
  if (!accessible)
    return NS_OK;

  PRUint32 role = ROLE_SYSTEM_TEXT; 

  nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(accessible));
  NS_ENSURE_STATE(accessNode);

  if (eventType == nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED ||
      eventType == nsIAccessibleEvent::EVENT_FOCUS) {
    UpdateSystemCaret();
  }
 
  PRInt32 childID = GetChildIDFor(accessible); 
  if (!childID)
    return NS_OK; 

  
  nsCOMPtr<nsIAccessible> newAccessible;
  if (eventType == nsIAccessibleEvent::EVENT_HIDE) {
    
    
    accessible->GetParent(getter_AddRefs(newAccessible));
  } else {
    newAccessible = accessible;
  }

  HWND hWnd = GetHWNDFor(accessible);
  NS_ENSURE_TRUE(hWnd, NS_ERROR_FAILURE);

  
  
  
  
  

  
  NotifyWinEvent(winEvent, hWnd, OBJID_CLIENT, childID);

  return NS_OK;
}



PRInt32 nsAccessibleWrap::GetChildIDFor(nsIAccessible* aAccessible)
{
  
  
  

  void *uniqueID;
  nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(aAccessible));
  if (!accessNode) {
    return 0;
  }
  accessNode->GetUniqueID(&uniqueID);

  
  
  return - NS_PTR_TO_INT32(uniqueID);
}

HWND
nsAccessibleWrap::GetHWNDFor(nsIAccessible *aAccessible)
{
  nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(aAccessible));
  nsCOMPtr<nsPIAccessNode> privateAccessNode(do_QueryInterface(accessNode));
  if (!privateAccessNode)
    return 0;

  HWND hWnd = 0;

  nsIFrame *frame = privateAccessNode->GetFrame();
  if (frame) {
    nsIWidget *window = frame->GetWindow();
    PRBool isVisible;
    window->IsVisible(isVisible);
    if (isVisible) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      hWnd = (HWND)frame->GetWindow()->GetNativeData(NS_NATIVE_WINDOW);
    }
  }

  if (!hWnd) {
    void* handle = nsnull;
    nsCOMPtr<nsIAccessibleDocument> accessibleDoc;
    accessNode->GetAccessibleDocument(getter_AddRefs(accessibleDoc));
    if (!accessibleDoc)
      return 0;

    accessibleDoc->GetWindowHandle(&handle);
    hWnd = (HWND)handle;
  }

  return hWnd;
}

IDispatch *nsAccessibleWrap::NativeAccessible(nsIAccessible *aXPAccessible)
{
  if (!aXPAccessible) {
   NS_WARNING("Not passing in an aXPAccessible");
   return NULL;
  }

  nsCOMPtr<nsIAccessibleWin32Object> accObject(do_QueryInterface(aXPAccessible));
  if (accObject) {
    void* hwnd;
    accObject->GetHwnd(&hwnd);
    if (hwnd) {
      IDispatch *retval = nsnull;
      AccessibleObjectFromWindow(reinterpret_cast<HWND>(hwnd),
        OBJID_WINDOW, IID_IAccessible, (void **) &retval);
      return retval;
    }
  }

  IAccessible *msaaAccessible;
  aXPAccessible->GetNativeInterface((void**)&msaaAccessible);

  return static_cast<IDispatch*>(msaaAccessible);
}


void nsAccessibleWrap::GetXPAccessibleFor(const VARIANT& aVarChild, nsIAccessible **aXPAccessible)
{
  *aXPAccessible = nsnull;
  if (!mWeakShell)
    return; 

  
  if (aVarChild.lVal == CHILDID_SELF) {
    *aXPAccessible = static_cast<nsIAccessible*>(this);
  }
  else if (MustPrune(this)) {
    return;
  }
  else {
    
    
    
    
    nsCOMPtr<nsIAccessible> xpAccessible, nextAccessible;
    GetFirstChild(getter_AddRefs(xpAccessible));
    for (PRInt32 index = 0; xpAccessible; index ++) {
      if (!xpAccessible)
        break; 
      if (aVarChild.lVal == index) {
        *aXPAccessible = xpAccessible;
        break;
      }
      nextAccessible = xpAccessible;
      nextAccessible->GetNextSibling(getter_AddRefs(xpAccessible));
    }
  }
  NS_IF_ADDREF(*aXPAccessible);
}

void nsAccessibleWrap::UpdateSystemCaret()
{
  
  
  ::DestroyCaret();

  nsRefPtr<nsRootAccessible> rootAccessible = GetRootAccessible();
  if (!rootAccessible) {
    return;
  }

  nsRefPtr<nsCaretAccessible> caretAccessible = rootAccessible->GetCaretAccessible();
  if (!caretAccessible) {
    return;
  }

  nsIWidget *widget;
  nsRect caretRect = caretAccessible->GetCaretRect(&widget);        
  HWND caretWnd; 
  if (caretRect.IsEmpty() || !(caretWnd = (HWND)widget->GetNativeData(NS_NATIVE_WINDOW))) {
    return;
  }

  
  
  HBITMAP caretBitMap = CreateBitmap(1, caretRect.height, 1, 1, NULL);
  if (::CreateCaret(caretWnd, caretBitMap, 1, caretRect.height)) {  
    ::ShowCaret(caretWnd);
    RECT windowRect;
    ::GetWindowRect(caretWnd, &windowRect);
    ::SetCaretPos(caretRect.x - windowRect.left, caretRect.y - windowRect.top);
    ::DeleteObject(caretBitMap);
  }
}

PRBool nsAccessibleWrap::MustPrune(nsIAccessible *aAccessible)
{ 
  PRUint32 role = Role(aAccessible);
  return role == nsIAccessibleRole::ROLE_MENUITEM || 
         role == nsIAccessibleRole::ROLE_ENTRY ||
         role == nsIAccessibleRole::ROLE_PASSWORD_TEXT ||
         role == nsIAccessibleRole::ROLE_PUSHBUTTON ||
         role == nsIAccessibleRole::ROLE_TOGGLE_BUTTON ||
         role == nsIAccessibleRole::ROLE_GRAPHIC;
}
