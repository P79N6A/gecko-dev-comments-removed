





#include "AccessibleWrap.h"
#include "Accessible-inl.h"

#include "Compatibility.h"
#include "DocAccessible-inl.h"
#include "EnumVariant.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsIAccessibleEvent.h"
#include "nsIAccessibleRelation.h"
#include "nsWinUtils.h"
#include "ServiceProvider.h"
#include "Relation.h"
#include "Role.h"
#include "RootAccessible.h"
#include "sdnAccessible.h"
#include "States.h"

#ifdef A11Y_LOG
#include "Logging.h"
#endif

#include "nsIMutableArray.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIServiceManager.h"
#include "nsTextFormatter.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsEventMap.h"
#include "nsArrayUtils.h"
#include "mozilla/Preferences.h"

#include "oleacc.h"

using namespace mozilla;
using namespace mozilla::a11y;

const uint32_t USE_ROLE_STRING = 0;







#ifdef DEBUG_LEAKS
static gAccessibles = 0;
#endif

static const int32_t kIEnumVariantDisconnected = -1;





ITypeInfo* AccessibleWrap::gTypeInfo = nullptr;

NS_IMPL_ISUPPORTS_INHERITED0(AccessibleWrap, Accessible)






STDMETHODIMP
AccessibleWrap::QueryInterface(REFIID iid, void** ppv)
{
  A11Y_TRYBLOCK_BEGIN

  if (!ppv)
    return E_INVALIDARG;

  *ppv = nullptr;

  if (IID_IUnknown == iid || IID_IDispatch == iid || IID_IAccessible == iid)
    *ppv = static_cast<IAccessible*>(this);
  else if (IID_IEnumVARIANT == iid) {
    
    if (!HasChildren() || nsAccUtils::MustPrune(this))
      return E_NOINTERFACE;

    *ppv = static_cast<IEnumVARIANT*>(new ChildrenEnumVariant(this));
  } else if (IID_IServiceProvider == iid)
    *ppv = new ServiceProvider(this);
  else if (IID_ISimpleDOMNode == iid) {
    if (IsDefunct() || (!HasOwnContent() && !IsDoc()))
      return E_NOINTERFACE;

    *ppv = static_cast<ISimpleDOMNode*>(new sdnAccessible(GetNode()));
  }

  if (nullptr == *ppv) {
    HRESULT hr = ia2Accessible::QueryInterface(iid, ppv);
    if (SUCCEEDED(hr))
      return hr;
  }

  if (nullptr == *ppv) {
    HRESULT hr = ia2AccessibleComponent::QueryInterface(iid, ppv);
    if (SUCCEEDED(hr))
      return hr;
  }

  if (nullptr == *ppv) {
    HRESULT hr = ia2AccessibleHyperlink::QueryInterface(iid, ppv);
    if (SUCCEEDED(hr))
      return hr;
  }

  if (nullptr == *ppv) {
    HRESULT hr = ia2AccessibleValue::QueryInterface(iid, ppv);
    if (SUCCEEDED(hr))
      return hr;
  }

  if (nullptr == *ppv)
    return E_NOINTERFACE;

  (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
  return S_OK;

  A11Y_TRYBLOCK_END
}





STDMETHODIMP
AccessibleWrap::get_accParent( IDispatch __RPC_FAR *__RPC_FAR *ppdispParent)
{
  A11Y_TRYBLOCK_BEGIN

  if (!ppdispParent)
    return E_INVALIDARG;

  *ppdispParent = nullptr;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  DocAccessible* doc = AsDoc();
  if (doc) {
    
    
    if (!doc->ParentDocument() ||
        (nsWinUtils::IsWindowEmulationStarted() &&
         nsCoreUtils::IsTabDocument(doc->DocumentNode()))) {
      HWND hwnd = static_cast<HWND>(doc->GetNativeWindow());
      if (hwnd && SUCCEEDED(::AccessibleObjectFromWindow(hwnd, OBJID_WINDOW,
                                                         IID_IAccessible,
                                                         (void**)ppdispParent))) {
        return S_OK;
      }
    }
  }

  Accessible* xpParentAcc = Parent();
  if (!xpParentAcc)
    return S_FALSE;

  *ppdispParent = NativeAccessible(xpParentAcc);
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::get_accChildCount( long __RPC_FAR *pcountChildren)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pcountChildren)
    return E_INVALIDARG;

  *pcountChildren = 0;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (nsAccUtils::MustPrune(this))
    return S_OK;

  *pcountChildren = ChildCount();
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::get_accChild(
       VARIANT varChild,
       IDispatch __RPC_FAR *__RPC_FAR *ppdispChild)
{
  A11Y_TRYBLOCK_BEGIN

  if (!ppdispChild)
    return E_INVALIDARG;

  *ppdispChild = nullptr;
  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  
  
  
  
  
  Accessible* child = GetXPAccessibleFor(varChild);
  if (!child)
    return E_INVALIDARG;

  if (child->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *ppdispChild = NativeAccessible(child);
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::get_accName(
       VARIANT varChild,
       BSTR __RPC_FAR *pszName)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pszName)
    return E_INVALIDARG;

  *pszName = nullptr;

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
  return S_OK;

  A11Y_TRYBLOCK_END
}


STDMETHODIMP
AccessibleWrap::get_accValue(
       VARIANT varChild,
       BSTR __RPC_FAR *pszValue)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pszValue)
    return E_INVALIDARG;

  *pszValue = nullptr;

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
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::get_accDescription(VARIANT varChild,
                                   BSTR __RPC_FAR *pszDescription)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pszDescription)
    return E_INVALIDARG;

  *pszDescription = nullptr;

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

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::get_accRole(
       VARIANT varChild,
       VARIANT __RPC_FAR *pvarRole)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pvarRole)
    return E_INVALIDARG;

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
      MOZ_CRASH("Unknown role.");
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

  return E_FAIL;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::get_accState(
       VARIANT varChild,
       VARIANT __RPC_FAR *pvarState)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pvarState)
    return E_INVALIDARG;

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
  return S_OK;

  A11Y_TRYBLOCK_END
}


