





































#ifndef nsIDNService_h__
#define nsIDNService_h__

#include "nsIIDNService.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIUnicodeNormalizer.h"
#include "nsIDNKitInterface.h"
#include "nsString.h"

class nsIPrefBranch;





#define kACEPrefixLen 4 

class nsIDNService : public nsIIDNService,
                     public nsIObserver,
                     public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDNSERVICE
  NS_DECL_NSIOBSERVER

  nsIDNService();
  virtual ~nsIDNService();

  nsresult Init();

private:
  void normalizeFullStops(nsAString& s);
  nsresult stringPrepAndACE(const nsAString& in, nsACString& out,
                            PRBool allowUnassigned);
  nsresult encodeToACE(const nsAString& in, nsACString& out);
  nsresult stringPrep(const nsAString& in, nsAString& out,
                      PRBool allowUnassigned);
  nsresult decodeACE(const nsACString& in, nsACString& out,
                     PRBool allowUnassigned);
  nsresult UTF8toACE(const nsACString& in, nsACString& out,
                     PRBool allowUnassigned);
  nsresult ACEtoUTF8(const nsACString& in, nsACString& out,
                     PRBool allowUnassigned);
  PRBool isInWhitelist(const nsACString &host);
  void prefsChanged(nsIPrefBranch *prefBranch, const PRUnichar *pref);

  PRBool mMultilingualTestBed;  
  idn_nameprep_t mNamePrepHandle;
  nsCOMPtr<nsIUnicodeNormalizer> mNormalizer;
  char mACEPrefix[kACEPrefixLen+1];
  nsXPIDLString mIDNBlacklist;
  PRBool mShowPunycode;
  nsCOMPtr<nsIPrefBranch> mIDNWhitelistPrefBranch;
};

#endif  
