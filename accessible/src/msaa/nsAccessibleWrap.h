









































#ifndef _nsAccessibleWrap_H_
#define _nsAccessibleWrap_H_

#include "nsCOMPtr.h"
#include "nsAccessible.h"
#include "Accessible2.h"
#include "CAccessibleComponent.h"
#include "CAccessibleHyperlink.h"
#include "CAccessibleValue.h"

#define DECL_IUNKNOWN_INHERITED                                               \
public:                                                                       \
STDMETHODIMP QueryInterface(REFIID, void**);                                  \

#define IMPL_IUNKNOWN_QUERY_HEAD(Class)                                       \
STDMETHODIMP                                                                  \
Class::QueryInterface(REFIID iid, void** ppv)                                 \
{                                                                             \
  HRESULT hr = E_NOINTERFACE;                                                 \
  *ppv = NULL;                                                                \

#define IMPL_IUNKNOWN_QUERY_TAIL                                              \
  return hr;                                                                  \
}                                                                             \

#define IMPL_IUNKNOWN_QUERY_ENTRY(Class)                                      \
  hr = Class::QueryInterface(iid, ppv);                                       \
  if (SUCCEEDED(hr))                                                          \
    return hr;                                                                \

#define IMPL_IUNKNOWN_QUERY_ENTRY_COND(Class, Cond)                           \
  if (Cond) {                                                                 \
    hr = Class::QueryInterface(iid, ppv);                                     \
    if (SUCCEEDED(hr))                                                        \
      return hr;                                                              \
  }                                                                           \

#define IMPL_IUNKNOWN_INHERITED0(Class, Super)                                \
  IMPL_IUNKNOWN_QUERY_HEAD(Class)                                             \
  IMPL_IUNKNOWN_QUERY_ENTRY(Super)                                            \
  IMPL_IUNKNOWN_QUERY_TAIL                                                    \

#define IMPL_IUNKNOWN_INHERITED1(Class, Super, I1)                            \
  IMPL_IUNKNOWN_QUERY_HEAD(Class)                                             \
  IMPL_IUNKNOWN_QUERY_ENTRY(I1);                                              \
  IMPL_IUNKNOWN_QUERY_ENTRY(Super)                                            \
  IMPL_IUNKNOWN_QUERY_TAIL                                                    \

#define IMPL_IUNKNOWN_INHERITED2(Class, Super, I1, I2)                        \
  IMPL_IUNKNOWN_QUERY_HEAD(Class)                                             \
  IMPL_IUNKNOWN_QUERY_ENTRY(I1);                                              \
  IMPL_IUNKNOWN_QUERY_ENTRY(I2);                                              \
  IMPL_IUNKNOWN_QUERY_ENTRY(Super)                                            \
  IMPL_IUNKNOWN_QUERY_TAIL                                                    \


