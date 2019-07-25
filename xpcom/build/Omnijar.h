






































#ifndef mozilla_Omnijar_h
#define mozilla_Omnijar_h

#include "nscore.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsIFile;
class nsZipArchive;
class nsIURI;

namespace mozilla {

#ifdef MOZ_ENABLE_LIBXUL
#define OMNIJAR_EXPORT
#else
#define OMNIJAR_EXPORT NS_EXPORT
#endif

class OMNIJAR_EXPORT Omnijar {
private:







static nsIFile *sPath[2];



static PRBool sIsOmnijar[2];

#ifdef MOZ_ENABLE_LIBXUL



static nsZipArchive *sReader[2];
#endif

public:
enum Type {
    GRE = 0,
    APP = 1
};





static PRBool
IsInitialized()
{
    
    return sPath[0] != nsnull;
}





static nsresult SetBase(nsIFile *aGrePath, nsIFile *aAppPath);






static already_AddRefed<nsIFile>
GetPath(Type aType)
{
    NS_ABORT_IF_FALSE(sPath[0], "Omnijar not initialized");

    if (sIsOmnijar[aType]) {
        NS_IF_ADDREF(sPath[aType]);
        return sPath[aType];
    }
    return nsnull;
}





static PRBool
HasOmnijar(Type aType)
{
    return sIsOmnijar[aType];
}





static already_AddRefed<nsIFile> GetBase(Type aType);





#ifdef MOZ_ENABLE_LIBXUL
static nsZipArchive *GetReader(Type aType);
#else
static nsZipArchive *GetReader(Type aType) { return nsnull; }
#endif





#ifdef MOZ_ENABLE_LIBXUL
static nsZipArchive *GetReader(nsIFile *aPath);
#else
static nsZipArchive *GetReader(nsIFile *aPath) { return nsnull; }
#endif








static nsresult GetURIString(Type aType, nsCString &result);

}; 

} 

#endif
