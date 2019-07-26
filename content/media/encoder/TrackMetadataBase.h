




#ifndef TrackMetadataBase_h_
#define TrackMetadataBase_h_

#include "nsTArray.h"
#include "nsCOMPtr.h"
namespace mozilla {


class TrackMetadataBase
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TrackMetadataBase)
  enum MetadataKind {
    METADATA_OPUS,    
    METADATA_VP8,
    METADATA_VORBIS,
    METADATA_AVC,
    METADATA_AAC,
    METADATA_UNKNOWN  
  };
  virtual ~TrackMetadataBase() {}
  
  virtual MetadataKind GetKind() const = 0;
};

}
#endif
