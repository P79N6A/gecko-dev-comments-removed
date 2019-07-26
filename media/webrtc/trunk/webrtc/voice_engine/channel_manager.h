









#ifndef WEBRTC_VOICE_ENGINE_CHANNEL_MANAGER_H
#define WEBRTC_VOICE_ENGINE_CHANNEL_MANAGER_H

#include <vector>

#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class Config;

namespace voe {

class Channel;

















class ChannelOwner {
 public:
  explicit ChannelOwner(Channel* channel);
  ChannelOwner(const ChannelOwner& channel_owner);

  ~ChannelOwner();

  ChannelOwner& operator=(const ChannelOwner& other);

  Channel* channel() { return channel_ref_->channel.get(); }
  bool IsValid() { return channel_ref_->channel.get() != NULL; }
 private:
  
  
  
  struct ChannelRef {
    ChannelRef(Channel* channel);
    const scoped_ptr<Channel> channel;
    Atomic32 ref_count;
  };

  ChannelRef* channel_ref_;
};

class ChannelManager {
 public:
  ChannelManager(uint32_t instance_id, const Config& config);

  
  
  
  
  
  class Iterator {
   public:
    explicit Iterator(ChannelManager* channel_manager);

    Channel* GetChannel();
    bool IsValid();

    void Increment();

   private:
    size_t iterator_pos_;
    std::vector<ChannelOwner> channels_;

    DISALLOW_COPY_AND_ASSIGN(Iterator);
  };

  
  
  
  
  
  ChannelOwner CreateChannel();
  ChannelOwner CreateChannel(const Config& external_config);

  
  
  ChannelOwner GetChannel(int32_t channel_id);
  void GetAllChannels(std::vector<ChannelOwner>* channels);

  void DestroyChannel(int32_t channel_id);
  void DestroyAllChannels();

  size_t NumOfChannels() const;

 private:
  
  ChannelOwner CreateChannelInternal(const Config& config);

  uint32_t instance_id_;

  Atomic32 last_channel_id_;

  scoped_ptr<CriticalSectionWrapper> lock_;
  std::vector<ChannelOwner> channels_;

  const Config& config_;

  DISALLOW_COPY_AND_ASSIGN(ChannelManager);
};
}  
}  

#endif  
