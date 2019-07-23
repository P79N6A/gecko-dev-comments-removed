







































#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranchInternal.h"
#include "nsIPrefLocalizedString.h"
#include "nsISecurityPref.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIRelativeFilePref.h"
#include "nsILocalFile.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsTArray.h"
#include "nsWeakReference.h"

class nsPrefBranch : public nsIPrefBranchInternal,
                     public nsISecurityPref,
                     public nsIObserver,
                     public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPREFBRANCH
  NS_DECL_NSIPREFBRANCH2
  NS_DECL_NSISECURITYPREF
  NS_DECL_NSIOBSERVER

  nsPrefBranch(const char *aPrefRoot, PRBool aDefaultBranch);
  virtual ~nsPrefBranch();

  PRInt32 GetRootLength() { return mPrefRootLength; }

protected:
  nsPrefBranch()	
    { }

  nsresult   GetDefaultFromPropertiesFile(const char *aPrefName, PRUnichar **return_buf);
  const char *getPrefName(const char *aPrefName);
  nsresult   getValidatedPrefName(const char *aPrefName, const char **_retval);
  void       freeObserverList(void);

private:
  PRInt32               mPrefRootLength;
  nsAutoVoidArray       *mObservers;
  nsCString             mPrefRoot;
  PRBool                mIsDefault;

};


class nsPrefLocalizedString : public nsIPrefLocalizedString,
                              public nsISupportsString
{
public:
  nsPrefLocalizedString();
  virtual ~nsPrefLocalizedString();

  NS_DECL_ISUPPORTS
  NS_FORWARD_NSISUPPORTSSTRING(mUnicodeString->)
  NS_FORWARD_NSISUPPORTSPRIMITIVE(mUnicodeString->)

  nsresult Init();

private:
  NS_IMETHOD GetData(PRUnichar**);
  NS_IMETHOD SetData(const PRUnichar* aData);
  NS_IMETHOD SetDataWithLength(PRUint32 aLength, const PRUnichar *aData);
                               
  nsCOMPtr<nsISupportsString> mUnicodeString;
};


class nsRelativeFilePref : public nsIRelativeFilePref
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRELATIVEFILEPREF
  
                nsRelativeFilePref();
  virtual       ~nsRelativeFilePref();
  
private:
  nsCOMPtr<nsILocalFile> mFile;
  nsCString mRelativeToKey;
};
