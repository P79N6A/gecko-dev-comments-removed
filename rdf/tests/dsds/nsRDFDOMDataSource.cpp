




































#ifndef __gen_nsRDFDOMDataSource_h__
#define __gen_nsRDFDOMDataSource_h__

#include "nsRDFDOMDataSource.h"




#define NS_IRDFDATASOURCE_IID_STR "0F78DA58-8321-11d2-8EAC-00805F29F370"
#define NS_IRDFDATASOURCE_IID \
  {0x0F78DA58, 0x8321, 0x11d2, \
    { 0x8E, 0xAC, 0x00, 0x80, 0x5F, 0x29, 0xF3, 0x70 }}

class nsRDFDOMDataSource : public nsISupports {
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRDFDATASOURCE_IID)

  
  NS_IMETHOD Init(const char *uri) = 0;

  
  NS_IMETHOD GetURI(char * *aURI) = 0;

  
  NS_IMETHOD GetSource(nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue, nsIRDFResource **_retval) = 0;

  
  NS_IMETHOD GetSources(nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue, nsISimpleEnumerator **_retval) = 0;

  
  NS_IMETHOD GetTarget(nsIRDFResource *aSource, nsIRDFResource *aProperty, PRBool aTruthValue, nsIRDFNode **_retval) = 0;

  
  NS_IMETHOD GetTargets(nsIRDFResource *aSource, nsIRDFResource *aProperty, PRBool aTruthValue, nsISimpleEnumerator **_retval) = 0;

  
  NS_IMETHOD Assert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue) = 0;

  
  NS_IMETHOD Unassert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget) = 0;

  
  NS_IMETHOD HasAssertion(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue, PRBool *_retval) = 0;

  
  NS_IMETHOD AddObserver(nsIRDFObserver *aObserver) = 0;

  
  NS_IMETHOD RemoveObserver(nsIRDFObserver *aObserver) = 0;

  
  NS_IMETHOD ArcLabelsIn(nsIRDFNode *aNode, nsISimpleEnumerator **_retval) = 0;

  
  NS_IMETHOD ArcLabelsOut(nsIRDFResource *aSource, nsISimpleEnumerator **_retval) = 0;

  
  NS_IMETHOD GetAllResources(nsISimpleEnumerator **_retval) = 0;

  
  NS_IMETHOD Flush() = 0;

  
  NS_IMETHOD GetAllCmds(nsIRDFResource *aSource, nsISimpleEnumerator **_retval) = 0;

  
  NS_IMETHOD IsCommandEnabled(nsISupportsArray * aSources, nsIRDFResource *aCommand, nsISupportsArray * aArguments, PRBool *_retval) = 0;

  
  NS_IMETHOD DoCommand(nsISupportsArray * aSources, nsIRDFResource *aCommand, nsISupportsArray * aArguments) = 0;

  
  NS_IMETHOD BeginUpdateBatch() = 0;

  
  NS_IMETHOD EndUpdateBatch() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsRDFDOMDataSource, NS_IRDFDATASOURCE_IID)

#endif 
