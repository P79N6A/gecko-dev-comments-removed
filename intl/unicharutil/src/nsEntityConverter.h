




































#include "nsIEntityConverter.h"
#include "nsIFactory.h"
#include "nsIStringBundle.h"
#include "nsCOMPtr.h"

nsresult NS_NewEntityConverter(nsISupports** oResult);

#define kVERSION_STRING_LEN 128

class nsEntityVersionList
{
public:
    nsEntityVersionList() : mEntities(NULL) {}
    
    PRUint32 mVersion;
    PRUnichar mEntityListName[kVERSION_STRING_LEN+1];
    nsCOMPtr<nsIStringBundle> mEntities;
};

class nsEntityConverter: public nsIEntityConverter
{
public:
	
	
	
	
	nsEntityConverter();
	virtual ~nsEntityConverter();

	
	
	
	NS_DECL_ISUPPORTS

	
	
	
	NS_IMETHOD ConvertUTF32ToEntity(PRUint32 character, PRUint32 entityVersion, char **_retval);
	NS_IMETHOD ConvertToEntity(PRUnichar character, PRUint32 entityVersion, char **_retval);

	NS_IMETHOD ConvertToEntities(const PRUnichar *inString, PRUint32 entityVersion, PRUnichar **_retval);

protected:

  
  NS_IMETHOD LoadVersionPropertyFile();

  
  const PRUnichar* GetVersionName(PRUint32 versionNumber);

  
  nsIStringBundle* GetVersionBundleInstance(PRUint32 versionNumber);

  
  already_AddRefed<nsIStringBundle> LoadEntityBundle(PRUint32 version);


  nsEntityVersionList *mVersionList;            
  PRUint32 mVersionListLength;                  
};
