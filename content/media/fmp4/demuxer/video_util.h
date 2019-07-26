



#ifndef MEDIA_BASE_VIDEO_UTIL_H_
#define MEDIA_BASE_VIDEO_UTIL_H_

#include "mp4_demuxer/basictypes.h"

namespace mp4_demuxer {

class VideoFrame;


IntSize GetNaturalSize(const IntSize& visible_size,
                                    int aspect_ratio_numerator,
                                    int aspect_ratio_denominator);


































































}  

#endif  
