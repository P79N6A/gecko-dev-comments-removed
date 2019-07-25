



































#include "nsProfiler.h"
#include <stdio.h>

NS_IMPL_ISUPPORTS1(nsProfiler, nsIProfiler)


nsProfiler::nsProfiler()
{
}


NS_IMETHODIMP
nsProfiler::StartProfiler()
{
  return NS_OK;
}

NS_IMETHODIMP
nsProfiler::StopProfiler()
{
  return NS_OK;
}

