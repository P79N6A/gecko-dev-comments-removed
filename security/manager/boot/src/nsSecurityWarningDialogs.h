






































#ifndef nsSecurityWarningDialogs_h
#define nsSecurityWarningDialogs_h

#include "nsISecurityWarningDialogs.h"
#include "nsIPrefBranch.h"
#include "nsIStringBundle.h"
#include "nsCOMPtr.h"

class nsSecurityWarningDialogs : public nsISecurityWarningDialogs
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISECURITYWARNINGDIALOGS

  nsSecurityWarningDialogs();
  virtual ~nsSecurityWarningDialogs();

  nsresult Init();

protected:
  nsresult AlertDialog(nsIInterfaceRequestor *ctx, const char *prefName,
                   const PRUnichar *messageName,
                   const PRUnichar *showAgainName);
  nsresult ConfirmDialog(nsIInterfaceRequestor *ctx, const char *prefName,
                   const PRUnichar *messageName, 
                   const PRUnichar *showAgainName, PRBool* _result);
  nsCOMPtr<nsIStringBundle> mStringBundle;
  nsCOMPtr<nsIPrefBranch> mPrefBranch;
};

#define NS_SECURITYWARNINGDIALOGS_CID \
 { /* 8d995d4f-adcc-4159-b7f1-e94af72eeb88 */       \
  0x8d995d4f, 0xadcc, 0x4159,                       \
 {0xb7, 0xf1, 0xe9, 0x4a, 0xf7, 0x2e, 0xeb, 0x88} }

#endif
