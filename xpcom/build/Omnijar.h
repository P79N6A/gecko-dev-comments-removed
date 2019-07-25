






































#ifndef mozilla_Omnijar_h
#define mozilla_Omnijar_h

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsIFile;
class nsZipArchive;
class nsIURI;

namespace mozilla {

class Omnijar {
private:






static nsIFile *sPath[2];




static nsZipArchive *sReader[2];




static PRPackedBool sInitialized;

public:
enum Type {
    GRE = 0,
    APP = 1
};





static inline PRPackedBool
IsInitialized()
{
    return sInitialized;
}








static void Init(nsIFile *aGrePath = nsnull, nsIFile *aAppPath = nsnull);




static void CleanUp();






static inline already_AddRefed<nsIFile>
GetPath(Type aType)
{
    NS_ABORT_IF_FALSE(IsInitialized(), "Omnijar not initialized");
    NS_IF_ADDREF(sPath[aType]);
    return sPath[aType];
}





static inline PRBool
HasOmnijar(Type aType)
{
    NS_ABORT_IF_FALSE(IsInitialized(), "Omnijar not initialized");
    return !!sPath[aType];
}





static inline nsZipArchive *
GetReader(Type aType)
{
    NS_ABORT_IF_FALSE(IsInitialized(), "Omnijar not initialized");
    return sReader[aType];
}





static nsZipArchive *GetReader(nsIFile *aPath);








static nsresult GetURIString(Type aType, nsACString &result);

private:



static void InitOne(nsIFile *aPath, Type aType);
static void CleanUpOne(Type aType);

}; 

} 

#endif
