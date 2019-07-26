





#ifndef nsSecurityWarningDialogs_h
#define nsSecurityWarningDialogs_h

#include "nsISecurityWarningDialogs.h"
#include "nsIPrefBranch.h"
#include "nsIStringBundle.h"
#include "nsCOMPtr.h"

class nsSecurityWarningDialogs : public nsISecurityWarningDialogs
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISECURITYWARNINGDIALOGS

  nsSecurityWarningDialogs();

  nsresult Init();

protected:
  virtual ~nsSecurityWarningDialogs();

  nsresult AlertDialog(nsIInterfaceRequestor *ctx, const char *prefName,
                   const char16_t *messageName,
                   const char16_t *showAgainName,
                   bool aAsync, const uint32_t aBucket);
  nsresult ConfirmDialog(nsIInterfaceRequestor *ctx, const char *prefName,
                   const char16_t *messageName, 
                   const char16_t *showAgainName, const uint32_t aBucket,
                   bool* _result);
  nsCOMPtr<nsIStringBundle> mStringBundle;
  nsCOMPtr<nsIPrefBranch> mPrefBranch;
};

#define NS_SECURITYWARNINGDIALOGS_CID \
 { /* 8d995d4f-adcc-4159-b7f1-e94af72eeb88 */       \
  0x8d995d4f, 0xadcc, 0x4159,                       \
 {0xb7, 0xf1, 0xe9, 0x4a, 0xf7, 0x2e, 0xeb, 0x88} }

#endif
