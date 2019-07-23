




































#ifndef nsOfflineCacheDevice_h__
#define nsOfflineCacheDevice_h__

#include "nsCacheDevice.h"
#include "nsILocalFile.h"
#include "nsIObserver.h"
#include "mozIStorageConnection.h"
#include "nsCOMPtr.h"
#include "nsVoidArray.h"

class nsOfflineCacheDevice : public nsCacheDevice
{
public:
  nsOfflineCacheDevice();

  



  virtual ~nsOfflineCacheDevice();

  virtual nsresult        Init();
  virtual nsresult        Shutdown();

  virtual const char *    GetDeviceID(void);
  virtual nsCacheEntry *  FindEntry(nsCString * key, PRBool *collision);
  virtual nsresult        DeactivateEntry(nsCacheEntry * entry);
  virtual nsresult        BindEntry(nsCacheEntry * entry);
  virtual void            DoomEntry( nsCacheEntry * entry );

  virtual nsresult OpenInputStreamForEntry(nsCacheEntry *    entry,
                                           nsCacheAccessMode mode,
                                           PRUint32          offset,
                                           nsIInputStream ** result);

  virtual nsresult OpenOutputStreamForEntry(nsCacheEntry *     entry,
                                            nsCacheAccessMode  mode,
                                            PRUint32           offset,
                                            nsIOutputStream ** result);

  virtual nsresult        GetFileForEntry(nsCacheEntry *    entry,
                                          nsIFile **        result);

  virtual nsresult        OnDataSizeChange(nsCacheEntry * entry, PRInt32 deltaSize);
  
  virtual nsresult        Visit(nsICacheVisitor * visitor);

  virtual nsresult        EvictEntries(const char * clientID);


  
  nsresult                GetOwnerDomains(const char *        clientID,
                                          PRUint32 *          count,
                                          char ***            domains);
  nsresult                GetOwnerURIs(const char *           clientID,
                                       const nsACString &     ownerDomain,
                                       PRUint32 *             count,
                                       char ***               uris);
  nsresult                SetOwnedKeys(const char *           clientID,
                                       const nsACString &     ownerDomain,
                                       const nsACString &     ownerUrl,
                                       PRUint32               count,
                                       const char **          keys);
  nsresult                GetOwnedKeys(const char *           clientID,
                                       const nsACString &     ownerDomain,
                                       const nsACString &     ownerUrl,
                                       PRUint32 *             count,
                                       char ***               keys);
  nsresult                AddOwnedKey(const char *            clientID,
                                      const nsACString &      ownerDomain,
                                      const nsACString &      ownerURI,
                                      const nsACString &      key);
  nsresult                RemoveOwnedKey(const char *         clientID,
                                         const nsACString &   ownerDomain,
                                         const nsACString &   ownerURI,
                                         const nsACString &   key);
  nsresult                KeyIsOwned(const char *             clientID,
                                     const nsACString &       ownerDomain,
                                     const nsACString &       ownerURI,
                                     const nsACString &       key,
                                     PRBool *                 isOwned);

  nsresult                ClearKeysOwnedByDomain(const char *clientID,
                                                 const nsACString &ownerDomain);
  nsresult                EvictUnownedEntries(const char *clientID);

  nsresult                CreateTemporaryClientID(nsACString &clientID);
  nsresult                MergeTemporaryClientID(const char *clientID,
                                                 const char *fromClientID);


  



  void                    SetCacheParentDirectory(nsILocalFile * parentDir);
  void                    SetCapacity(PRUint32  capacity);

  nsILocalFile *          CacheDirectory() { return mCacheDirectory; }
  PRUint32                CacheCapacity() { return mCacheCapacity; }
  PRUint32                CacheSize();
  PRUint32                EntryCount();
  
private:
  PRBool   Initialized() { return mDB != nsnull; }
  nsresult UpdateEntry(nsCacheEntry *entry);
  nsresult UpdateEntrySize(nsCacheEntry *entry, PRUint32 newSize);
  nsresult DeleteEntry(nsCacheEntry *entry, PRBool deleteData);
  nsresult DeleteData(nsCacheEntry *entry);
  nsresult EnableEvictionObserver();
  nsresult DisableEvictionObserver();
  nsresult RunSimpleQuery(mozIStorageStatement *statment,
                          PRUint32 resultIndex,
                          PRUint32 * count,
                          char *** values);

  nsCOMPtr<mozIStorageConnection> mDB;
  nsCOMPtr<mozIStorageStatement>  mStatement_CacheSize;
  nsCOMPtr<mozIStorageStatement>  mStatement_EntryCount;
  nsCOMPtr<mozIStorageStatement>  mStatement_UpdateEntry;
  nsCOMPtr<mozIStorageStatement>  mStatement_UpdateEntrySize;
  nsCOMPtr<mozIStorageStatement>  mStatement_UpdateEntryFlags;
  nsCOMPtr<mozIStorageStatement>  mStatement_DeleteEntry;
  nsCOMPtr<mozIStorageStatement>  mStatement_FindEntry;
  nsCOMPtr<mozIStorageStatement>  mStatement_BindEntry;
  nsCOMPtr<mozIStorageStatement>  mStatement_ClearOwnership;
  nsCOMPtr<mozIStorageStatement>  mStatement_RemoveOwnership;
  nsCOMPtr<mozIStorageStatement>  mStatement_ClearDomain;
  nsCOMPtr<mozIStorageStatement>  mStatement_AddOwnership;
  nsCOMPtr<mozIStorageStatement>  mStatement_CheckOwnership;
  nsCOMPtr<mozIStorageStatement>  mStatement_DeleteUnowned;
  nsCOMPtr<mozIStorageStatement>  mStatement_ListOwned;
  nsCOMPtr<mozIStorageStatement>  mStatement_ListOwners;
  nsCOMPtr<mozIStorageStatement>  mStatement_ListOwnerDomains;
  nsCOMPtr<mozIStorageStatement>  mStatement_ListOwnerURIs;
  nsCOMPtr<mozIStorageStatement>  mStatement_SwapClientID;

  nsCOMPtr<nsILocalFile>          mCacheDirectory;
  PRUint32                        mCacheCapacity;
  PRInt32                         mDeltaCounter;
};

#endif 
