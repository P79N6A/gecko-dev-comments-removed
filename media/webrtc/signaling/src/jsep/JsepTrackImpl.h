



#ifndef _JSEPTRACKIMPL_H_
#define _JSEPTRACKIMPL_H_

#include <map>

#include <mozilla/RefPtr.h>
#include <mozilla/UniquePtr.h>

#include "signaling/src/jsep/JsepCodecDescription.h"
#include "signaling/src/jsep/JsepTrack.h"
#include "signaling/src/sdp/Sdp.h"
#include "signaling/src/sdp/SdpMediaSection.h"

namespace mozilla {

class JsepTrackNegotiatedDetailsImpl : public JsepTrackNegotiatedDetails
{
public:
  virtual ~JsepTrackNegotiatedDetailsImpl()
  {
    for (auto c = mCodecs.begin(); c != mCodecs.end(); ++c) {
      delete *c;
    }
  }

  
  virtual mozilla::SdpMediaSection::Protocol
  GetProtocol() const MOZ_OVERRIDE
  {
    return mProtocol;
  }
  virtual Maybe<std::string>
  GetBandwidth(const std::string& type) const MOZ_OVERRIDE
  {
    return mBandwidth;
  }
  virtual size_t
  GetCodecCount() const MOZ_OVERRIDE
  {
    return mCodecs.size();
  }
  virtual nsresult
  GetCodec(size_t index, const JsepCodecDescription** config) const MOZ_OVERRIDE
  {
    if (index >= mCodecs.size()) {
      return NS_ERROR_INVALID_ARG;
    }
    *config = mCodecs[index];
    return NS_OK;
  }

  virtual const SdpExtmapAttributeList::Extmap*
  GetExt(const std::string& ext_name) const MOZ_OVERRIDE
  {
    auto it = mExtmap.find(ext_name);
    if (it != mExtmap.end()) {
      return &it->second;
    }
    return nullptr;
  }

  virtual std::vector<uint8_t> GetUniquePayloadTypes() const MOZ_OVERRIDE
  {
    return mUniquePayloadTypes;
  }

  virtual void AddUniquePayloadType(uint8_t pt) MOZ_OVERRIDE
  {
    mUniquePayloadTypes.push_back(pt);
  }

  virtual void ClearUniquePayloadTypes() MOZ_OVERRIDE
  {
    mUniquePayloadTypes.clear();
  }

private:
  
  
  friend class JsepSessionImpl;

  mozilla::SdpMediaSection::Protocol mProtocol;
  Maybe<std::string> mBandwidth;
  std::vector<JsepCodecDescription*> mCodecs;
  std::map<std::string, SdpExtmapAttributeList::Extmap> mExtmap;
  std::vector<uint8_t> mUniquePayloadTypes;
};

} 

#endif
