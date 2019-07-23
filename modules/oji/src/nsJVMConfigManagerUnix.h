




































#ifndef nsJVMConfigManagerUnix_h___
#define nsJVMConfigManagerUnix_h___

#include "nsJVMConfigManager.h"
#include "nsString.h"
#include "nsILineInputStream.h"
#include "nsHashtable.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsIFileStreams.h"

class nsJVMConfigManagerUnix : public nsIJVMConfigManager
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIJVMCONFIGMANAGER

    nsJVMConfigManagerUnix();
    virtual ~nsJVMConfigManagerUnix();

protected:
    nsresult InitJVMConfigList();

    void ClearJVMConfigList();

    nsresult InitJVMConfigList(nsILineInputStream* aGlobal,
                               nsILineInputStream* aPrivate);

    


    nsresult ParseStream(nsILineInputStream* aStream);

    






    nsresult ParseLine(nsAString& aLine);

    


    nsresult SearchDefault();

    


    nsresult SearchDirectory(nsAString& aDirName);

    



    nsresult AddDirectory(nsIFile* aHomeDir);
    
    nsresult AddDirectory(nsAString& aHomeDirName);

    




    static PRBool GetValueFromLine(nsAString& aLine, const char* aKey,
                                   nsAString& _retval);

    static nsresult GetLineInputStream(nsIFile* aFile,
                                       nsILineInputStream** _retval);

    


    static nsresult GetMozillaPluginPath(nsAString& aLine, nsAString& _retval);

    


    static nsresult GetAgentVersion(nsCAutoString& _retval);

    


    static nsresult GetAgentVersion(float* _retval);

    static nsresult GetNSVersion(nsAString& _retval);

    


    static PRBool TestArch(nsILocalFile* aPluginPath, nsAString& aArch);

    


    static PRBool TestNSVersion(nsILocalFile* aArchPath, nsAString& aNSVersion);

    


    static PRBool TestExists(nsILocalFile* aBaseDir, nsAString& aSubName);

    


    nsHashtable mJVMConfigList;
};

#endif 
