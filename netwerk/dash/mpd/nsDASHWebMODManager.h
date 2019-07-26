














































#ifndef _NSDASHWEBMODMANAGER_H_
#define _NSDASHWEBMODMANAGER_H_

#include "nsTArray.h"
#include "nsIURI.h"
#include "nsString.h"
#include "MPD.h"
#include "Period.h"
#include "AdaptationSet.h"
#include "Representation.h"
#include "IMPDManager.h"

namespace mozilla {
namespace net {

class nsDASHWebMODManager : public IMPDManager
{
public:
  nsDASHWebMODManager(MPD* mpd);
  ~nsDASHWebMODManager();

  
  Period const * GetFirstPeriod() const;
  uint32_t GetNumAdaptationSets() const;
  AdaptationSetType
                 GetAdaptationSetType(uint32_t const aAdaptSetIdx) const;
  uint32_t GetNumRepresentations(uint32_t const aAdaptSetIdx) const;
  Representation const * GetRepresentation(uint32_t const aAdaptSetIdx,
                                           uint32_t const aRepIdx) const;
  nsresult GetFirstSegmentUrl(uint32_t const aAdaptSetIdx,
                              uint32_t const aRepIdx,
                              nsAString &aUrl) const;
  double GetStartTime() const;
  double GetDuration() const;

private:
  
  AdaptationSet const * GetAdaptationSet(uint32_t const aAdaptSetIdx) const;
  AdaptationSetType GetAdaptationSetType(nsAString const &mimeType) const;
  uint32_t GetNumSegments(Representation const* aRep) const;

  
  nsAutoPtr<MPD> mMpd;
};

}
}

#endif 
