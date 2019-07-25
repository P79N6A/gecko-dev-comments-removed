



































#include <string>
#ifdef MOZ_INSTRUMENT_EVENT_LOOP
#include "EventTracer.h"
#endif
#include "sampler.h"
#include "nsProfiler.h"
#include "nsMemory.h"

using std::string;

NS_IMPL_ISUPPORTS1(nsProfiler, nsIProfiler)


nsProfiler::nsProfiler()
{
}


NS_IMETHODIMP
nsProfiler::StartProfiler(PRUint32 aInterval, PRUint32 aEntries,
                          const char** aFeatures, PRUint32 aFeatureCount)
{
  SAMPLER_START(aInterval, aEntries, aFeatures, aFeatureCount);
#ifdef MOZ_INSTRUMENT_EVENT_LOOP
  mozilla::InitEventTracing();
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsProfiler::StopProfiler()
{
  SAMPLER_STOP();
  return NS_OK;
}

NS_IMETHODIMP
nsProfiler::GetProfile(char **aProfile)
{
  char *profile = SAMPLER_GET_PROFILE();
  if (profile) {
    PRUint32 len = strlen(profile);
    char *profileStr = static_cast<char *>
                         (nsMemory::Clone(profile, (len + 1) * sizeof(char)));
    profileStr[len] = '\0';
    *aProfile = profileStr;
    free(profile);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsProfiler::IsActive(bool *aIsActive)
{
  *aIsActive = SAMPLER_IS_ACTIVE();
  return NS_OK;
}

NS_IMETHODIMP
nsProfiler::GetResponsivenessTimes(PRUint32 *aCount, double **aResult)
{
  unsigned int len = 100;
  const double* times = SAMPLER_GET_RESPONSIVENESS();
  if (!times) {
    *aCount = 0;
    *aResult = nsnull;
    return NS_OK;
  }

  double *fs = static_cast<double *>
                       (nsMemory::Clone(times, len * sizeof(double)));

  *aCount = len;
  *aResult = fs;

  return NS_OK;
}

NS_IMETHODIMP
nsProfiler::GetFeatures(PRUint32 *aCount, char ***aFeatures)
{
  PRUint32 len = 0;

  const char **features = SAMPLER_GET_FEATURES();
  if (!features) {
    *aCount = 0;
    *aFeatures = nsnull;
    return NS_OK;
  }

  while (features[len]) {
    len++;
  }

  char **featureList = static_cast<char **>
                       (nsMemory::Alloc(len * sizeof(char*)));

  for (size_t i = 0; i < len; i++) {
    PRUint32 strLen = strlen(features[i]);
    featureList[i] = static_cast<char *>
                         (nsMemory::Clone(features[i], (strLen + 1) * sizeof(char)));
  }

  *aFeatures = featureList;
  *aCount = len;
  return NS_OK;
}
