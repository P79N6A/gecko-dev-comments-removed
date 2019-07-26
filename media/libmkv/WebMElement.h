







#ifdef __cplusplus
extern "C" {
#endif

#ifndef MKV_CONTEXT_HPP
#define MKV_CONTEXT_HPP 1

#include "EbmlWriter.h"


void writeHeader(EbmlGlobal *ebml);
void writeSegmentInformation(EbmlGlobal *ebml, EbmlLoc *startInfo, unsigned long timeCodeScale, double duration);

void writeVideoTrack(EbmlGlobal *ebml, unsigned int trackNumber, int flagLacing,
                     char *codecId, unsigned int pixelWidth, unsigned int pixelHeight,
                     double frameRate);
void writeAudioTrack(EbmlGlobal *glob, unsigned int trackNumber, int flagLacing,
                     char *codecId, double samplingFrequency, unsigned int channels,
                     unsigned char *private_, unsigned long privateSize);

void writeSimpleBlock(EbmlGlobal *ebml, unsigned char trackNumber, short timeCode,
                      int isKeyframe, unsigned char lacingFlag, int discardable,
                      unsigned char *data, unsigned long dataLength);

#endif

#ifdef __cplusplus
}
#endif
