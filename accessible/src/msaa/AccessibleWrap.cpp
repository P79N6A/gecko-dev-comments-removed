




#include "AccessibleWrap.h"

#include "Compatibility.h"
#include "DocAccessible-inl.h"
#include "EnumVariant.h"
#include "ia2AccessibleRelation.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsIAccessibleEvent.h"
#include "nsIAccessibleRelation.h"
#include "nsWinUtils.h"
#include "Relation.h"
#include "Role.h"
#include "RootAccessible.h"
#include "States.h"
#include "uiaRawElmProvider.h"

#ifdef DEBUG
#include "Logging.h"
#endif

#include "nsIMutableArray.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIServiceManager.h"
#include "nsTextFormatter.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsEventMap.h"
#include "nsArrayUtils.h"
#include "mozilla/Preferences.h"

#include "Accessible2_i.c"
#include "AccessibleRole.h"
#include "AccessibleStates.h"
#include "OLEACC.H"

using namespace mozilla;
using namespace mozilla::a11y;

const uint32_t USE_ROLE_STRING = 0;







#ifdef DEBUG_LEAKS
static gAccessibles = 0;
#endif

EXTERN_C GUID CDECL CLSID_Accessible =
{ 0x61044601, 0xa811, 0x4e2b, { 0xbb, 0xba, 0x17, 0xbf, 0xab, 0xd3, 0x29, 0xd7 } };

static const int32_t kIEnumVariantDisconnected = -1;





ITypeInfo* AccessibleWrap::gTypeInfo = NULL;

NS_IMPL_ISUPPORTS_INHERITED0(AccessibleWrap, Accessible);






