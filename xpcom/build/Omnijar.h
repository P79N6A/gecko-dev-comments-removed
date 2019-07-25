





































#ifndef mozilla_Omnijar_h
#define mozilla_Omnijar_h

class nsILocalFile;
class nsZipArchive;

#ifdef MOZ_OMNIJAR

namespace mozilla {






nsILocalFile *OmnijarPath();
nsZipArchive *OmnijarReader();
void SetOmnijar(nsILocalFile* aPath);

} 

#endif 

#endif 
