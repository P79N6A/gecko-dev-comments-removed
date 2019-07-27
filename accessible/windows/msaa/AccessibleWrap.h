





#ifndef mozilla_a11y_AccessibleWrap_h_
#define mozilla_a11y_AccessibleWrap_h_

#include "nsCOMPtr.h"
#include "Accessible.h"
#include "Accessible2.h"
#include "ia2Accessible.h"
#include "ia2AccessibleComponent.h"
#include "ia2AccessibleHyperlink.h"
#include "ia2AccessibleValue.h"

#ifdef __GNUC__



#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

namespace mozilla {
namespace a11y {

class AccessibleWrap : public Accessible,
                       public ia2Accessible,
                       public ia2AccessibleComponent,
                       public ia2AccessibleHyperlink,
                       public ia2AccessibleValue
{
public: 
  AccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    Accessible(aContent, aDoc) { }

  
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

  
  virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);

  virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid,
                                                ITypeInfo **ppTInfo);

  virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid,
                                                  LPOLESTR *rgszNames,
                                                  UINT cNames,
                                                  LCID lcid,
                                                  DISPID *rgDispId);

  virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid,
                                           LCID lcid, WORD wFlags,
                                           DISPPARAMS *pDispParams,
                                           VARIANT *pVarResult,
                                           EXCEPINFO *pExcepInfo,
                                           UINT *puArgErr);

  
  virtual nsresult HandleAccEvent(AccEvent* aEvent);

  
  static int32_t GetChildIDFor(Accessible* aAccessible);
  static HWND GetHWNDFor(Accessible* aAccessible);

  






  void UpdateSystemCaretFor(Accessible* aAccessible);

  


  Accessible* GetXPAccessibleFor(const VARIANT& aVarChild);

  NS_IMETHOD GetNativeInterface(void **aOutAccessible);

  static IDispatch *NativeAccessible(nsIAccessible *aXPAccessible);

protected:
  virtual ~AccessibleWrap() { }

  


  static ITypeInfo* GetTI(LCID lcid);

  static ITypeInfo* gTypeInfo;


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
    NAVRELATION_DESCRIPTION_FOR = 0x100f,
    NAVRELATION_NODE_PARENT_OF = 0x1010,
    NAVRELATION_CONTAINING_DOCUMENT = 0x1011,
    NAVRELATION_CONTAINING_TAB_PANE = 0x1012,
    NAVRELATION_CONTAINING_APPLICATION = 0x1014
  };
};

} 
} 

#ifdef XP_WIN

#undef GetMessage
#undef CreateEvent
#undef GetClassName
#undef GetBinaryType
#undef RemoveDirectory
#endif

#endif
