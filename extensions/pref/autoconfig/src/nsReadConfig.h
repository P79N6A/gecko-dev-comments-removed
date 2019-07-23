





































#include "nsCOMPtr.h"
#include "nsIReadConfig.h"
#include "nsIAutoConfig.h"
#include "nsIObserver.h"


class nsReadConfig : public nsIReadConfig,
                     public nsIObserver
{

    public:

        NS_DECL_ISUPPORTS
        NS_DECL_NSIREADCONFIG
        NS_DECL_NSIOBSERVER

        nsReadConfig();
        virtual ~nsReadConfig();

        nsresult Init();

    protected:
  
        nsresult readConfigFile();
        nsresult openAndEvaluateJSFile(const char *aFileName, PRInt32 obscureValue, 
                                        PRBool isEncoded, PRBool isBinDir);
        PRBool mRead;
private:
        nsCOMPtr<nsIAutoConfig> mAutoConfig;
};
