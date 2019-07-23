




































#include "prtypes.h"
#include "pldhash.h"
#include "nsCOMPtr.h"
#include "nsHashtable.h"
#include "nsIFastLoadService.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"

class nsFastLoadFileReader;
class nsFastLoadFileWriter;

class nsFastLoadService : public nsIFastLoadService
{
  public:
    nsFastLoadService();
  private:
    ~nsFastLoadService();

  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIFASTLOADSERVICE

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  private:
    PRLock*                         mLock;
    PLDHashTable*                   mFastLoadPtrMap;
    nsCOMPtr<nsIObjectInputStream>  mInputStream;
    nsCOMPtr<nsIObjectOutputStream> mOutputStream;
    nsCOMPtr<nsIFastLoadFileIO>     mFileIO;
    PRInt32                         mDirection;
    nsHashtable                     mChecksumTable;
};
