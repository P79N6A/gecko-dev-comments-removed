


































#ifndef IMPDMANAGER_H_
#define IMPDMANAGER_H_

#include "nsTArray.h"
#include "nsIURI.h"
#include "nsIDOMElement.h"

namespace mozilla {
namespace net {


enum DASHMPDProfile
{
  WebMOnDemand,
  NotValid
  
};

class Period;
class Representation;
class Segment;

class IMPDManager
{
public:
  
  enum AdaptationSetType {
    DASH_ASTYPE_INVALID = 0,
    DASH_VIDEO_STREAM,
    DASH_AUDIO_STREAM,
    DAHS_AUDIO_VIDEO_STREAM
  };
  IMPDManager()
  {
    MOZ_COUNT_CTOR(IMPDManager);
  }

  virtual ~IMPDManager()
  {
    MOZ_COUNT_DTOR(IMPDManager);
  }

  
  virtual Period const * GetFirstPeriod() const = 0;

  
  
  
  
  virtual uint32_t GetNumAdaptationSets() const = 0;

  
  virtual AdaptationSetType
          GetAdaptationSetType(uint32_t const aAdaptSetIdx) const = 0;

  
  
  virtual uint32_t
          GetNumRepresentations(uint32_t const aAdaptSetIdx) const = 0;

  
  
  virtual Representation const *
          GetRepresentation(uint32_t const aAdaptSetIdx,
                            uint32_t const aRepIdx) const = 0;

  
  
  virtual nsresult GetFirstSegmentUrl(uint32_t const aAdaptSetIdx,
                                      uint32_t const aRepIdx,
                                      nsAString &aUrl) const = 0;

  
  virtual double GetStartTime() const = 0;

  
  virtual double GetDuration() const = 0;

  
  
  
  
  virtual bool GetBestRepForBandwidth(uint32_t aAdaptSetIdx,
                                      uint64_t aBandwidth,
                                      uint32_t &aRepIdx) const = 0;
public:
  
  static IMPDManager* Create(DASHMPDProfile Profile, nsIDOMElement* aRoot);

private:
  
  static IMPDManager* CreateWebMOnDemandManager(nsIDOMElement* aRoot);
};

}
}

#endif
