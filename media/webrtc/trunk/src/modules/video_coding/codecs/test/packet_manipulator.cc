









#include "modules/video_coding/codecs/test/packet_manipulator.h"

#include <cassert>
#include <cstdio>

namespace webrtc {
namespace test {

PacketManipulatorImpl::PacketManipulatorImpl(PacketReader* packet_reader,
                                             const NetworkingConfig& config,
                                             bool verbose)
    : packet_reader_(packet_reader),
      config_(config),
      active_burst_packets_(0),
      critsect_(CriticalSectionWrapper::CreateCriticalSection()),
      random_seed_(1),
      verbose_(verbose) {
  assert(packet_reader);
}

PacketManipulatorImpl::~PacketManipulatorImpl() {
  delete critsect_;
}

int PacketManipulatorImpl::ManipulatePackets(
    webrtc::EncodedImage* encoded_image) {
  assert(encoded_image);
  int nbr_packets_dropped = 0;
  
  
  
  
  
  int new_length = 0;
  packet_reader_->InitializeReading(encoded_image->_buffer,
                                    encoded_image->_length,
                                    config_.packet_size_in_bytes);
  WebRtc_UWord8* packet = NULL;
  int nbr_bytes_to_read;
  
  
  bool packet_loss_has_occurred = false;
  while ((nbr_bytes_to_read = packet_reader_->NextPacket(&packet)) > 0) {
    
    if (active_burst_packets_ > 0) {
      active_burst_packets_--;
      nbr_packets_dropped++;
    } else if (RandomUniform() < config_.packet_loss_probability ||
        packet_loss_has_occurred) {
      packet_loss_has_occurred = true;
      nbr_packets_dropped++;
      if (config_.packet_loss_mode == kBurst) {
        
        active_burst_packets_ = config_.packet_loss_burst_length - 1;
      }
    } else {
      new_length += nbr_bytes_to_read;
    }
  }
  encoded_image->_length = new_length;
  if (nbr_packets_dropped > 0) {
    
    encoded_image->_completeFrame = false;
    if (verbose_) {
      printf("Dropped %d packets for frame %d (frame length: %d)\n",
             nbr_packets_dropped, encoded_image->_timeStamp,
             encoded_image->_length);
    }
  }
  return nbr_packets_dropped;
}

void PacketManipulatorImpl::InitializeRandomSeed(unsigned int seed) {
  random_seed_ = seed;
}

inline double PacketManipulatorImpl::RandomUniform() {
  
  
  
  critsect_->Enter();
  srand(random_seed_);
  random_seed_ = std::rand();
  critsect_->Leave();
  return (random_seed_ + 1.0)/(RAND_MAX + 1.0);
}

const char* PacketLossModeToStr(PacketLossMode e) {
  switch (e) {
    case kUniform:
      return "Uniform";
    case kBurst:
      return "Burst";
    default:
      assert(false);
      return "Unknown";
  }
}

}  
}  