STDMETHODIMP
AccessibleWrap::get_accHelp(
       VARIANT varChild,
       BSTR __RPC_FAR *pszHelp)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pszHelp)
    return E_INVALIDARG;

  *pszHelp = nullptr;
  return S_FALSE;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::get_accHelpTopic(
       BSTR __RPC_FAR *pszHelpFile,
       VARIANT varChild,
       long __RPC_FAR *pidTopic)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pszHelpFile || !pidTopic)
    return E_INVALIDARG;

  *pszHelpFile = nullptr;
  *pidTopic = 0;
  return S_FALSE;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::get_accKeyboardShortcut(
       VARIANT varChild,
       BSTR __RPC_FAR *pszKeyboardShortcut)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pszKeyboardShortcut)
    return E_INVALIDARG;
  *pszKeyboardShortcut = nullptr;

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

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::get_accFocus(
       VARIANT __RPC_FAR *pvarChild)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pvarChild)
    return E_INVALIDARG;

  VariantInit(pvarChild);

  
  
  
  
  
  
  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  
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

  return S_OK;

  A11Y_TRYBLOCK_END
}



class AccessibleEnumerator MOZ_FINAL : public IEnumVARIANT
{
public:
  AccessibleEnumerator(nsIArray* aArray) : mArray(aArray), mCurIndex(0) { }
  AccessibleEnumerator(const AccessibleEnumerator& toCopy) :
    mArray(toCopy.mArray), mCurIndex(toCopy.mCurIndex) { }
  ~AccessibleEnumerator() { }

  
  DECL_IUNKNOWN

  
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
};

