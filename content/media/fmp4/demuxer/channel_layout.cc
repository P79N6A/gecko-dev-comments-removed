



#include "mp4_demuxer/channel_layout.h"

#include "mp4_demuxer/basictypes.h"

namespace mp4_demuxer {

static const int kLayoutToChannels[] = {
    0,   
    0,   
    1,   
    2,   
    3,   
    3,   
    4,   
    4,   
    4,   
    5,   
    6,   
    5,   
    6,   
    7,   
    8,   
    8,   
    2,   
    3,   
    4,   
    5,   
    6,   
    6,   
    6,   
    7,   
    7,   
    7,   
    7,   
    8,   
    8,   
    0,   
};







static const int kChannelOrderings[CHANNEL_LAYOUT_MAX][CHANNELS_MAX] = {
    

    
    {  -1 , -1 , -1 , -1  , -1 , -1 , -1    , -1    , -1 , -1 , -1 },

    
    {  -1 , -1 , -1 , -1  , -1 , -1 , -1    , -1    , -1 , -1 , -1 },

    
    {  -1 , -1 , 0  , -1  , -1 , -1 , -1    , -1    , -1 , -1 , -1 },

    
    {  0  , 1  , -1 , -1  , -1 , -1 , -1    , -1    , -1 , -1 , -1 },

    
    {  0  , 1  , -1 , -1  , -1 , -1 , -1    , -1    , 2  , -1 , -1 },

    
    {  0  , 1  , 2  , -1  , -1 , -1 , -1    , -1    , -1 , -1 , -1 },

    
    {  0  , 1  , 2  , -1  , -1 , -1 , -1    , -1    , 3  , -1 , -1 },

    
    {  0  , 1  , -1 , -1  , -1 , -1 , -1    , -1    , -1 , 2  ,  3 },

    
    {  0  , 1  , -1 , -1  , 2  , 3  , -1    , -1    , -1 , -1 , -1 },

    
    {  0  , 1  , 2  , -1  , -1 , -1 , -1    , -1    , -1 , 3  ,  4 },

    
    {  0  , 1  , 2  , 3   , -1 , -1 , -1    , -1    , -1 , 4  ,  5 },

    

    
    {  0  , 1  , 2  , -1  , 3  , 4  , -1    , -1    , -1 , -1 , -1 },

    
    {  0  , 1  , 2  , 3   , 4  , 5  , -1    , -1    , -1 , -1 , -1 },

    
    {  0  , 1  , 2  , -1  , 5  , 6  , -1    , -1    , -1 , 3  ,  4 },

    
    {  0  , 1  , 2  , 3   , 6  , 7  , -1    , -1    , -1 , 4  ,  5 },

    
    {  0  , 1  , 2  , 3   , -1 , -1 , 6     , 7     , -1 , 4  ,  5 },

    
    {  0  , 1  , -1 , -1  , -1 , -1 , -1    , -1    , -1 , -1 , -1 },

    
    {  0  , 1  , -1 ,  2  , -1 , -1 , -1    , -1    , -1 , -1 , -1 },

    
    {  0  , 1  ,  2 ,  3  , -1 , -1 , -1    , -1    , -1 , -1 , -1 },

    
    {  0  , 1  ,  2 ,  4  , -1 , -1 , -1    , -1    ,  3 , -1 , -1 },

    
    {  0  , 1  , 2  , -1  , -1 , -1 , -1    , -1    ,  5 , 3  ,  4 },

    
    {  0  , 1  , -1 , -1  , -1 , -1 ,  4    ,  5    , -1 , 2  ,  3 },

    

    
    {  0  , 1  , 2  , -1  , 3  , 4  , -1    , -1    ,  5 , -1 , -1 },

    
    {  0  , 1  , 2  , 3   , -1 , -1 , -1    , -1    ,  6 , 4  ,  5 },

    
    {  0  , 1  , 2  , 3   , 4  , 5  , -1    , -1    ,  6 , -1 , -1 },

    
    {  0  , 1  , -1 , 6   , -1 , -1 , 4     , 5     , -1 , 2  ,  3 },

    
    {  0  , 1  , 2  , -1  , -1 , -1 , 5     , 6     , -1 , 3  ,  4 },

    
    {  0  , 1  , 2  , 3   , 4  , 5  , 6     , 7     , -1 , -1 , -1 },

    
    {  0  , 1  , 2  , -1  , 5  , 6  , -1    , -1    ,  7 , 3  ,  4 },

    
    {  -1 , -1 , -1 , -1  , -1 , -1 , -1    , -1    , -1 , -1 , -1 },

    
};

int ChannelLayoutToChannelCount(ChannelLayout layout) {
  DCHECK_LT(static_cast<size_t>(layout), arraysize(kLayoutToChannels));
  return kLayoutToChannels[layout];
}


ChannelLayout GuessChannelLayout(int channels) {
  switch (channels) {
    case 1:
      return CHANNEL_LAYOUT_MONO;
    case 2:
      return CHANNEL_LAYOUT_STEREO;
    case 3:
      return CHANNEL_LAYOUT_SURROUND;
    case 4:
      return CHANNEL_LAYOUT_QUAD;
    case 5:
      return CHANNEL_LAYOUT_5_0;
    case 6:
      return CHANNEL_LAYOUT_5_1;
    case 7:
      return CHANNEL_LAYOUT_6_1;
    case 8:
      return CHANNEL_LAYOUT_7_1;
    default:
      DMX_LOG("Unsupported channel count: %d\n", channels);
  }
  return CHANNEL_LAYOUT_UNSUPPORTED;
}

int ChannelOrder(ChannelLayout layout, Channels channel) {
  DCHECK_LT(static_cast<size_t>(layout), arraysize(kChannelOrderings));
  DCHECK_LT(static_cast<size_t>(channel), arraysize(kChannelOrderings[0]));
  return kChannelOrderings[layout][channel];
}

}  
