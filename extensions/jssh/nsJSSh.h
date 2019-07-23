





































#ifndef __NS_JSSH_H__
#define __NS_JSSH_H__

#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsIRunnable.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIJSSh.h"
#include "nsIJSContextStack.h"
#include "nsIPrincipal.h"
#include "nsStringAPI.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIXPCScriptable.h"

class nsJSSh : public nsIRunnable, public nsIJSSh,
               public nsIScriptObjectPrincipal,
               public nsIXPCScriptable
{
public:
  nsJSSh(nsIInputStream* input, nsIOutputStream*output,
         const nsACString &startupURI);
  ~nsJSSh();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIJSSH
  
  virtual nsIPrincipal *GetPrincipal();
  NS_DECL_NSIXPCSCRIPTABLE
  
public:
  PRBool LoadURL(const char *url, jsval* retval=nsnull);
  
  nsCOMPtr<nsIInputStream> mInput;
  nsCOMPtr<nsIOutputStream> mOutput;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIJSContextStack> mContextStack;
  PRInt32 mSuspendCount; 
  
  enum { cBufferSize=100*1024 };
  char mBuffer[cBufferSize];
  int mBufferPtr;
  JSContext* mJSContext;
  JSObject *mGlobal;
  JSObject *mContextObj;
  PRBool mQuit;
  nsCString mStartupURI;
  nsCString mPrompt;
  PRBool mEmitHeader;
  nsCString mProtocol;
};

already_AddRefed<nsIRunnable>
CreateJSSh(nsIInputStream* input, nsIOutputStream*output,
           const nsACString &startupURI);

#endif 
