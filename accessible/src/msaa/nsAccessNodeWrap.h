









































#ifndef _nsAccessNodeWrap_H_
#define _nsAccessNodeWrap_H_






#pragma warning( disable : 4509 )

#include "nsCOMPtr.h"
#include "nsIAccessible.h"
#include "nsIAccessibleEvent.h"
#include "nsIWinAccessNode.h"
#include "ISimpleDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIContent.h"
#include "nsAccessNode.h"
#include "OLEIDL.H"
#include "OLEACC.H"
#include <winuser.h>
#ifndef WINABLEAPI
#include <winable.h>
#endif
#ifdef MOZ_CRASHREPORTER
#include "nsICrashReporter.h"
#endif

typedef LRESULT (STDAPICALLTYPE *LPFNNOTIFYWINEVENT)(DWORD event,HWND hwnd,LONG idObjectType,LONG idObject);
typedef LRESULT (STDAPICALLTYPE *LPFNGETGUITHREADINFO)(DWORD idThread, GUITHREADINFO* pgui);

class nsAccessNodeWrap :  public nsAccessNode,
                          public nsIWinAccessNode,
                          public ISimpleDOMNode,
                          public IServiceProvider
{
  public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWINACCESSNODE

  public: 
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void** ppv);

  public: 
    nsAccessNodeWrap(nsIDOMNode *, nsIWeakReference* aShell);
    virtual ~nsAccessNodeWrap();

    
    STDMETHODIMP QueryInterface(REFIID, void**);

  public:

    virtual  HRESULT STDMETHODCALLTYPE get_nodeInfo( 
         BSTR __RPC_FAR *tagName,
         short __RPC_FAR *nameSpaceID,
         BSTR __RPC_FAR *nodeValue,
         unsigned int __RPC_FAR *numChildren,
         unsigned int __RPC_FAR *aUniqueID,
         unsigned short __RPC_FAR *nodeType);
  
    virtual  HRESULT STDMETHODCALLTYPE get_attributes( 
         unsigned short maxAttribs,
         BSTR __RPC_FAR *attribNames,
         short __RPC_FAR *nameSpaceID,
         BSTR __RPC_FAR *attribValues,
         unsigned short __RPC_FAR *numAttribs);
  
    virtual  HRESULT STDMETHODCALLTYPE get_attributesForNames( 
         unsigned short maxAttribs,
         BSTR __RPC_FAR *attribNames,
         short __RPC_FAR *nameSpaceID,
         BSTR __RPC_FAR *attribValues);
  
    virtual  HRESULT STDMETHODCALLTYPE get_computedStyle( 
         unsigned short maxStyleProperties,
         boolean useAlternateView,
         BSTR __RPC_FAR *styleProperties,
         BSTR __RPC_FAR *styleValues,
         unsigned short __RPC_FAR *numStyleProperties);
  
    virtual  HRESULT STDMETHODCALLTYPE get_computedStyleForProperties( 
         unsigned short numStyleProperties,
         boolean useAlternateView,
         BSTR __RPC_FAR *styleProperties,
         BSTR __RPC_FAR *styleValues);
        
    virtual HRESULT STDMETHODCALLTYPE scrollTo( boolean scrollTopLeft);

    virtual  HRESULT STDMETHODCALLTYPE get_parentNode(ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
    virtual  HRESULT STDMETHODCALLTYPE get_firstChild(ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
    virtual  HRESULT STDMETHODCALLTYPE get_lastChild(ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
    virtual  HRESULT STDMETHODCALLTYPE get_previousSibling(ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
    virtual  HRESULT STDMETHODCALLTYPE get_nextSibling(ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);
    virtual  HRESULT STDMETHODCALLTYPE get_childAt(unsigned childIndex,
                                                                  ISimpleDOMNode __RPC_FAR *__RPC_FAR *node);

    virtual  HRESULT STDMETHODCALLTYPE get_innerHTML(
         BSTR __RPC_FAR *innerHTML);

    virtual  HRESULT STDMETHODCALLTYPE get_localInterface( 
         void __RPC_FAR *__RPC_FAR *localInterface);
        
    virtual  HRESULT STDMETHODCALLTYPE get_language(
         BSTR __RPC_FAR *language);

    static void InitAccessibility();
    static void ShutdownAccessibility();

    
    static HINSTANCE gmAccLib;
    static HINSTANCE gmUserLib;
    static LPFNACCESSIBLEOBJECTFROMWINDOW gmAccessibleObjectFromWindow;
    static LPFNNOTIFYWINEVENT gmNotifyWinEvent;
    static LPFNGETGUITHREADINFO gmGetGUIThreadInfo;

    static int FilterA11yExceptions(unsigned int aCode, EXCEPTION_POINTERS *aExceptionInfo);

    static PRBool IsOnlyMsaaCompatibleJawsPresent();

    static void TurnOffNewTabSwitchingForJawsAndWE();

    static void DoATSpecificProcessing();
  protected:
    void GetAccessibleFor(nsIDOMNode *node, nsIAccessible **newAcc);
    ISimpleDOMNode* MakeAccessNode(nsIDOMNode *node);

    


    static HRESULT QueryISimpleDOMNode(nsIAccessNode *aAccessNode,
                                       ISimpleDOMNode __RPC_FAR *__RPC_FAR *aNode);

    static PRBool gIsEnumVariantSupportDisabled;

    



     static PRBool gIsIA2Disabled;

    



    static nsIAccessibleTextChangeEvent *gTextEvent;
};




HRESULT GetHRESULT(nsresult aResult);

#endif

