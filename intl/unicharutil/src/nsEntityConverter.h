




#ifndef nsEntityConverter_h__
#define nsEntityConverter_h__

#include "nsIEntityConverter.h"
#include "nsIFactory.h"
#include "nsIStringBundle.h"
#include "nsCOMPtr.h"

#define kVERSION_STRING_LEN 128

class nsEntityVersionList
{
public:
    nsEntityVersionList() : mEntities(NULL) {}
    
    uint32_t mVersion;
    PRUnichar mEntityListName[kVERSION_STRING_LEN+1];
    nsCOMPtr<nsIStringBundle> mEntities;
};

class nsEntityConverter: public nsIEntityConverter
{
public:
	
	
	
	
	nsEntityConverter();
	virtual ~nsEntityConverter();

	
	
	
	NS_DECL_ISUPPORTS

	
	
	
	NS_IMETHOD ConvertUTF32ToEntity(uint32_t character, uint32_t entityVersion, char **_retval);
	NS_IMETHOD ConvertToEntity(PRUnichar character, uint32_t entityVersion, char **_retval);

	NS_IMETHOD ConvertToEntities(const PRUnichar *inString, uint32_t entityVersion, PRUnichar **_retval);

protected:

  
  NS_IMETHOD LoadVersionPropertyFile();

  
  const PRUnichar* GetVersionName(uint32_t versionNumber);

  
  nsIStringBundle* GetVersionBundleInstance(uint32_t versionNumber);

  
  already_AddRefed<nsIStringBundle> LoadEntityBundle(uint32_t version);


  nsEntityVersionList *mVersionList;            
  uint32_t mVersionListLength;                  
};

#endif
