








#ifndef _nsAccessNodeWrap_H_
#define _nsAccessNodeWrap_H_






#ifdef _MSC_VER
#pragma warning( disable : 4509 )
#endif

#include "nsCOMPtr.h"
#include "nsIAccessible.h"
#include "nsIAccessibleEvent.h"
#include "nsIWinAccessNode.h"
#include "nsIDOMElement.h"
#include "nsIContent.h"
#include "nsAccessNode.h"
#include "oleidl.h"
#include "oleacc.h"
#include <winuser.h>
#ifdef MOZ_CRASHREPORTER
#include "nsICrashReporter.h"
#endif

#include "nsRefPtrHashtable.h"

#define A11Y_TRYBLOCK_BEGIN                                                    \
  MOZ_SEH_TRY {

#define A11Y_TRYBLOCK_END                                                             \
  } MOZ_SEH_EXCEPT(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(),       \
                                                          GetExceptionInformation())) \
  { }                                                                                 \
  return E_FAIL;

namespace mozilla {
namespace a11y {

class AccTextChangeEvent;

#ifdef __GNUC__



#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

class nsAccessNodeWrap : public nsAccessNode,
                         public nsIWinAccessNode,
                         public IServiceProvider
{
  public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWINACCESSNODE

public: 
  nsAccessNodeWrap(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~nsAccessNodeWrap();

  
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID aIID,
                                                   void** aInstancePtr);

  
  virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID aGuidService,
                                                 REFIID aIID,
                                                 void** aInstancePtr);

    static int FilterA11yExceptions(unsigned int aCode, EXCEPTION_POINTERS *aExceptionInfo);

  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT Msg,
                                     WPARAM WParam, LPARAM lParam);

  static nsRefPtrHashtable<nsPtrHashKey<void>, DocAccessible> sHWNDCache;

protected:

  



  static AccTextChangeEvent* gTextEvent;
  friend void PlatformShutdown();
};

} 
} 




HRESULT GetHRESULT(nsresult aResult);

#endif

