




#ifndef nsEntityConverter_h__
#define nsEntityConverter_h__

#include "nsIEntityConverter.h"
#include "nsIStringBundle.h"
#include "nsCOMPtr.h"

#define kVERSION_STRING_LEN 128

class nsEntityVersionList
{
public:
    nsEntityVersionList() {}
    
    uint32_t mVersion;
    char16_t mEntityListName[kVERSION_STRING_LEN+1];
    nsCOMPtr<nsIStringBundle> mEntities;
};

class nsEntityConverter: public nsIEntityConverter
{
public:
	
	
	
	
	nsEntityConverter();

	
	
	
	NS_DECL_ISUPPORTS

	
	
	
	NS_IMETHOD ConvertUTF32ToEntity(uint32_t character, uint32_t entityVersion, char **_retval);
	NS_IMETHOD ConvertToEntity(char16_t character, uint32_t entityVersion, char **_retval);

	NS_IMETHOD ConvertToEntities(const char16_t *inString, uint32_t entityVersion, char16_t **_retval);

protected:

  
  NS_IMETHOD LoadVersionPropertyFile();

  
  const char16_t* GetVersionName(uint32_t versionNumber);

  
  nsIStringBundle* GetVersionBundleInstance(uint32_t versionNumber);

  
  already_AddRefed<nsIStringBundle> LoadEntityBundle(uint32_t version);


  nsEntityVersionList *mVersionList;            
  uint32_t mVersionListLength;                  

  virtual ~nsEntityConverter();
};

#endif