STDMETHODIMP
AccessibleEnumerator::QueryInterface(REFIID iid, void ** ppvObject)
{
  A11Y_TRYBLOCK_BEGIN

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

  *ppvObject = nullptr;
  return E_NOINTERFACE;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleEnumerator::Next(unsigned long celt, VARIANT FAR* rgvar, unsigned long FAR* pceltFetched)
{
  A11Y_TRYBLOCK_BEGIN

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

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleEnumerator::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
  A11Y_TRYBLOCK_BEGIN

  *ppenum = new AccessibleEnumerator(*this);
  if (!*ppenum)
    return E_OUTOFMEMORY;
  NS_ADDREF(*ppenum);
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleEnumerator::Skip(unsigned long celt)
{
  A11Y_TRYBLOCK_BEGIN

  uint32_t length = 0;
  mArray->GetLength(&length);
  
  if (celt > length - mCurIndex) {
    mCurIndex = length;
    return S_FALSE;
  }
  mCurIndex += celt;
  return S_OK;

  A11Y_TRYBLOCK_END
}


















STDMETHODIMP
AccessibleWrap::get_accSelection(VARIANT __RPC_FAR *pvarChildren)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pvarChildren)
    return E_INVALIDARG;

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
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::get_accDefaultAction(
       VARIANT varChild,
       BSTR __RPC_FAR *pszDefaultAction)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pszDefaultAction)
    return E_INVALIDARG;

  *pszDefaultAction = nullptr;

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

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::accSelect(
       long flagsSelect,
       VARIANT varChild)
{
  A11Y_TRYBLOCK_BEGIN

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

  return E_FAIL;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::accLocation(
       long __RPC_FAR *pxLeft,
       long __RPC_FAR *pyTop,
       long __RPC_FAR *pcxWidth,
       long __RPC_FAR *pcyHeight,
       VARIANT varChild)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pxLeft || !pyTop || !pcxWidth || !pcyHeight)
    return E_INVALIDARG;

  *pxLeft = 0;
  *pyTop = 0;
  *pcxWidth = 0;
  *pcyHeight = 0;

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

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::accNavigate(
       long navDir,
       VARIANT varStart,
       VARIANT __RPC_FAR *pvarEndUpAt)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pvarEndUpAt)
    return E_INVALIDARG;

  VariantInit(pvarEndUpAt);

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* accessible = GetXPAccessibleFor(varStart);
  if (!accessible)
    return E_INVALIDARG;

  if (accessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* navAccessible = nullptr;
  Maybe<RelationType> xpRelation;

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
      xpRelation.construct(RelationType::CONTROLLED_BY);
      break;
    case NAVRELATION_CONTROLLER_FOR:
      xpRelation.construct(RelationType::CONTROLLER_FOR);
      break;
    case NAVRELATION_LABEL_FOR:
      xpRelation.construct(RelationType::LABEL_FOR);
      break;
    case NAVRELATION_LABELLED_BY:
      xpRelation.construct(RelationType::LABELLED_BY);
      break;
    case NAVRELATION_MEMBER_OF:
      xpRelation.construct(RelationType::MEMBER_OF);
      break;
    case NAVRELATION_NODE_CHILD_OF:
      xpRelation.construct(RelationType::NODE_CHILD_OF);
      break;
    case NAVRELATION_FLOWS_TO:
      xpRelation.construct(RelationType::FLOWS_TO);
      break;
    case NAVRELATION_FLOWS_FROM:
      xpRelation.construct(RelationType::FLOWS_FROM);
      break;
    case NAVRELATION_SUBWINDOW_OF:
      xpRelation.construct(RelationType::SUBWINDOW_OF);
      break;
    case NAVRELATION_EMBEDS:
      xpRelation.construct(RelationType::EMBEDS);
      break;
    case NAVRELATION_EMBEDDED_BY:
      xpRelation.construct(RelationType::EMBEDDED_BY);
      break;
    case NAVRELATION_POPUP_FOR:
      xpRelation.construct(RelationType::POPUP_FOR);
      break;
    case NAVRELATION_PARENT_WINDOW_OF:
      xpRelation.construct(RelationType::PARENT_WINDOW_OF);
      break;
    case NAVRELATION_DEFAULT_BUTTON:
      xpRelation.construct(RelationType::DEFAULT_BUTTON);
      break;
    case NAVRELATION_DESCRIBED_BY:
      xpRelation.construct(RelationType::DESCRIBED_BY);
      break;
    case NAVRELATION_DESCRIPTION_FOR:
      xpRelation.construct(RelationType::DESCRIPTION_FOR);
      break;
    case NAVRELATION_NODE_PARENT_OF:
      xpRelation.construct(RelationType::NODE_PARENT_OF);
      break;

    default:
      return E_INVALIDARG;
  }

  pvarEndUpAt->vt = VT_EMPTY;

  if (!xpRelation.empty()) {
    Relation rel = RelationByType(xpRelation.ref());
    navAccessible = rel.Next();
  }

  if (!navAccessible)
    return E_FAIL;

  pvarEndUpAt->pdispVal = NativeAccessible(navAccessible);
  pvarEndUpAt->vt = VT_DISPATCH;
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::accHitTest(
       long xLeft,
       long yTop,
       VARIANT __RPC_FAR *pvarChild)
{
  A11Y_TRYBLOCK_BEGIN

  if (!pvarChild)
    return E_INVALIDARG;

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
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
AccessibleWrap::accDoDefaultAction(
       VARIANT varChild)
{
  A11Y_TRYBLOCK_BEGIN

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  Accessible* xpAccessible = GetXPAccessibleFor(varChild);
  if (!xpAccessible)
    return E_INVALIDARG;

  if (xpAccessible->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  return GetHRESULT(xpAccessible->DoAction(0));

  A11Y_TRYBLOCK_END
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
AccessibleWrap::GetTypeInfoCount(UINT *pctinfo)
{
  if (!pctinfo)
    return E_INVALIDARG;

  *pctinfo = 1;
  return S_OK;
}

STDMETHODIMP
AccessibleWrap::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
  if (!ppTInfo)
    return E_INVALIDARG;

  *ppTInfo = nullptr;

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

  
  
  
  if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Metro) {
    return NS_OK;
  }

  uint32_t eventType = aEvent->GetEventType();

  static_assert(sizeof(gWinEventMap)/sizeof(gWinEventMap[0]) == nsIAccessibleEvent::EVENT_LAST_ENTRY,
                "MSAA event map skewed");

  NS_ENSURE_TRUE(eventType > 0 && eventType < ArrayLength(gWinEventMap), NS_ERROR_FAILURE);

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
  nsAutoCString id;
  nsIContent* cnt = accessible->GetContent();
  if (cnt) {
    cnt->Tag()->ToString(tag);
    nsIAtom* aid = cnt->GetID();
    if (aid)
      aid->ToUTF8String(id);
  }

#ifdef A11Y_LOG
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
        nsViewManager* vm = shell->GetViewManager();
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

IDispatch*
AccessibleWrap::NativeAccessible(nsIAccessible* aAccessible)
{
  if (!aAccessible) {
   NS_WARNING("Not passing in an aAccessible");
   return nullptr;
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

    DocAccessible* document = Document();
    Accessible* child =
      document->GetAccessibleByUniqueIDInSubtree(uniqueID);

    
    if (IsDoc())
      return child;

    
    
    Accessible* parent = child;
    while (parent && parent != document) {
      if (parent == this)
        return child;

      parent = parent->Parent();
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

  nsIWidget* widget = nullptr;
  nsIntRect caretRect = SelectionMgr()->GetCaretRect(&widget);
  HWND caretWnd; 
  if (caretRect.IsEmpty() || !(caretWnd = (HWND)widget->GetNativeData(NS_NATIVE_WINDOW))) {
    return;
  }

  
  
  HBITMAP caretBitMap = CreateBitmap(1, caretRect.height, 1, 1, nullptr);
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

  ITypeLib *typeLib = nullptr;
  HRESULT hr = LoadRegTypeLib(LIBID_Accessibility, 1, 0, lcid, &typeLib);
  if (FAILED(hr))
    return nullptr;

  hr = typeLib->GetTypeInfoOfGuid(IID_IAccessible, &gTypeInfo);
  typeLib->Release();

  if (FAILED(hr))
    return nullptr;

  return gTypeInfo;
}