class nsAccessibleWrap : public nsAccessible,
                         public CAccessibleComponent,
                         public CAccessibleHyperlink,
                         public CAccessibleValue,
                         public IAccessible2,
                         public IEnumVARIANT
{
  public: 
    nsAccessibleWrap(nsIDOMNode*, nsIWeakReference *aShell);
    virtual ~nsAccessibleWrap();

    
    NS_DECL_ISUPPORTS_INHERITED

  public: 
    STDMETHODIMP QueryInterface(REFIID, void**);

  
    CLSID GetClassID() const;

  public: 
    virtual  HRESULT STDMETHODCALLTYPE get_accParent( 
         IDispatch __RPC_FAR *__RPC_FAR *ppdispParent);

    virtual  HRESULT STDMETHODCALLTYPE get_accChildCount( 
         long __RPC_FAR *pcountChildren);

    virtual  HRESULT STDMETHODCALLTYPE get_accChild( 
         VARIANT varChild,
         IDispatch __RPC_FAR *__RPC_FAR *ppdispChild);

    virtual  HRESULT STDMETHODCALLTYPE get_accName( 
         VARIANT varChild,
         BSTR __RPC_FAR *pszName);

    virtual  HRESULT STDMETHODCALLTYPE get_accValue( 
         VARIANT varChild,
         BSTR __RPC_FAR *pszValue);

    virtual  HRESULT STDMETHODCALLTYPE get_accDescription( 
         VARIANT varChild,
         BSTR __RPC_FAR *pszDescription);

    virtual  HRESULT STDMETHODCALLTYPE get_accRole( 
         VARIANT varChild,
         VARIANT __RPC_FAR *pvarRole);

    virtual  HRESULT STDMETHODCALLTYPE get_accState( 
         VARIANT varChild,
         VARIANT __RPC_FAR *pvarState);

    virtual  HRESULT STDMETHODCALLTYPE get_accHelp( 
         VARIANT varChild,
         BSTR __RPC_FAR *pszHelp);

    virtual  HRESULT STDMETHODCALLTYPE get_accHelpTopic( 
         BSTR __RPC_FAR *pszHelpFile,
         VARIANT varChild,
         long __RPC_FAR *pidTopic);

    virtual  HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut( 
         VARIANT varChild,
         BSTR __RPC_FAR *pszKeyboardShortcut);

    virtual  HRESULT STDMETHODCALLTYPE get_accFocus( 
         VARIANT __RPC_FAR *pvarChild);

    virtual  HRESULT STDMETHODCALLTYPE get_accSelection( 
         VARIANT __RPC_FAR *pvarChildren);

    virtual  HRESULT STDMETHODCALLTYPE get_accDefaultAction( 
         VARIANT varChild,
         BSTR __RPC_FAR *pszDefaultAction);

    virtual  HRESULT STDMETHODCALLTYPE accSelect( 
         long flagsSelect,
         VARIANT varChild);

    virtual  HRESULT STDMETHODCALLTYPE accLocation( 
         long __RPC_FAR *pxLeft,
         long __RPC_FAR *pyTop,
         long __RPC_FAR *pcxWidth,
         long __RPC_FAR *pcyHeight,
         VARIANT varChild);

    virtual  HRESULT STDMETHODCALLTYPE accNavigate( 
         long navDir,
         VARIANT varStart,
         VARIANT __RPC_FAR *pvarEndUpAt);

    virtual  HRESULT STDMETHODCALLTYPE accHitTest( 
         long xLeft,
         long yTop,
         VARIANT __RPC_FAR *pvarChild);

    virtual  HRESULT STDMETHODCALLTYPE accDoDefaultAction( 
         VARIANT varChild);

    virtual  HRESULT STDMETHODCALLTYPE put_accName( 
         VARIANT varChild,
         BSTR szName);

    virtual  HRESULT STDMETHODCALLTYPE put_accValue( 
         VARIANT varChild,
         BSTR szValue);

  public: 
    virtual  HRESULT STDMETHODCALLTYPE get_nRelations(
         long *nRelations);

    virtual  HRESULT STDMETHODCALLTYPE get_relation(
         long relationIndex,
         IAccessibleRelation **relation);

    virtual  HRESULT STDMETHODCALLTYPE get_relations(
         long maxRelations,
         IAccessibleRelation **relation,
         long *nRelations);

    virtual HRESULT STDMETHODCALLTYPE role(
             long *role);

    virtual HRESULT STDMETHODCALLTYPE scrollTo(
         enum IA2ScrollType scrollType);

    virtual HRESULT STDMETHODCALLTYPE scrollToPoint(
         enum IA2CoordinateType coordinateType,
	       long x,
	       long y);

    virtual  HRESULT STDMETHODCALLTYPE get_groupPosition(
         long *groupLevel,
         long *similarItemsInGroup,
         long *positionInGroup);

    virtual  HRESULT STDMETHODCALLTYPE get_states(
         AccessibleStates *states);

    virtual  HRESULT STDMETHODCALLTYPE get_extendedRole(
         BSTR *extendedRole);

    virtual  HRESULT STDMETHODCALLTYPE get_localizedExtendedRole(
         BSTR *localizedExtendedRole);

    virtual  HRESULT STDMETHODCALLTYPE get_nExtendedStates(
         long *nExtendedStates);

    virtual  HRESULT STDMETHODCALLTYPE get_extendedStates(
         long maxExtendedStates,
         BSTR **extendedStates,
         long *nExtendedStates);

    virtual  HRESULT STDMETHODCALLTYPE get_localizedExtendedStates(
         long maxLocalizedExtendedStates,
         BSTR **localizedExtendedStates,
         long *nLocalizedExtendedStates);

    virtual  HRESULT STDMETHODCALLTYPE get_uniqueID(
         long *uniqueID);

    virtual  HRESULT STDMETHODCALLTYPE get_windowHandle(
         HWND *windowHandle);

    virtual  HRESULT STDMETHODCALLTYPE get_indexInParent(
         long *indexInParent);

    virtual  HRESULT STDMETHODCALLTYPE get_locale(
         IA2Locale *locale);

    virtual  HRESULT STDMETHODCALLTYPE get_attributes(
         BSTR *attributes);

  public: 
    
    
    

    virtual  HRESULT STDMETHODCALLTYPE Next( 
         ULONG celt,
         VARIANT __RPC_FAR *rgVar,
         ULONG __RPC_FAR *pCeltFetched);
  
    virtual HRESULT STDMETHODCALLTYPE Skip( 
         ULONG celt);
  
    virtual HRESULT STDMETHODCALLTYPE Reset( void);
  
    virtual HRESULT STDMETHODCALLTYPE Clone( 
         IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum);

        
  

  STDMETHODIMP GetTypeInfoCount(UINT *p);
  STDMETHODIMP GetTypeInfo(UINT i, LCID lcid, ITypeInfo **ppti);
  STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                               UINT cNames, LCID lcid, DISPID *rgDispId);
  STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

  
  virtual nsresult FireAccessibleEvent(nsIAccessibleEvent *aEvent);

  
  static PRInt32 GetChildIDFor(nsIAccessible* aAccessible);
  static HWND GetHWNDFor(nsIAccessible *aAccessible);
  static HRESULT ConvertToIA2Attributes(nsIPersistentProperties *aAttributes,
                                        BSTR *aIA2Attributes);

  






  void UpdateSystemCaret();

  virtual void GetXPAccessibleFor(const VARIANT& aVarChild, nsIAccessible **aXPAccessible);
  NS_IMETHOD GetNativeInterface(void **aOutAccessible);

  
  
  static STDMETHODIMP AccessibleObjectFromWindow(HWND hwnd,DWORD dwObjectID,REFIID riid,void **ppvObject);
  static STDMETHODIMP NotifyWinEvent(DWORD event,HWND hwnd,LONG idObjectType,LONG idObject);

  static IDispatch *NativeAccessible(nsIAccessible *aXPAccessible);

  




  void UnattachIEnumVariant();

