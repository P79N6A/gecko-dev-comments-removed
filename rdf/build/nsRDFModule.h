



































#ifndef nsRDFModule_h___
#define nsRDFModule_h___

#include "rdf.h"
#include "nsIModule.h"


class nsRDFModule : public nsIModule
{
public:
  nsRDFModule();
  virtual ~nsRDFModule();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIMODULE

  nsresult Initialize();

protected:
  void Shutdown();

  PRBool mInitialized;
};

#endif 
