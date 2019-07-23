




































#ifndef nsPermissionManager_h__
#define nsPermissionManager_h__

#include "nsIPermissionManager.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsTHashtable.h"
#include "nsString.h"
#include "nsITimer.h"

class nsIPermission;












#define NUMBER_OF_TYPES       (8)
#define NUMBER_OF_PERMISSIONS (16)

class nsHostEntry : public PLDHashEntryHdr
{
public:
  
  typedef const char* KeyType;
  typedef const char* KeyTypePointer;

  nsHostEntry(const char* aHost);
  nsHostEntry(const nsHostEntry& toCopy);

  ~nsHostEntry()
  {
  }

  KeyType GetKey() const
  {
    return mHost;
  }

  PRBool KeyEquals(KeyTypePointer aKey) const
  {
    return !strcmp(mHost, aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return aKey;
  }

  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    
    
    return PL_DHashStringKey(nsnull, aKey);
  }

  enum { ALLOW_MEMMOVE = PR_TRUE };

  
  inline const nsDependentCString GetHost() const
  {
    return nsDependentCString(mHost);
  }

  
  void SetPermission(PRInt32 aTypeIndex, PRUint32 aPermission)
  {
    mPermissions[aTypeIndex] = (PRUint8)aPermission;
  }

  PRUint32 GetPermission(PRInt32 aTypeIndex) const
  {
    return (PRUint32)mPermissions[aTypeIndex];
  }

  PRBool PermissionsAreEmpty() const
  {
    
    return (*reinterpret_cast<const PRUint32*>(&mPermissions[0])==0 && 
            *reinterpret_cast<const PRUint32*>(&mPermissions[4])==0 );
  }

private:
  const char *mHost;

  
  
  
  
  
  PRUint8 mPermissions[NUMBER_OF_TYPES];
};


class nsPermissionManager : public nsIPermissionManager,
                            public nsIObserver,
                            public nsSupportsWeakReference
{
public:

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPERMISSIONMANAGER
  NS_DECL_NSIOBSERVER

  nsPermissionManager();
  virtual ~nsPermissionManager();
  nsresult Init();

private:

  nsresult AddInternal(const nsAFlatCString &aHost,
                       PRInt32  aTypeIndex,
                       PRUint32 aPermission,
                       PRBool   aNotify);
  PRInt32 GetTypeIndex(const char *aTypeString,
                       PRBool      aAdd);

  nsHostEntry *GetHostEntry(const nsAFlatCString &aHost,
                            PRUint32              aType,
                            PRBool                aExactHostMatch);

  nsresult CommonTestPermission(nsIURI     *aURI,
                                const char *aType,
                                PRUint32   *aPermission,
                                PRBool      aExactHostMatch);

  
  
  void        LazyWrite();
  static void DoLazyWrite(nsITimer *aTimer, void *aClosure);
  nsresult    Write();

  nsresult Read();
  void     NotifyObserversWithPermission(const nsACString &aHost,
                                         const char       *aType,
                                         PRUint32          aPermission,
                                         const PRUnichar  *aData);
  void     NotifyObservers(nsIPermission *aPermission, const PRUnichar *aData);
  nsresult RemoveAllFromMemory();
  nsresult GetHost(nsIURI *aURI, nsACString &aResult);
  void     RemoveTypeStrings();

  nsCOMPtr<nsIObserverService> mObserverService;
  nsCOMPtr<nsIFile>            mPermissionsFile;
  nsCOMPtr<nsITimer>           mWriteTimer;
  nsTHashtable<nsHostEntry>    mHostTable;
  PRUint32                     mHostCount;
  PRPackedBool                 mChangedList;
  PRPackedBool                 mHasUnknownTypes;

  
  char                        *mTypeArray[NUMBER_OF_TYPES];
};


#define NS_PERMISSIONMANAGER_CID \
{ 0x4f6b5e00, 0xc36, 0x11d5, { 0xa5, 0x35, 0x0, 0x10, 0xa4, 0x1, 0xeb, 0x10 } }

#endif 
