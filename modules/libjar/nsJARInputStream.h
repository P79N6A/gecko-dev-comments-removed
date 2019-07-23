






































#ifndef nsJARINPUTSTREAM_h__
#define nsJARINPUTSTREAM_h__

#include "nsIInputStream.h"
#include "nsJAR.h"
#include "nsTArray.h"






class nsJARInputStream : public nsIInputStream
{
  public:
    nsJARInputStream() : 
        mOutSize(0), mInCrc(0), mOutCrc(0), mCurPos(0),
        mMode(MODE_NOTINITED)
    { 
      memset(&mZs, 0, sizeof(z_stream));
    }

    ~nsJARInputStream() { Close(); }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
   
    
    nsresult InitFile(nsJAR *aJar, nsZipItem *item);

    nsresult InitDirectory(nsJAR *aJar,
                           const nsACString& aJarDirSpec,
                           const char* aDir);
  
  private:
    nsRefPtr<nsZipHandle>  mFd;         
    PRUint32               mOutSize;    
    PRUint32               mInCrc;      
    PRUint32               mOutCrc;     
    z_stream               mZs;         

    
    nsRefPtr<nsJAR>        mJar;        
    PRUint32               mNameLen;    
    nsCString              mBuffer;     
    PRUint32               mCurPos;     
    PRUint32               mArrPos;     
    nsTArray<nsCString>    mArray;      

	typedef enum {
        MODE_NOTINITED,
        MODE_CLOSED,
        MODE_DIRECTORY,
        MODE_INFLATE,
        MODE_COPY
    } JISMode;

    JISMode                mMode;		

    nsresult ContinueInflate(char* aBuf, PRUint32 aCount, PRUint32* aBytesRead);
    nsresult ReadDirectory(char* aBuf, PRUint32 aCount, PRUint32* aBytesRead);
    PRUint32 CopyDataToBuffer(char* &aBuffer, PRUint32 &aCount);
};

#endif 

