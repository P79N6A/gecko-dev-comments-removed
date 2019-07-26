




#ifndef nsReadConfig_h
#define nsReadConfig_h

#include "nsCOMPtr.h"
#include "nsIReadConfig.h"
#include "nsIAutoConfig.h"
#include "nsIObserver.h"


class nsReadConfig : public nsIReadConfig,
                     public nsIObserver
{

    public:

        NS_DECL_THREADSAFE_ISUPPORTS
        NS_DECL_NSIREADCONFIG
        NS_DECL_NSIOBSERVER

        nsReadConfig();

        nsresult Init();

    protected:

        virtual ~nsReadConfig();

        nsresult readConfigFile();
        nsresult openAndEvaluateJSFile(const char *aFileName, int32_t obscureValue, 
                                        bool isEncoded, bool isBinDir);
        bool mRead;
private:
        nsCOMPtr<nsIAutoConfig> mAutoConfig;
};

#endif
