



































#include <string>
#include "sampler.h"
#include "nsProfiler.h"
#include "nsMemory.h"

using std::string;

NS_IMPL_ISUPPORTS1(nsProfiler, nsIProfiler)


nsProfiler::nsProfiler()
{
}


NS_IMETHODIMP
nsProfiler::StartProfiler(PRUint32 aInterval, PRUint32 aEntries)
{
  SAMPLER_START(aInterval, aEntries);
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
  PRUint32 len = strlen(profile);
  char *profileStr = static_cast<char *>
                       (nsMemory::Clone(profile, len * sizeof(char)));
  *aProfile = profileStr;
  free(profile);
  return NS_OK;
}

NS_IMETHODIMP
nsProfiler::IsActive(bool *aIsActive)
{
  *aIsActive = SAMPLER_IS_ACTIVE();
  return NS_OK;
}

NS_IMETHODIMP
nsProfiler::GetResponsivenessTimes(PRUint32 *aCount, float **aResult)
{
  unsigned int len = 100;
  const float* times = SAMPLER_GET_RESPONSIVENESS();
  if (!times) {
    *aCount = 0;
    *aResult = nsnull;
    return NS_OK;
  }

  float *fs = static_cast<float *>
                       (nsMemory::Clone(times, len * sizeof(float)));

  *aCount = len;
  *aResult = fs;

  return NS_OK;
}

