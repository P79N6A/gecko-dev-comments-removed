







































#include "xpctest_multiple.h"
#include "nsISupports.h"
#include "xpctest_private.h"
#include "nsIClassInfoImpl.h"

class xpcTestParentOne : public nsIXPCTestParentOne {
public: 
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTPARENTONE
    xpcTestParentOne();
private: 
    const char *name;
};

NS_IMPL_ISUPPORTS1(xpcTestParentOne, nsIXPCTestParentOne)

xpcTestParentOne :: xpcTestParentOne() 
{
    NS_ADDREF_THIS();
}

NS_IMETHODIMP xpcTestParentOne :: GetParentOneAttribute(char * *_retval) 
{                                                             
    char aString[] = "xpcTestParentOne attribute";     
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                                            

NS_IMETHODIMP xpcTestParentOne :: SetParentOneAttribute(const char * aParentOneAttribute) 
{                                                             
    name = aParentOneAttribute;                               
    return NS_OK;                                             
}                                                                  

NS_IMETHODIMP                                                 
xpcTestParentOne :: ParentOneMethod(char * *_retval)                   
{                                                             
    char aString[] = "xpcTestParentOne method";  
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                                                   
NS_IMETHODIMP
xpctest::ConstructXPCTestParentOne(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestParentOne *obj = new xpcTestParentOne();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}                                                                




class xpcTestParentTwo : public nsIXPCTestParentTwo {
public: 
  NS_DECL_NSIXPCTESTPARENTTWO
  NS_DECL_ISUPPORTS
  xpcTestParentTwo();
private:
    const char *name;
};

NS_IMPL_ISUPPORTS1(xpcTestParentTwo, nsIXPCTestParentTwo)

xpcTestParentTwo :: xpcTestParentTwo()
{
    NS_ADDREF_THIS();
}

NS_IMETHODIMP                                                 
xpcTestParentTwo :: GetParentTwoAttribute(char * *_retval)             
{                                                             
    char aString[] = "xpcTestParentTwo attribute";               
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                                        

NS_IMETHODIMP                                                 
xpcTestParentTwo :: SetParentTwoAttribute(const char * aParentTwoAttribute)  
{                                                             
    name = aParentTwoAttribute;                               
    return NS_OK;                                             
}                                                                   

NS_IMETHODIMP                                                 
xpcTestParentTwo :: ParentTwoMethod(char **_retval) 
{                  
    char aString[] = "xpcTestParentTwo method";  
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}
  
NS_IMETHODIMP
xpctest::ConstructXPCTestParentTwo(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestParentTwo *obj = new xpcTestParentTwo();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}                                                                






class xpcTestChild2 : public nsIXPCTestChild2, public nsIXPCTestParentOne, public nsIXPCTestParentTwo {
public: 
  NS_DECL_NSIXPCTESTCHILD2
  NS_DECL_NSIXPCTESTPARENTONE
  NS_DECL_NSIXPCTESTPARENTTWO
  NS_DECL_ISUPPORTS
  xpcTestChild2();
private:
    const char *name;
};

NS_IMPL_ISUPPORTS3_CI(xpcTestChild2,nsIXPCTestChild2,nsIXPCTestParentOne,nsIXPCTestParentTwo)

xpcTestChild2 :: xpcTestChild2() 
{
    NS_ADDREF_THIS();
}

NS_IMETHODIMP 
xpcTestChild2:: GetChildAttribute(char * *_retval) 
{
    char aString[] = "xpcTestChild2 attribute";    
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}

NS_IMETHODIMP 
xpcTestChild2:: SetChildAttribute(const char * aChildAttribute) 
{
    name = aChildAttribute;
    return NS_OK;
}

NS_IMETHODIMP
xpcTestChild2 ::ChildMethod(char **_retval) 
{
    char aString[] = "xpcTestChild2 method";  
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}


NS_IMETHODIMP                                                 
xpcTestChild2 :: GetParentOneAttribute(char * *_retval)     
{                                                             
    char aString[] = "xpcTestChild2 attribute";     
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                                            

NS_IMETHODIMP                                                 
xpcTestChild2:: SetParentOneAttribute(const char * aParentOneAttribute) 
{                                                             
    name = aParentOneAttribute;                               
    return NS_OK;                                             
}                                                                  

NS_IMETHODIMP                                                 
xpcTestChild2 :: ParentOneMethod(char * *_retval)                   
{                                                             
    char aString[] = "xpcTestChild2 method";  
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                  

NS_IMETHODIMP                                                 
xpcTestChild2 :: GetParentTwoAttribute(char * *_retval)             
{                                                             
    char aString[] = "xpcTestChild2 attribute";               
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                                            

NS_IMETHODIMP                                                 
xpcTestChild2 :: SetParentTwoAttribute(const char * aParentTwoAttribute)  
{                                                             
    name = aParentTwoAttribute;                               
    return NS_OK;                                             
}                                                                  

NS_IMETHODIMP                                                 
xpcTestChild2 :: ParentTwoMethod(char **_retval) {                  
    char aString[] = "xpcTestChild2 method";  
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                 

NS_IMETHODIMP
xpctest::ConstructXPCTestChild2(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestChild2 *obj = new xpcTestChild2();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}








class xpcTestChild3 : public nsIXPCTestChild3 {
public: 
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCTESTCHILD3
  NS_DECL_NSIXPCTESTPARENTONE
  xpcTestChild3();
private:
    const char *name;
};

NS_IMPL_ISUPPORTS2(xpcTestChild3,nsIXPCTestChild3,nsIXPCTestParentOne)

xpcTestChild3 :: xpcTestChild3() 
{
    NS_ADDREF_THIS();

}

NS_IMETHODIMP 
xpcTestChild3:: GetChildAttribute(char * *_retval) 
{
    char aString[] = "xpcTestChild3 attribute";    
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}

NS_IMETHODIMP 
xpcTestChild3 :: SetChildAttribute(const char * aChildAttribute) 
{
    name = aChildAttribute;
    return NS_OK;
}

NS_IMETHODIMP
xpcTestChild3 ::ChildMethod(char **_retval) 
{
    const char aString[] = "xpcTestChild3 method";
    *_retval = (char*) nsMemory::Clone((const char *)aString,
                sizeof(char)*(strlen((const char *)aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}


NS_IMETHODIMP                                                 
xpcTestChild3 :: GetParentOneAttribute(char * *_retval)     
{                                                             
    const char aString[] = "xpcTestChild3 attribute";     
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                                            

NS_IMETHODIMP                                                 
xpcTestChild3:: SetParentOneAttribute(const char * aParentOneAttribute) 
{                                                             
    name = aParentOneAttribute;                               
    return NS_OK;                                             
}                                                                  

NS_IMETHODIMP                                                 
xpcTestChild3 :: ParentOneMethod(char * *_retval)                   
{                                                             
    char aString[] = "xpcTestChild3 method";  
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                  

NS_IMETHODIMP
xpctest::ConstructXPCTestChild3(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestChild3 *obj = new xpcTestChild3();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}








class xpcTestChild4 : public nsIXPCTestChild4, public xpcTestParentOne, public xpcTestParentTwo {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTCHILD4
    xpcTestChild4();
private:
    const char *name;
};

NS_IMPL_ISUPPORTS3(xpcTestChild4,nsIXPCTestChild4,nsIXPCTestParentOne,nsIXPCTestParentTwo)

xpcTestChild4 :: xpcTestChild4() 
{
    NS_ADDREF_THIS();
}

NS_IMETHODIMP 
xpcTestChild4:: GetChildAttribute(char * *_retval) 
{
    char aString[] = "xpcTestChild4 attribute";    
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}

NS_IMETHODIMP 
xpcTestChild4:: SetChildAttribute(const char * aChildAttribute) 
{
    name = aChildAttribute;
    return NS_OK;
}

NS_IMETHODIMP
xpcTestChild4 ::ChildMethod(char **_retval) 
{
    char aString[] = "xpcTestChild4 method";  
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}

NS_IMETHODIMP
xpctest::ConstructXPCTestChild4(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestChild4 *obj = new xpcTestChild4();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}







class xpcTestChild5 : public nsIXPCTestChild5, public xpcTestParentTwo {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTCHILD5
    NS_DECL_NSIXPCTESTPARENTONE
    xpcTestChild5();
private:
    const char *name;
};

NS_IMPL_ISUPPORTS3(xpcTestChild5,nsIXPCTestChild5,nsIXPCTestParentOne,nsIXPCTestParentTwo)

xpcTestChild5 :: xpcTestChild5() 
{
    NS_ADDREF_THIS();
}

NS_IMETHODIMP 
xpcTestChild5:: GetChildAttribute(char * *_retval) 
{
    char aString[] = "xpcTestChild5 attribute";    
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}

NS_IMETHODIMP 
xpcTestChild5:: SetChildAttribute(const char * aChildAttribute) 
{
    name = aChildAttribute;
    return NS_OK;
}

NS_IMETHODIMP
xpcTestChild5 ::ChildMethod(char **_retval) 
{
    char aString[] = "xpcTestChild5 method";  
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}


NS_IMETHODIMP                                                 
xpcTestChild5 :: GetParentOneAttribute(char * *_retval)     
{                                                             
    char aString[] = "xpcTestChild5 attribute";     
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                                            

NS_IMETHODIMP                                                 
xpcTestChild5:: SetParentOneAttribute(const char * aParentOneAttribute) 
{                                                             
    name = aParentOneAttribute;                               
    return NS_OK;                                             
}                                                                  

NS_IMETHODIMP                                                 
xpcTestChild5 :: ParentOneMethod(char * *_retval)                   
{                                                             
    char aString[] = "xpcTestChild5 method";  
    *_retval = (char*) nsMemory::Clone(aString,            
                sizeof(char)*(strlen(aString)+1));            
    return **_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;        
}                                  

NS_IMETHODIMP
xpctest::ConstructXPCTestChild5(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcTestChild5 *obj = new xpcTestChild5();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}
