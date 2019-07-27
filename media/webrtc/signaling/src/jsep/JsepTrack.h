



#ifndef _JSEPTRACK_H_
#define _JSEPTRACK_H_

#include <string>

#include <mozilla/RefPtr.h>
#include <mozilla/UniquePtr.h>
#include <mozilla/Maybe.h>
#include "nsISupportsImpl.h"
#include "nsError.h"

#include "signaling/src/jsep/JsepTransport.h"
#include "signaling/src/sdp/Sdp.h"
#include "signaling/src/sdp/SdpMediaSection.h"

namespace mozilla {


struct JsepCodecDescription;

class JsepTrackNegotiatedDetails
{
public:
  virtual ~JsepTrackNegotiatedDetails() {}

  virtual mozilla::SdpMediaSection::Protocol GetProtocol() const = 0;
  virtual Maybe<std::string> GetBandwidth(const std::string& type) const = 0;
  virtual size_t GetCodecCount() const = 0;
  virtual nsresult GetCodec(size_t index,
                            const JsepCodecDescription** config) const = 0;
  virtual const SdpExtmapAttributeList::Extmap* GetExt(
      const std::string& ext_name) const = 0;
  virtual std::vector<uint8_t> GetUniquePayloadTypes() const = 0;

  virtual void AddUniquePayloadType(uint8_t pt) = 0;
  virtual void ClearUniquePayloadTypes() = 0;
};

class JsepTrack
{
public:
  enum Direction { kJsepTrackSending, kJsepTrackReceiving };

  JsepTrack(mozilla::SdpMediaSection::MediaType type,
            const std::string& streamid,
            const std::string& trackid,
            Direction direction = kJsepTrackSending)
      : mType(type),
        mStreamId(streamid),
        mTrackId(trackid),
        mDirection(direction)
  {
  }

  virtual mozilla::SdpMediaSection::MediaType
  GetMediaType() const
  {
    return mType;
  }

  virtual const std::string&
  GetStreamId() const
  {
    return mStreamId;
  }

  virtual void
  SetStreamId(const std::string& id)
  {
    mStreamId = id;
  }

  virtual const std::string&
  GetTrackId() const
  {
    return mTrackId;
  }

  virtual void
  SetTrackId(const std::string& id)
  {
    mTrackId = id;
  }

  virtual const std::string&
  GetCNAME() const
  {
    return mCNAME;
  }

  virtual void
  SetCNAME(const std::string& cname)
  {
    mCNAME = cname;
  }

  virtual Direction
  GetDirection() const
  {
    return mDirection;
  }

  virtual const std::vector<uint32_t>&
  GetSsrcs() const
  {
    return mSsrcs;
  }

  virtual void
  AddSsrc(uint32_t ssrc)
  {
    mSsrcs.push_back(ssrc);
  }

  
  virtual const JsepTrackNegotiatedDetails*
  GetNegotiatedDetails() const
  {
    if (mNegotiatedDetails) {
      return mNegotiatedDetails.get();
    }
    return nullptr;
  }

  virtual JsepTrackNegotiatedDetails*
  GetNegotiatedDetails()
  {
    if (mNegotiatedDetails) {
      return mNegotiatedDetails.get();
    }
    return nullptr;
  }

  
  virtual void
  SetNegotiatedDetails(UniquePtr<JsepTrackNegotiatedDetails> details)
  {
    mNegotiatedDetails = Move(details);
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(JsepTrack);

protected:
  virtual ~JsepTrack() {}

private:
  const mozilla::SdpMediaSection::MediaType mType;
  std::string mStreamId;
  std::string mTrackId;
  std::string mCNAME;
  const Direction mDirection;
  UniquePtr<JsepTrackNegotiatedDetails> mNegotiatedDetails;
  std::vector<uint32_t> mSsrcs;
};


struct JsepTrackPair {
  size_t mLevel;
  
  Maybe<size_t> mBundleLevel;
  RefPtr<JsepTrack> mSending;
  RefPtr<JsepTrack> mReceiving;
  RefPtr<JsepTransport> mRtpTransport;
  RefPtr<JsepTransport> mRtcpTransport;
};

} 

#endif
