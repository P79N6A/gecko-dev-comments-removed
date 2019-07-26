



#include "nsFMRadioSettings.h"

NS_IMPL_ISUPPORTS1(nsFMRadioSettings, nsIFMRadioSettings)

nsFMRadioSettings::nsFMRadioSettings(int32_t aUpperLimit,
                                     int32_t aLowerLimit,
                                     int32_t aChannelWidth)
{
  mUpperLimit = aUpperLimit;
  mLowerLimit = aLowerLimit;
  mChannelWidth  = aChannelWidth;
}

nsFMRadioSettings::~nsFMRadioSettings()
{

}


NS_IMETHODIMP nsFMRadioSettings::GetUpperLimit(int32_t *aUpperLimit)
{
  *aUpperLimit = mUpperLimit;
  return NS_OK;
}

NS_IMETHODIMP nsFMRadioSettings::SetUpperLimit(int32_t aUpperLimit)
{
  mUpperLimit = aUpperLimit;
  return NS_OK;
}


NS_IMETHODIMP nsFMRadioSettings::GetLowerLimit(int32_t *aLowerLimit)
{
  *aLowerLimit = mLowerLimit;
  return NS_OK;
}

NS_IMETHODIMP nsFMRadioSettings::SetLowerLimit(int32_t aLowerLimit)
{
  mLowerLimit = aLowerLimit;
  return NS_OK;
}


NS_IMETHODIMP nsFMRadioSettings::GetChannelWidth(int32_t *aChannelWidth)
{
  *aChannelWidth = mChannelWidth;
  return NS_OK;
}

NS_IMETHODIMP nsFMRadioSettings::SetChannelWidth(int32_t aChannelWidth)
{
  mChannelWidth = aChannelWidth;
  return NS_OK;
}

