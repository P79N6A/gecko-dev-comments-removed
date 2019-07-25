





#ifndef nsJARINPUTSTREAM_h__
#define nsJARINPUTSTREAM_h__

#include "nsIInputStream.h"
#include "nsJAR.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"






class nsJARInputStream MOZ_FINAL : public nsIInputStream
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
    uint32_t               mOutSize;    
    uint32_t               mInCrc;      
    uint32_t               mOutCrc;     
    z_stream               mZs;         

    
    nsRefPtr<nsJAR>        mJar;        
    uint32_t               mNameLen;    
    nsCString              mBuffer;     
    uint32_t               mCurPos;     
    uint32_t               mArrPos;     
    nsTArray<nsCString>    mArray;      

	typedef enum {
        MODE_NOTINITED,
        MODE_CLOSED,
        MODE_DIRECTORY,
        MODE_INFLATE,
        MODE_COPY
    } JISMode;

    JISMode                mMode;		

    nsresult ContinueInflate(char* aBuf, uint32_t aCount, uint32_t* aBytesRead);
    nsresult ReadDirectory(char* aBuf, uint32_t aCount, uint32_t* aBytesRead);
    uint32_t CopyDataToBuffer(char* &aBuffer, uint32_t &aCount);
};

#endif 

