




#ifndef nsGeoGridFuzzer_h
#define nsGeoGridFuzzer_h

#include "nsCOMPtr.h"
#include "nsIDOMGeoPosition.h"
#include "nsGeolocationSettings.h"

class nsGeoGridFuzzer final
{
public:

  static already_AddRefed<nsIDOMGeoPosition>
    FuzzLocation(const GeolocationSetting& aSetting, nsIDOMGeoPosition* aPosition);

private:
  nsGeoGridFuzzer() {} 
  nsGeoGridFuzzer(const nsGeoGridFuzzer&) {} 
};

#endif
