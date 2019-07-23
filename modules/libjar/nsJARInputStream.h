






































#ifndef nsJARINPUTSTREAM_h__
#define nsJARINPUTSTREAM_h__

#include "nsIInputStream.h"
#include "nsJAR.h"
#include "nsTArray.h"






class nsJARInputStream : public nsIInputStream
{
  public:
    nsJARInputStream() : 
        mInSize(0), mCurPos(0), mInflate(nsnull), mDirectory(0), mClosed(PR_FALSE)
  { }

  ~nsJARInputStream() {
    Close();
  }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
   
    
    nsresult InitFile(nsZipHandle *aFd, nsZipItem *item);

    nsresult InitDirectory(nsJAR *aJar,
                           const nsACString& aJarDirSpec,
                           const char* aDir);
  
  private:
    PRUint32      mInSize;          
    PRUint32      mCurPos;          

    struct InflateStruct {
        PRUint32      mOutSize;     
        PRUint32      mInCrc;       
        PRUint32      mOutCrc;      
        z_stream      mZs;          
        unsigned char mReadBuf[ZIP_BUFLEN]; 
    };
    struct InflateStruct *   mInflate;

    
    nsRefPtr<nsJAR>         mJar;     
    PRUint32                mNameLen; 
    nsCAutoString           mBuffer;  
    PRUint32                mArrPos;  
    nsTArray<nsCString>     mArray;   
  PRPackedBool            mDirectory; 
    PRPackedBool            mClosed;  
    nsSeekableZipHandle     mFd;      

    nsresult ContinueInflate(char* aBuf, PRUint32 aCount, PRUint32* aBytesRead);
    nsresult ReadDirectory(char* aBuf, PRUint32 aCount, PRUint32* aBytesRead);
    PRUint32 CopyDataToBuffer(char* &aBuffer, PRUint32 &aCount);
};

#endif 