STDMETHODIMP
AccessibleWrap::QueryInterface(REFIID iid, void** ppv)
{
__try {
  *ppv = NULL;

  if (IID_IUnknown == iid || IID_IDispatch == iid || IID_IAccessible == iid)
    *ppv = static_cast<IAccessible*>(this);
  else if (IID_IEnumVARIANT == iid) {
    
    if (!HasChildren() || nsAccUtils::MustPrune(this))
      return E_NOINTERFACE;

    *ppv = static_cast<IEnumVARIANT*>(new ChildrenEnumVariant(this));
  } else if (IID_IServiceProvider == iid)
    *ppv = static_cast<IServiceProvider*>(this);
  else if (IID_IAccessible2 == iid && !Compatibility::IsIA2Off())
    *ppv = static_cast<IAccessible2*>(this);

  if (NULL == *ppv) {
    HRESULT hr = ia2AccessibleComponent::QueryInterface(iid, ppv);
    if (SUCCEEDED(hr))
      return hr;
  }

  if (NULL == *ppv) {
    HRESULT hr = ia2AccessibleHyperlink::QueryInterface(iid, ppv);
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
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}




STDMETHODIMP
AccessibleWrap::QueryService(REFGUID aGuidService, REFIID aIID,
                             void** aInstancePtr)
{
  if (!aInstancePtr)
    return E_INVALIDARG;

  *aInstancePtr = NULL;

  
  if (aGuidService == IID_IAccessibleEx &&
      Preferences::GetBool("accessibility.uia.enable")) {
    IAccessibleEx* accEx = new uiaRawElmProvider(this);
    HRESULT hr = accEx->QueryInterface(aIID, aInstancePtr);
    if (FAILED(hr))
      delete accEx;

    return hr;
  }

  return nsAccessNodeWrap::QueryService(aGuidService, aIID, aInstancePtr);
}





STDMETHODIMP
AccessibleWrap::get_accParent( IDispatch __RPC_FAR *__RPC_FAR *ppdispParent)
{
__try {
  *ppdispParent = NULL;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  DocAccessible* doc = AsDoc();
  if (doc) {
    
    
    if (!doc->ParentDocument() ||
        nsWinUtils::IsWindowEmulationStarted() &&
        nsCoreUtils::IsTabDocument(doc->GetDocumentNode())) {
      HWND hwnd = static_cast<HWND>(doc->GetNativeWindow());
      if (hwnd && SUCCEEDED(::AccessibleObjectFromWindow(hwnd, OBJID_WINDOW,
                                                         IID_IAccessible,
                                                         (void**)ppdispParent))) {
        return S_OK;
      }
    }
  }

  Accessible* xpParentAcc = Parent();
  if (!xpParentAcc) {
    if (IsApplication())
      return S_OK;

    NS_ERROR("No parent accessible. Should we really assert here?");
    return E_UNEXPECTED;
  }

  *ppdispParent = NativeAccessible(xpParentAcc);

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}

STDMETHODIMP
AccessibleWrap::get_accChildCount( long __RPC_FAR *pcountChildren)
{
__try {
  if (!pcountChildren)
    return E_INVALIDARG;

  *pcountChildren = 0;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (nsAccUtils::MustPrune(this))
    return S_OK;

  *pcountChildren = ChildCount();
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
AccessibleWrap::get_accChild(
       VARIANT varChild,
       IDispatch __RPC_FAR *__RPC_FAR *ppdispChild)
{
__try {
  *ppdispChild = NULL;
  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  
  
  
  
  
  Accessible* child = GetXPAccessibleFor(varChild);
  if (!child)
    return E_INVALIDARG;

  if (child->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *ppdispChild = NativeAccessible(child);

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
AccessibleWrap::get_accName(
       VARIANT varChild,
       BSTR __RPC_FAR *pszName)
{
__try {
  *pszName = NULL;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* xpAccessible = GetXPAccessibleFor(varChild);
  if (!xpAccessible)
    return E_INVALIDARG;

  if (xpAccessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsAutoString name;
  xpAccessible->Name(name);

  
  
  
  if (name.IsVoid())
    return S_FALSE;

  *pszName = ::SysAllocStringLen(name.get(), name.Length());
  if (!*pszName)
    return E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}


STDMETHODIMP
AccessibleWrap::get_accValue(
       VARIANT varChild,
       BSTR __RPC_FAR *pszValue)
{
__try {
  *pszValue = NULL;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* xpAccessible = GetXPAccessibleFor(varChild);
  if (!xpAccessible)
    return E_INVALIDARG;

  if (xpAccessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (xpAccessible->NativeRole() == roles::PASSWORD_TEXT)
    return E_ACCESSDENIED;

  nsAutoString value;
  xpAccessible->Value(value);

  
  
  
  if (value.IsEmpty())
    return S_FALSE;

  *pszValue = ::SysAllocStringLen(value.get(), value.Length());
  if (!*pszValue)
    return E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}

STDMETHODIMP
AccessibleWrap::get_accDescription(VARIANT varChild,
                                   BSTR __RPC_FAR *pszDescription)
{
__try {
  *pszDescription = NULL;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* xpAccessible = GetXPAccessibleFor(varChild);
  if (!xpAccessible)
    return E_INVALIDARG;

  if (xpAccessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsAutoString description;
  xpAccessible->Description(description);

  *pszDescription = ::SysAllocStringLen(description.get(),
                                        description.Length());
  return *pszDescription ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_accRole(
       VARIANT varChild,
       VARIANT __RPC_FAR *pvarRole)
{
__try {
  VariantInit(pvarRole);

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* xpAccessible = GetXPAccessibleFor(varChild);
  if (!xpAccessible)
    return E_INVALIDARG;

  if (xpAccessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

#ifdef DEBUG
  NS_ASSERTION(nsAccUtils::IsTextInterfaceSupportCorrect(xpAccessible),
               "Does not support nsIAccessibleText when it should");
#endif

  a11y::role geckoRole = xpAccessible->Role();
  uint32_t msaaRole = 0;

#define ROLE(_geckoRole, stringRole, atkRole, macRole, \
             _msaaRole, ia2Role, nameRule) \
  case roles::_geckoRole: \
    msaaRole = _msaaRole; \
    break;

  switch (geckoRole) {
#include "RoleMap.h"
    default:
      MOZ_NOT_REACHED("Unknown role.");
  };

#undef ROLE

  
  
  
  if (geckoRole == roles::ROW) {
    Accessible* xpParent = Parent();
    if (xpParent && xpParent->Role() == roles::TREE_TABLE)
      msaaRole = ROLE_SYSTEM_OUTLINEITEM;
  }
  
  
  if (msaaRole != USE_ROLE_STRING) {
    pvarRole->vt = VT_I4;
    pvarRole->lVal = msaaRole;  
    return S_OK;
  }

  
  
  
  nsIContent *content = xpAccessible->GetContent();
  if (!content)
    return E_FAIL;

  if (content->IsElement()) {
    nsAutoString roleString;
    if (msaaRole != ROLE_SYSTEM_CLIENT &&
        !content->GetAttr(kNameSpaceID_None, nsGkAtoms::role, roleString)) {
      nsIDocument * document = content->GetCurrentDoc();
      if (!document)
        return E_FAIL;

      nsINodeInfo *nodeInfo = content->NodeInfo();
      nodeInfo->GetName(roleString);

      
      if (!nodeInfo->NamespaceEquals(document->GetDefaultNamespaceID())) {
        nsAutoString nameSpaceURI;
        nodeInfo->GetNamespaceURI(nameSpaceURI);
        roleString += NS_LITERAL_STRING(", ") + nameSpaceURI;
      }
    }

    if (!roleString.IsEmpty()) {
      pvarRole->vt = VT_BSTR;
      pvarRole->bstrVal = ::SysAllocString(roleString.get());
      return S_OK;
    }
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_accState(
       VARIANT varChild,
       VARIANT __RPC_FAR *pvarState)
{
__try {
  VariantInit(pvarState);
  pvarState->vt = VT_I4;
  pvarState->lVal = 0;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* xpAccessible = GetXPAccessibleFor(varChild);
  if (!xpAccessible)
    return E_INVALIDARG;

  if (xpAccessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  
  
  
  
  
  
  

  uint32_t msaaState = 0;
  nsAccUtils::To32States(xpAccessible->State(), &msaaState, nullptr);
  pvarState->lVal = msaaState;
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}


STDMETHODIMP
AccessibleWrap::get_accHelp(
       VARIANT varChild,
       BSTR __RPC_FAR *pszHelp)
{
__try {
  *pszHelp = NULL;
  return S_FALSE;

} __except(FilterA11yExceptions(::GetExceptionCode(),
                                GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_accHelpTopic(
       BSTR __RPC_FAR *pszHelpFile,
       VARIANT varChild,
       long __RPC_FAR *pidTopic)
{
__try {
  *pszHelpFile = NULL;
  *pidTopic = 0;
  return S_FALSE;

} __except(FilterA11yExceptions(::GetExceptionCode(),
                                GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_accKeyboardShortcut(
       VARIANT varChild,
       BSTR __RPC_FAR *pszKeyboardShortcut)
{
__try {
  if (!pszKeyboardShortcut)
    return E_INVALIDARG;
  *pszKeyboardShortcut = NULL;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* acc = GetXPAccessibleFor(varChild);
  if (!acc)
    return E_INVALIDARG;

  if (acc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  KeyBinding keyBinding = acc->AccessKey();
  if (keyBinding.IsEmpty())
    keyBinding = acc->KeyboardShortcut();

  nsAutoString shortcut;
  keyBinding.ToString(shortcut);

  *pszKeyboardShortcut = ::SysAllocStringLen(shortcut.get(),
                                             shortcut.Length());
  return *pszKeyboardShortcut ? S_OK : E_OUTOFMEMORY;
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_accFocus(
       VARIANT __RPC_FAR *pvarChild)
{
  
  
  
  
  
  
__try {
  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  VariantInit(pvarChild);

  
  Accessible* focusedAccessible = FocusedChild();
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

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
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
  uint32_t mCurIndex;
  nsAutoRefCnt mRefCnt;
};

HRESULT
AccessibleEnumerator::QueryInterface(REFIID iid, void ** ppvObject)
{
__try {
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
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
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
__try {
  uint32_t length = 0;
  mArray->GetLength(&length);

  HRESULT hr = S_OK;

  
  if (celt > length - mCurIndex) {
    hr = S_FALSE;
    celt = length - mCurIndex;
  }

  for (uint32_t i = 0; i < celt; ++i, ++mCurIndex) {
    
    nsCOMPtr<nsIAccessible> accel(do_QueryElementAt(mArray, mCurIndex));
    NS_ASSERTION(accel, "Invalid pointer in mArray");

    if (accel) {
      rgvar[i].vt = VT_DISPATCH;
      rgvar[i].pdispVal = AccessibleWrap::NativeAccessible(accel);
    }
  }

  if (pceltFetched)
    *pceltFetched = celt;

  return hr;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
AccessibleEnumerator::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
__try {
  *ppenum = new AccessibleEnumerator(*this);
  if (!*ppenum)
    return E_OUTOFMEMORY;
  NS_ADDREF(*ppenum);
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}

STDMETHODIMP
AccessibleEnumerator::Skip(unsigned long celt)
{
__try {
  uint32_t length = 0;
  mArray->GetLength(&length);
  
  if (celt > length - mCurIndex) {
    mCurIndex = length;
    return S_FALSE;
  }
  mCurIndex += celt;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}


















STDMETHODIMP
AccessibleWrap::get_accSelection(VARIANT __RPC_FAR *pvarChildren)
{
__try {
  VariantInit(pvarChildren);
  pvarChildren->vt = VT_EMPTY;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (IsSelect()) {
    nsCOMPtr<nsIArray> selectedItems = SelectedItems();
    if (selectedItems) {
      
      nsRefPtr<AccessibleEnumerator> pEnum =
        new AccessibleEnumerator(selectedItems);

      
      if (!pEnum)
        return E_OUTOFMEMORY;
      pvarChildren->vt = VT_UNKNOWN;    
      NS_ADDREF(pvarChildren->punkVal = pEnum);
    }
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}

STDMETHODIMP
AccessibleWrap::get_accDefaultAction(
       VARIANT varChild,
       BSTR __RPC_FAR *pszDefaultAction)
{
__try {
  *pszDefaultAction = NULL;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* xpAccessible = GetXPAccessibleFor(varChild);
  if (!xpAccessible)
    return E_INVALIDARG;

  if (xpAccessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsAutoString defaultAction;
  if (NS_FAILED(xpAccessible->GetActionName(0, defaultAction)))
    return E_FAIL;

  *pszDefaultAction = ::SysAllocStringLen(defaultAction.get(),
                                          defaultAction.Length());
  return *pszDefaultAction ? S_OK : E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::accSelect(
       long flagsSelect,
       VARIANT varChild)
{
__try {
  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  
  Accessible* xpAccessible = GetXPAccessibleFor(varChild);
  if (!xpAccessible)
    return E_INVALIDARG;

  if (xpAccessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (flagsSelect & (SELFLAG_TAKEFOCUS|SELFLAG_TAKESELECTION|SELFLAG_REMOVESELECTION))
  {
    if (flagsSelect & SELFLAG_TAKEFOCUS)
      xpAccessible->TakeFocus();

    if (flagsSelect & SELFLAG_TAKESELECTION)
      xpAccessible->TakeSelection();

    if (flagsSelect & SELFLAG_ADDSELECTION)
      xpAccessible->SetSelected(true);

    if (flagsSelect & SELFLAG_REMOVESELECTION)
      xpAccessible->SetSelected(false);

    if (flagsSelect & SELFLAG_EXTENDSELECTION)
      xpAccessible->ExtendSelection();

    return S_OK;
  }

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::accLocation(
       long __RPC_FAR *pxLeft,
       long __RPC_FAR *pyTop,
       long __RPC_FAR *pcxWidth,
       long __RPC_FAR *pcyHeight,
       VARIANT varChild)
{
__try {
  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* xpAccessible = GetXPAccessibleFor(varChild);
  if (!xpAccessible)
    return E_INVALIDARG;

  if (xpAccessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  int32_t x, y, width, height;
  if (NS_FAILED(xpAccessible->GetBounds(&x, &y, &width, &height)))
    return E_FAIL;

  *pxLeft = x;
  *pyTop = y;
  *pcxWidth = width;
  *pcyHeight = height;
  return S_OK;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::accNavigate(
       long navDir,
       VARIANT varStart,
       VARIANT __RPC_FAR *pvarEndUpAt)
{
__try {
  if (!pvarEndUpAt)
    return E_INVALIDARG;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* accessible = GetXPAccessibleFor(varStart);
  if (!accessible)
    return E_INVALIDARG;

  if (accessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  VariantInit(pvarEndUpAt);

  Accessible* navAccessible = nullptr;
  uint32_t xpRelation = 0;

  switch(navDir) {
    case NAVDIR_FIRSTCHILD:
      if (!nsAccUtils::MustPrune(accessible))
        navAccessible = accessible->FirstChild();
      break;
    case NAVDIR_LASTCHILD:
      if (!nsAccUtils::MustPrune(accessible))
        navAccessible = accessible->LastChild();
      break;
    case NAVDIR_NEXT:
      navAccessible = accessible->NextSibling();
      break;
    case NAVDIR_PREVIOUS:
      navAccessible = accessible->PrevSibling();
      break;
    case NAVDIR_DOWN:
    case NAVDIR_LEFT:
    case NAVDIR_RIGHT:
    case NAVDIR_UP:
      return E_NOTIMPL;

    
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

    default:
      return E_INVALIDARG;
  }

  pvarEndUpAt->vt = VT_EMPTY;

  if (xpRelation) {
    Relation rel = RelationByType(xpRelation);
    navAccessible = rel.Next();
  }

  if (navAccessible) {
    pvarEndUpAt->pdispVal = NativeAccessible(navAccessible);
    pvarEndUpAt->vt = VT_DISPATCH;
    return S_OK;
  }
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::accHitTest(
       long xLeft,
       long yTop,
       VARIANT __RPC_FAR *pvarChild)
{
__try {
  VariantInit(pvarChild);

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* accessible = ChildAtPoint(xLeft, yTop, eDirectChild);

  
  if (accessible) {
    
    if (accessible == this) {
      pvarChild->vt = VT_I4;
      pvarChild->lVal = CHILDID_SELF;
    } else { 
      pvarChild->vt = VT_DISPATCH;
      pvarChild->pdispVal = NativeAccessible(accessible);
    }
  } else {
    
    pvarChild->vt = VT_EMPTY;
    return S_FALSE;
  }
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
AccessibleWrap::accDoDefaultAction(
       VARIANT varChild)
{
__try {
  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* xpAccessible = GetXPAccessibleFor(varChild);
  if (!xpAccessible)
    return E_INVALIDARG;

  if (xpAccessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  return GetHRESULT(xpAccessible->DoAction(0));

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::put_accName(
       VARIANT varChild,
       BSTR szName)
{
  return E_NOTIMPL;
}

STDMETHODIMP
AccessibleWrap::put_accValue(
       VARIANT varChild,
       BSTR szValue)
{
  return E_NOTIMPL;
}




STDMETHODIMP
AccessibleWrap::get_nRelations(long *aNRelations)
{
__try {
  if (!aNRelations)
    return E_INVALIDARG;

  *aNRelations = 0;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  for (uint32_t relType = nsIAccessibleRelation::RELATION_FIRST;
       relType <= nsIAccessibleRelation::RELATION_LAST; relType++) {
    Relation rel = RelationByType(relType);
    if (rel.Next())
      (*aNRelations)++;
  }
  return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_relation(long aRelationIndex,
                             IAccessibleRelation** aRelation)
{
__try {
  if (!aRelation)
    return E_INVALIDARG;

  *aRelation = NULL;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  uint32_t relIdx = 0;
  for (uint32_t relType = nsIAccessibleRelation::RELATION_FIRST;
       relType <= nsIAccessibleRelation::RELATION_LAST; relType++) {
    Relation rel = RelationByType(relType);
    nsRefPtr<ia2AccessibleRelation> ia2Relation =
      new ia2AccessibleRelation(relType, &rel);
    if (ia2Relation->HasTargets()) {
      if (relIdx == aRelationIndex) {
        ia2Relation.forget(aRelation);
        return S_OK;
      }

      relIdx++;
    }
  }

  return E_INVALIDARG;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_relations(long aMaxRelations,
                                IAccessibleRelation **aRelation,
                                long *aNRelations)
{
__try {
  if (!aRelation || !aNRelations)
    return E_INVALIDARG;

  *aNRelations = 0;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  for (uint32_t relType = nsIAccessibleRelation::RELATION_FIRST;
       relType <= nsIAccessibleRelation::RELATION_LAST &&
       *aNRelations < aMaxRelations; relType++) {
    Relation rel = RelationByType(relType);
    nsRefPtr<ia2AccessibleRelation> ia2Rel =
      new ia2AccessibleRelation(relType, &rel);
    if (ia2Rel->HasTargets()) {
      ia2Rel.forget(aRelation + (*aNRelations));
      (*aNRelations)++;
    }
  }
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::role(long *aRole)
{
__try {
  *aRole = 0;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

#define ROLE(_geckoRole, stringRole, atkRole, macRole, \
             msaaRole, ia2Role, nameRule) \
  case roles::_geckoRole: \
    *aRole = ia2Role; \
    break;

  a11y::role geckoRole = Role();
  switch (geckoRole) {
#include "RoleMap.h"
    default:
      MOZ_NOT_REACHED("Unknown role.");
  };

#undef ROLE

  
  
  if (geckoRole == roles::ROW) {
    Accessible* xpParent = Parent();
    if (xpParent && xpParent->Role() == roles::TREE_TABLE)
      *aRole = ROLE_SYSTEM_OUTLINEITEM;
  }

  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::scrollTo(enum IA2ScrollType aScrollType)
{
__try {
  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsCoreUtils::ScrollTo(mDoc->PresShell(), mContent, aScrollType);
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::scrollToPoint(enum IA2CoordinateType aCoordType,
                              long aX, long aY)
{
__try {
  if (IsDefunct())
      return CO_E_OBJNOTCONNECTED;

  uint32_t geckoCoordType = (aCoordType == IA2_COORDTYPE_SCREEN_RELATIVE) ?
    nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE :
    nsIAccessibleCoordinateType::COORDTYPE_PARENT_RELATIVE;

  nsresult rv = ScrollToPoint(geckoCoordType, aX, aY);
  return GetHRESULT(rv);

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_groupPosition(long *aGroupLevel,
                                  long *aSimilarItemsInGroup,
                                  long *aPositionInGroup)
{
__try {
  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  GroupPos groupPos = GroupPosition();

  
  
  
  if (!groupPos.setSize && !groupPos.posInSet)
    return S_FALSE;

  *aGroupLevel = groupPos.level;
  *aSimilarItemsInGroup = groupPos.setSize;
  *aPositionInGroup = groupPos.posInSet;

  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_states(AccessibleStates *aStates)
{
__try {
  *aStates = 0;

  

  uint64_t state = State();

  if (state & states::INVALID)
    *aStates |= IA2_STATE_INVALID_ENTRY;
  if (state & states::REQUIRED)
    *aStates |= IA2_STATE_REQUIRED;

  
  
  
  
  

  if (state & states::ACTIVE)
    *aStates |= IA2_STATE_ACTIVE;
  if (state & states::DEFUNCT)
    *aStates |= IA2_STATE_DEFUNCT;
  if (state & states::EDITABLE)
    *aStates |= IA2_STATE_EDITABLE;
  if (state & states::HORIZONTAL)
    *aStates |= IA2_STATE_HORIZONTAL;
  if (state & states::MODAL)
    *aStates |= IA2_STATE_MODAL;
  if (state & states::MULTI_LINE)
    *aStates |= IA2_STATE_MULTI_LINE;
  if (state & states::OPAQUE1)
    *aStates |= IA2_STATE_OPAQUE;
  if (state & states::SELECTABLE_TEXT)
    *aStates |= IA2_STATE_SELECTABLE_TEXT;
  if (state & states::SINGLE_LINE)
    *aStates |= IA2_STATE_SINGLE_LINE;
  if (state & states::STALE)
    *aStates |= IA2_STATE_STALE;
  if (state & states::SUPPORTS_AUTOCOMPLETION)
    *aStates |= IA2_STATE_SUPPORTS_AUTOCOMPLETION;
  if (state & states::TRANSIENT)
    *aStates |= IA2_STATE_TRANSIENT;
  if (state & states::VERTICAL)
    *aStates |= IA2_STATE_VERTICAL;

  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_extendedRole(BSTR *aExtendedRole)
{
__try {
  *aExtendedRole = NULL;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_NOTIMPL;
}

STDMETHODIMP
AccessibleWrap::get_localizedExtendedRole(BSTR *aLocalizedExtendedRole)
{
__try {
  *aLocalizedExtendedRole = NULL;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_NOTIMPL;
}

STDMETHODIMP
AccessibleWrap::get_nExtendedStates(long *aNExtendedStates)
{
__try {
  *aNExtendedStates = 0;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_NOTIMPL;
}

STDMETHODIMP
AccessibleWrap::get_extendedStates(long aMaxExtendedStates,
                                   BSTR **aExtendedStates,
                                   long *aNExtendedStates)
{
__try {
  *aExtendedStates = NULL;
  *aNExtendedStates = 0;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_NOTIMPL;
}

STDMETHODIMP
AccessibleWrap::get_localizedExtendedStates(long aMaxLocalizedExtendedStates,
                                            BSTR** aLocalizedExtendedStates,
                                            long* aNLocalizedExtendedStates)
{
__try {
  *aLocalizedExtendedStates = NULL;
  *aNLocalizedExtendedStates = 0;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_NOTIMPL;
}

STDMETHODIMP
AccessibleWrap::get_uniqueID(long *uniqueID)
{
__try {
  *uniqueID = - reinterpret_cast<long>(UniqueID());
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_windowHandle(HWND *aWindowHandle)
{
__try {
  *aWindowHandle = 0;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *aWindowHandle = GetHWNDFor(this);
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_indexInParent(long *aIndexInParent)
{
__try {
  if (!aIndexInParent)
    return E_INVALIDARG;

  *aIndexInParent = -1;
  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *aIndexInParent = IndexInParent();
  if (*aIndexInParent == -1)
    return S_FALSE;

  return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_locale(IA2Locale *aLocale)
{
__try {
  
  
  
  

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsAutoString lang;
  Language(lang);

  
  int32_t offset = lang.FindChar('-', 0);
  if (offset == -1) {
    if (lang.Length() == 2) {
      aLocale->language = ::SysAllocString(lang.get());
      return S_OK;
    }
  } else if (offset == 2) {
    aLocale->language = ::SysAllocStringLen(lang.get(), 2);

    
    
    offset = lang.FindChar('-', 3);
    if (offset == -1) {
      if (lang.Length() == 5) {
        aLocale->country = ::SysAllocString(lang.get() + 3);
        return S_OK;
      }
    } else if (offset == 5) {
      aLocale->country = ::SysAllocStringLen(lang.get() + 3, 2);
    }
  }

  
  
  aLocale->variant = ::SysAllocString(lang.get());
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
AccessibleWrap::get_attributes(BSTR *aAttributes)
{
  
  
__try {
  *aAttributes = NULL;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsCOMPtr<nsIPersistentProperties> attributes;
  nsresult rv = GetAttributes(getter_AddRefs(attributes));
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  return ConvertToIA2Attributes(attributes, aAttributes);

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}




STDMETHODIMP
AccessibleWrap::GetTypeInfoCount(UINT *pctinfo)
{
  *pctinfo = 1;
  return S_OK;
}

STDMETHODIMP
AccessibleWrap::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
  *ppTInfo = NULL;

  if (iTInfo != 0)
    return DISP_E_BADINDEX;

  ITypeInfo * typeInfo = GetTI(lcid);
  if (!typeInfo)
    return E_FAIL;

  typeInfo->AddRef();
  *ppTInfo = typeInfo;

  return S_OK;
}

STDMETHODIMP
AccessibleWrap::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                              UINT cNames, LCID lcid, DISPID *rgDispId)
{
  ITypeInfo *typeInfo = GetTI(lcid);
  if (!typeInfo)
    return E_FAIL;

  HRESULT hr = DispGetIDsOfNames(typeInfo, rgszNames, cNames, rgDispId);
  return hr;
}

STDMETHODIMP
AccessibleWrap::Invoke(DISPID dispIdMember, REFIID riid,
                       LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                       VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                       UINT *puArgErr)
{
  ITypeInfo *typeInfo = GetTI(lcid);
  if (!typeInfo)
    return E_FAIL;

  return typeInfo->Invoke(static_cast<IAccessible*>(this), dispIdMember,
                          wFlags, pDispParams, pVarResult, pExcepInfo,
                          puArgErr);
}



NS_IMETHODIMP
AccessibleWrap::GetNativeInterface(void **aOutAccessible)
{
  *aOutAccessible = static_cast<IAccessible*>(this);
  NS_ADDREF_THIS();
  return NS_OK;
}




nsresult
AccessibleWrap::HandleAccEvent(AccEvent* aEvent)
{
  nsresult rv = Accessible::HandleAccEvent(aEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  return FirePlatformEvent(aEvent);
}




nsresult
AccessibleWrap::FirePlatformEvent(AccEvent* aEvent)
{
  uint32_t eventType = aEvent->GetEventType();

  NS_ENSURE_TRUE(eventType > 0 &&
                 eventType < nsIAccessibleEvent::EVENT_LAST_ENTRY,
                 NS_ERROR_FAILURE);

  uint32_t winLastEntry = gWinEventMap[nsIAccessibleEvent::EVENT_LAST_ENTRY];
  NS_ASSERTION(winLastEntry == kEVENT_LAST_ENTRY,
               "MSAA event map skewed");

  uint32_t winEvent = gWinEventMap[eventType];
  if (!winEvent)
    return NS_OK;

  
  NS_ENSURE_TRUE(!IsDefunct(), NS_ERROR_FAILURE);

  Accessible* accessible = aEvent->GetAccessible();
  if (!accessible)
    return NS_OK;

  if (eventType == nsIAccessibleEvent::EVENT_TEXT_CARET_MOVED ||
      eventType == nsIAccessibleEvent::EVENT_FOCUS) {
    UpdateSystemCaret();
  }

  int32_t childID = GetChildIDFor(accessible); 
  if (!childID)
    return NS_OK; 

  HWND hWnd = GetHWNDFor(accessible);
  NS_ENSURE_TRUE(hWnd, NS_ERROR_FAILURE);

  nsAutoString tag;
  nsCAutoString id;
  nsIContent* cnt = accessible->GetContent();
  if (cnt) {
    cnt->Tag()->ToString(tag);
    nsIAtom* aid = cnt->GetID();
    if (aid)
      aid->ToUTF8String(id);
  }

#ifdef DEBUG
  if (logging::IsEnabled(logging::ePlatforms)) {
    printf("\n\nMSAA event: event: %d, target: %s@id='%s', childid: %d, hwnd: %d\n\n",
           eventType, NS_ConvertUTF16toUTF8(tag).get(), id.get(),
           childID, hWnd);
  }
#endif

  
  ::NotifyWinEvent(winEvent, hWnd, OBJID_CLIENT, childID);

  
  if (Compatibility::IsJAWS()) {
    if (eventType == nsIAccessibleEvent::EVENT_SELECTION &&
      accessible->Role() == roles::COMBOBOX_OPTION) {
      ::NotifyWinEvent(EVENT_OBJECT_FOCUS, hWnd, OBJID_CLIENT, childID);
    }
  }

  return NS_OK;
}



int32_t
AccessibleWrap::GetChildIDFor(Accessible* aAccessible)
{
  
  
  

  
  
  
  return aAccessible ? - NS_PTR_TO_INT32(aAccessible->UniqueID()) : 0;
}

HWND
AccessibleWrap::GetHWNDFor(Accessible* aAccessible)
{
  if (aAccessible) {
    DocAccessible* document = aAccessible->Document();
    if(!document)
      return nullptr;

    
    
    
    nsIFrame* frame = aAccessible->GetFrame();
    if (frame) {
      nsIWidget* widget = frame->GetNearestWidget();
      if (widget && widget->IsVisible()) {
        nsIPresShell* shell = document->PresShell();
        nsIViewManager* vm = shell->GetViewManager();
        if (vm) {
          nsCOMPtr<nsIWidget> rootWidget;
          vm->GetRootWidget(getter_AddRefs(rootWidget));
          
          
          
          if (rootWidget != widget)
            return static_cast<HWND>(widget->GetNativeData(NS_NATIVE_WINDOW));
        }
      }
    }

    return static_cast<HWND>(document->GetNativeWindow());
  }
  return nullptr;
}

HRESULT
AccessibleWrap::ConvertToIA2Attributes(nsIPersistentProperties *aAttributes,
                                       BSTR *aIA2Attributes)
{
  *aIA2Attributes = NULL;

  
  

  if (!aAttributes)
    return S_FALSE;

  nsCOMPtr<nsISimpleEnumerator> propEnum;
  aAttributes->Enumerate(getter_AddRefs(propEnum));
  if (!propEnum)
    return E_FAIL;

  nsAutoString strAttrs;

  const char kCharsToEscape[] = ":;=,\\";

  bool hasMore = false;
  while (NS_SUCCEEDED(propEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> propSupports;
    propEnum->GetNext(getter_AddRefs(propSupports));

    nsCOMPtr<nsIPropertyElement> propElem(do_QueryInterface(propSupports));
    if (!propElem)
      return E_FAIL;

    nsCAutoString name;
    if (NS_FAILED(propElem->GetKey(name)))
      return E_FAIL;

    uint32_t offset = 0;
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

  if (strAttrs.IsEmpty())
    return S_FALSE;

  *aIA2Attributes = ::SysAllocStringLen(strAttrs.get(), strAttrs.Length());
  return *aIA2Attributes ? S_OK : E_OUTOFMEMORY;
}

IDispatch*
AccessibleWrap::NativeAccessible(nsIAccessible* aAccessible)
{
  if (!aAccessible) {
   NS_WARNING("Not passing in an aAccessible");
   return NULL;
  }

  IAccessible* msaaAccessible = nullptr;
  aAccessible->GetNativeInterface(reinterpret_cast<void**>(&msaaAccessible));
  return static_cast<IDispatch*>(msaaAccessible);
}

Accessible*
AccessibleWrap::GetXPAccessibleFor(const VARIANT& aVarChild)
{
  if (aVarChild.vt != VT_I4)
    return nullptr;

  
  if (aVarChild.lVal == CHILDID_SELF)
    return this;

  if (nsAccUtils::MustPrune(this))
    return nullptr;

  
  
  

  if (aVarChild.lVal < 0) {
    
    void* uniqueID = reinterpret_cast<void*>(-aVarChild.lVal);

    
    if (IsDoc())
      return AsDoc()->GetAccessibleByUniqueIDInSubtree(uniqueID);

    
    if (ARIARole() == roles::DOCUMENT) {
      DocAccessible* document = Document();
      Accessible* child =
        document->GetAccessibleByUniqueIDInSubtree(uniqueID);

      
      
      Accessible* parent = child ? child->Parent() : nullptr;
      while (parent && parent != document) {
        if (parent == this)
          return child;

        parent = parent->Parent();
      }
    }

    return nullptr;
  }

  
  return GetChildAt(aVarChild.lVal - 1);
}

void
AccessibleWrap::UpdateSystemCaret()
{
  
  
  ::DestroyCaret();

  a11y::RootAccessible* rootAccessible = RootAccessible();
  if (!rootAccessible) {
    return;
  }

  nsRefPtr<nsCaretAccessible> caretAccessible = rootAccessible->GetCaretAccessible();
  if (!caretAccessible) {
    return;
  }

  nsIWidget *widget;
  nsIntRect caretRect = caretAccessible->GetCaretRect(&widget);
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

ITypeInfo*
AccessibleWrap::GetTI(LCID lcid)
{
  if (gTypeInfo)
    return gTypeInfo;

  ITypeLib *typeLib = NULL;
  HRESULT hr = LoadRegTypeLib(LIBID_Accessibility, 1, 0, lcid, &typeLib);
  if (FAILED(hr))
    return NULL;

  hr = typeLib->GetTypeInfoOfGuid(IID_IAccessible, &gTypeInfo);
  typeLib->Release();

  if (FAILED(hr))
    return NULL;

  return gTypeInfo;
}
