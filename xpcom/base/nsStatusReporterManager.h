






#include "nsIStatusReporter.h"
#include "nsCOMArray.h"
#include "nsString.h"

class nsStatusReporter MOZ_FINAL : public nsIStatusReporter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTATUSREPORTER

  nsStatusReporter(nsACString& aProcess, nsACString& aDesc);

  virtual ~nsStatusReporter();

protected:
  nsCString sProcess;
  nsCString sName;
  nsCString sDesc;
};


class nsStatusReporterManager : public nsIStatusReporterManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTATUSREPORTERMANAGER

  nsStatusReporterManager();
  virtual ~nsStatusReporterManager();

private:
  nsCOMArray<nsIStatusReporter> mReporters;
};
