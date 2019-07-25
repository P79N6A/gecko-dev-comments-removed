





#ifndef mozilla_dom_indexeddb_filestream_h__
#define mozilla_dom_indexeddb_filestream_h__

#include "IndexedDatabase.h"

#include "nsIFileStreams.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsISeekableStream.h"
#include "nsIStandardFileStream.h"

class nsIFile;
struct quota_FILE;

BEGIN_INDEXEDDB_NAMESPACE

class FileStream : public nsISeekableStream,
                   public nsIInputStream,
                   public nsIOutputStream,
                   public nsIStandardFileStream,
                   public nsIFileMetadata
{
public:
  FileStream()
  : mFlags(0),
    mDeferredOpen(false),
    mQuotaFile(nullptr)
  { }

  virtual ~FileStream()
  {
    Close();
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSISEEKABLESTREAM
  NS_DECL_NSISTANDARDFILESTREAM
  NS_DECL_NSIFILEMETADATA

  
  NS_IMETHOD
  Close();

  NS_IMETHOD
  Available(PRUint64* _retval);

  NS_IMETHOD
  Read(char* aBuf, PRUint32 aCount, PRUint32* _retval);

  NS_IMETHOD
  ReadSegments(nsWriteSegmentFun aWriter, void* aClosure, PRUint32 aCount,
               PRUint32* _retval);

  NS_IMETHOD
  IsNonBlocking(bool* _retval);

  

  

  NS_IMETHOD
  Flush();

  NS_IMETHOD
  Write(const char* aBuf, PRUint32 aCount, PRUint32* _retval);

  NS_IMETHOD
  WriteFrom(nsIInputStream* aFromStream, PRUint32 aCount, PRUint32* _retval);

  NS_IMETHOD
  WriteSegments(nsReadSegmentFun aReader, void* aClosure, PRUint32 aCount,
                PRUint32* _retval);

  

protected:
  


  void
  CleanUpOpen()
  {
    mFilePath.Truncate();
    mDeferredOpen = false;
  }

  





  virtual nsresult
  DoOpen();

  



  nsresult
  DoPendingOpen()
  {
    if (!mDeferredOpen) {
      return NS_OK;
    }

    return DoOpen();
  }

  


  nsString mFilePath;
  nsString mMode;

  


  PRInt32 mFlags;

  


  bool mDeferredOpen;

  


  quota_FILE* mQuotaFile;
};

END_INDEXEDDB_NAMESPACE

#endif 
