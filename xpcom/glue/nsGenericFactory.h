




































#ifndef nsGenericFactory_h___
#define nsGenericFactory_h___

#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsIClassInfo.h"





class nsGenericFactory : public nsIGenericFactory, public nsIClassInfo {
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_GENERICFACTORY_CID)

    nsGenericFactory(const nsModuleComponentInfo *info = NULL);
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSICLASSINFO
    
    
    NS_IMETHOD SetComponentInfo(const nsModuleComponentInfo *info);
    NS_IMETHOD GetComponentInfo(const nsModuleComponentInfo **infop);

    NS_IMETHOD CreateInstance(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    NS_IMETHOD LockFactory(PRBool aLock);

    static NS_METHOD Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);
private:
    ~nsGenericFactory();

    const nsModuleComponentInfo *mInfo;
};



#include "nsIModule.h"
#include "plhash.h"

class nsGenericModule : public nsIModule
{
public:
    nsGenericModule(const char* moduleName, 
                    PRUint32 componentCount,
                    const nsModuleComponentInfo* components,
                    nsModuleConstructorProc ctor,
                    nsModuleDestructorProc dtor);

private:
    ~nsGenericModule();

public:
    NS_DECL_ISUPPORTS

    NS_DECL_NSIMODULE

    struct FactoryNode
    {
        FactoryNode(nsIGenericFactory* fact, FactoryNode* next) 
        { 
            mFactory = fact; 
            mNext    = next;
        }
        ~FactoryNode(){}

        nsCOMPtr<nsIGenericFactory> mFactory;
        FactoryNode* mNext;
    };




protected:
    nsresult Initialize(nsIComponentManager* compMgr);

    void Shutdown();
    nsresult AddFactoryNode(nsIGenericFactory* fact);

    PRBool                       mInitialized;
    const char*                  mModuleName;
    PRUint32                     mComponentCount;
    const nsModuleComponentInfo* mComponents;
    FactoryNode*                 mFactoriesNotToBeRegistered;
    nsModuleConstructorProc      mCtor;
    nsModuleDestructorProc       mDtor;
};

#endif 

