









































#ifndef __NS_SANE_PLUGIN_FACTORY_H__
#define __NS_SANE_PLUGIN_FACTORY_H__

class nsSanePluginFactoryImpl : public nsIPlugin
{
public:
    nsSanePluginFactoryImpl(const nsCID &aClass, const char* className,
                            const char* contractID);

    
    NS_DECL_ISUPPORTS ;

    
    NS_IMETHOD CreateInstance(nsISupports *aOuter,
                              const nsIID &aIID,
                              void **aResult);

    NS_IMETHOD LockFactory(PRBool aLock);
    NS_IMETHOD Initialize(void);
    NS_IMETHOD Shutdown(void);
    NS_IMETHOD GetMIMEDescription(const char* *result);
    NS_IMETHOD GetValue(nsPluginVariable variable, void *value);
    NS_IMETHOD CreatePluginInstance(nsISupports *aOuter, REFNSIID aIID, 
                                    const char* aPluginMIMEType,
                                    void **aResult);

protected:
    virtual ~nsSanePluginFactoryImpl();

  nsCID       mClassID;
  const char* mClassName;
  const char* mContractID;

};

#endif 





