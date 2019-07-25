





































#ifndef __NSDOMWORKERLOCATION_H__
#define __NSDOMWORKERLOCATION_H__

#include "nsIClassInfo.h"
#include "nsIDOMWorkers.h"
#include "nsIXPCScriptable.h"

#include "nsCOMPtr.h"
#include "nsStringGlue.h"

class nsIURL;

class nsDOMWorkerLocation : public nsIWorkerLocation,
                            public nsIClassInfo,
                            public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWORKERLOCATION
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIXPCSCRIPTABLE

  static already_AddRefed<nsIWorkerLocation> NewLocation(nsIURL* aURL);

protected:
  nsDOMWorkerLocation() { }

private:
  nsCString mHref;
  nsCString mProtocol;
  nsCString mHost;
  nsCString mHostname;
  nsCString mPort;
  nsCString mPathname;
  nsCString mSearch;
  nsCString mHash;
};

#endif 
