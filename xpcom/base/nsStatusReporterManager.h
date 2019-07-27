






#include "nsIStatusReporter.h"
#include "nsCOMArray.h"
#include "nsString.h"

class nsStatusReporter final : public nsIStatusReporter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTATUSREPORTER

  nsStatusReporter(nsACString& aProcess, nsACString& aDesc);

private:
  nsCString sProcess;
  nsCString sName;
  nsCString sDesc;

  virtual ~nsStatusReporter();
};


class nsStatusReporterManager : public nsIStatusReporterManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTATUSREPORTERMANAGER

  nsStatusReporterManager();

private:
  nsCOMArray<nsIStatusReporter> mReporters;

  virtual ~nsStatusReporterManager();
};
