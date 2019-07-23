






































#ifndef nsJARINPUTSTREAM_h__
#define nsJARINPUTSTREAM_h__

#include "nsIInputStream.h"
#include "nsJAR.h"
#include "nsTArray.h"






class nsJARInputStream : public nsIInputStream
{
  public:
    nsJARInputStream() : 
        mCurPos(0), mCompressed(false), mDirectory(false), mClosed(true)
    { }

    ~nsJARInputStream() { Close(); }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
   
    
    nsresult InitFile(nsJAR *aJar, nsZipItem *item);

    nsresult InitDirectory(nsJAR *aJar,
                           const nsACString& aJarDirSpec,
                           const char* aDir);
  
  private:
    nsRefPtr<nsZipHandle>  mFd;         
    PRUint32               mCurPos;     
    PRUint32               mOutSize;    
    PRUint32               mInCrc;      
    PRUint32               mOutCrc;     
    z_stream               mZs;         

    
    nsRefPtr<nsJAR>        mJar;        
    PRUint32               mNameLen;    
    nsCString              mBuffer;     
    PRUint32               mArrPos;     
    nsTArray<nsCString>    mArray;      

    bool                   mCompressed; 
    bool                   mDirectory;  
    bool                   mClosed;     

    nsresult ContinueInflate(char* aBuf, PRUint32 aCount, PRUint32* aBytesRead);
    nsresult ReadDirectory(char* aBuf, PRUint32 aCount, PRUint32* aBytesRead);
    PRUint32 CopyDataToBuffer(char* &aBuffer, PRUint32 &aCount);
};

#endif 

