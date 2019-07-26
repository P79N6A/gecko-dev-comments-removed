





#ifndef mozilla_dom_fm_radio_settings_h__
#define mozilla_dom_fm_radio_settings_h__

#include "nsIFMRadio.h"

class nsFMRadioSettings : public nsIFMRadioSettings
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFMRADIOSETTINGS

  nsFMRadioSettings(int32_t aUpperLimit, int32_t aLowerLimit, int32_t aChannelWidth);
private:
  ~nsFMRadioSettings();
  int32_t mUpperLimit;
  int32_t mLowerLimit;
  int32_t mChannelWidth;
};
#endif

