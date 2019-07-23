











































#include "txStandaloneStylesheetCompiler.h"
#include "txStandaloneXSLTProcessor.h"
#include "nsXPCOM.h"
#include "xmExternalDriver.hpp"
#ifdef MOZ_JPROF
#include "jprof.h"
#endif

class txDriverProcessor : public txStandaloneXSLTProcessor,
                          public xmExternalDriver
{
public:
    txDriverProcessor() : mXML(0), mOut(0)
    {
    }

    int loadStylesheet (char * filename)
    {
        txParsedURL url;
        url.init(NS_ConvertASCIItoUTF16(filename));
        nsresult rv =
            TX_CompileStylesheetPath(url, getter_AddRefs(mStylesheet));
        return NS_SUCCEEDED(rv) ? 0 : 1;
    }
    int setInputDocument (char * filename)
    {
        delete mXML;
        mXML = parsePath(nsDependentCString(filename), mObserver);
        return mXML ? 0 : 1;
    }
    int openOutput (char * outputFilename)
    {
        mOut = new ofstream(outputFilename);
        return mXML ? 0 : 1;
    }
    int runTransform ()
    {
        if (!mXML || !mStylesheet || !mOut)
            return 1;
        nsresult rv = transform(*mXML, mStylesheet, *mOut, mObserver);
        return NS_FAILED(rv);
    }
    int closeOutput ()
    {
        if (mOut)
            mOut->close();
        delete mOut;
        mOut = 0;
        return 0;
    }
    int terminate()
    {
        delete mXML;
        mXML = 0;
        if (mOut && mOut->is_open())
            mOut->close();
        delete mOut;
        mOut = 0;
        mStylesheet = 0;
        return 0;
    }
    ~txDriverProcessor()
    {
        delete mXML;
        delete mOut;
    }
private:
    txXPathNode *mXML;
    nsRefPtr<txStylesheet> mStylesheet;
    SimpleErrorObserver mObserver;
    ofstream* mOut;
};

int main (int argc, char ** argv)
{
    txDriverProcessor driver;
#ifdef MOZ_JPROF
    setupProfilingStuff();
#endif
    NS_InitXPCOM2(nsnull, nsnull, nsnull);
    if (!txDriverProcessor::init())
        return 1;
    driver.main (argc, argv);
    txDriverProcessor::shutdown();
    NS_ShutdownXPCOM(nsnull);
    return 0;
}
