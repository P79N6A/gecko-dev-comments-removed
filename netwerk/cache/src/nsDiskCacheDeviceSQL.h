




































#ifndef nsDiskCacheDeviceSQL_h__
#define nsDiskCacheDeviceSQL_h__

#include "nsCacheDevice.h"
#include "nsILocalFile.h"
#include "nsIObserver.h"
#include "mozIStorageConnection.h"
#include "nsCOMPtr.h"

class nsDiskCacheDevice : public nsCacheDevice
{
public:
  nsDiskCacheDevice();

  


 
  virtual ~nsDiskCacheDevice();

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


  



  void                    SetCacheParentDirectory(nsILocalFile * parentDir);
  void                    SetCapacity(PRUint32  capacity);

  nsILocalFile *          CacheDirectory() { return mCacheDirectory; }
  PRUint32                CacheCapacity() { return mCacheCapacity; }
  PRUint32                CacheSize();
  PRUint32                EntryCount();
  

private:    
  PRBool   Initialized() { return mDB != nsnull; }
  nsresult EvictDiskCacheEntries(PRUint32 targetCapacity);
  nsresult UpdateEntry(nsCacheEntry *entry);
  nsresult UpdateEntrySize(nsCacheEntry *entry, PRUint32 newSize);
  nsresult DeleteEntry(nsCacheEntry *entry, PRBool deleteData);
  nsresult DeleteData(nsCacheEntry *entry);
  nsresult EnableEvictionObserver();
  nsresult DisableEvictionObserver();

#if 0
  
  static void EvictionObserver(struct sqlite3_context *, int, struct Mem **);
#endif

  nsCOMPtr<mozIStorageConnection> mDB;
  nsCOMPtr<mozIStorageStatement>  mStatement_CacheSize;
  nsCOMPtr<mozIStorageStatement>  mStatement_EntryCount;
  nsCOMPtr<mozIStorageStatement>  mStatement_UpdateEntry;
  nsCOMPtr<mozIStorageStatement>  mStatement_UpdateEntrySize;
  nsCOMPtr<mozIStorageStatement>  mStatement_DeleteEntry;
  nsCOMPtr<mozIStorageStatement>  mStatement_FindEntry;
  nsCOMPtr<mozIStorageStatement>  mStatement_BindEntry;

  nsCOMPtr<nsILocalFile>          mCacheDirectory;
  PRUint32                        mCacheCapacity;     
  PRInt32                         mDeltaCounter;
};

#endif 
