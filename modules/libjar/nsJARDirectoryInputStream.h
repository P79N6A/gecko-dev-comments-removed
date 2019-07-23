





































#ifndef nsJARDIRECTORYINPUTSTREAM_h__
#define nsJARDIRECTORYINPUTSTREAM_h__

#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsIInputStream.h"
#include "nsJAR.h"






class nsJARDirectoryInputStream : public nsIInputStream
{
  public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM

    static nsresult Create(nsIZipReader* aZip,
                           const nsACString& aJarDirSpec,
                           const char* aDir,
                           nsIInputStream** result);
  private:
    nsJARDirectoryInputStream();
    virtual ~nsJARDirectoryInputStream();

    nsresult 
    Init(nsIZipReader* aZip, const nsACString& aJarDirSpec, const char* aDir);

    PRUint32 CopyDataToBuffer(char* &aBuffer, PRUint32 &aCount);

  protected:
    nsIZipReader*           mZip;        
    nsresult                mStatus;     
    PRUint32                mDirNameLen; 
    nsCAutoString           mBuffer;     
    PRUint32                mArrPos;     
    PRUint32                mBufPos;     
    nsCStringArray          mArray;      
};

#endif 
