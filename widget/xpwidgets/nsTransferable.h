




#ifndef nsTransferable_h__
#define nsTransferable_h__

#include "nsIFormatConverter.h"
#include "nsITransferable.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"

class nsString;
class nsDataObj;






struct DataStruct
{
  DataStruct ( const char* aFlavor )
    : mDataLen(0), mFlavor(aFlavor), mCacheFileName(nullptr) { }
  ~DataStruct();
  
  const nsCString& GetFlavor() const { return mFlavor; }
  void SetData( nsISupports* inData, uint32_t inDataLen );
  void GetData( nsISupports** outData, uint32_t *outDataLen );
  already_AddRefed<nsIFile> GetFileSpec(const char* aFileName);
  bool IsDataAvailable() const { return (mData && mDataLen > 0) || (!mData && mCacheFileName); }
  
protected:

  enum {
    
    
    kLargeDatasetSize = 1000000        
  };
  
  nsresult WriteCache(nsISupports* aData, uint32_t aDataLen );
  nsresult ReadCache(nsISupports** aData, uint32_t* aDataLen );
  
  nsCOMPtr<nsISupports> mData;   
  uint32_t mDataLen;
  const nsCString mFlavor;
  char *   mCacheFileName;

};





class nsTransferable : public nsITransferable
{
public:

  nsTransferable();
  virtual ~nsTransferable();

    
  NS_DECL_ISUPPORTS
  NS_DECL_NSITRANSFERABLE

protected:

    
  nsresult GetTransferDataFlavors(nsISupportsArray** aDataFlavorList);
 
  nsTArray<DataStruct> mDataArray;
  nsCOMPtr<nsIFormatConverter> mFormatConv;
  bool mPrivateData;
#if DEBUG
  bool mInitialized;
#endif

};

#endif 
