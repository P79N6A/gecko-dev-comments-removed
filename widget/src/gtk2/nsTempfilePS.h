







































#ifndef nsTempfilePS_h__
#define nsTempfilePS_h__

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsString.h"

class nsILocalFile;

class nsTempfilePS
{
public:
    nsTempfilePS();
    ~nsTempfilePS();

    









    nsresult CreateTempFile(nsILocalFile** aFile);

    













    nsresult CreateTempFile(nsILocalFile** aFile,
            FILE **aHandle, const char *aMode);

private:
    nsCOMPtr<nsIFile> mTempDir;
    PRUint32 mCount;
};

#endif	