protected:
  virtual nsresult FirePlatformEvent(nsIAccessibleEvent *aEvent);

  
  
  
  PRInt32 mEnumVARIANTPosition;

  enum navRelations {
    NAVRELATION_CONTROLLED_BY = 0x1000,
    NAVRELATION_CONTROLLER_FOR = 0x1001,
    NAVRELATION_LABEL_FOR = 0x1002,
    NAVRELATION_LABELLED_BY = 0x1003,
    NAVRELATION_MEMBER_OF = 0x1004,
    NAVRELATION_NODE_CHILD_OF = 0x1005,
    NAVRELATION_FLOWS_TO = 0x1006,
    NAVRELATION_FLOWS_FROM = 0x1007,
    NAVRELATION_SUBWINDOW_OF = 0x1008,
    NAVRELATION_EMBEDS = 0x1009,
    NAVRELATION_EMBEDDED_BY = 0x100a,
    NAVRELATION_POPUP_FOR = 0x100b,
    NAVRELATION_PARENT_WINDOW_OF = 0x100c,
    NAVRELATION_DEFAULT_BUTTON = 0x100d,
    NAVRELATION_DESCRIBED_BY = 0x100e,
    NAVRELATION_DESCRIPTION_FOR = 0x100f
  };
};


typedef class nsHTMLTextFieldAccessible    nsHTMLTextFieldAccessibleWrap;
typedef class nsXULTextFieldAccessible     nsXULTextFieldAccessibleWrap;

#endif
