







#include "EbmlIDs.h"
#include "WebMElement.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define kVorbisPrivateMaxSize  4000
#define UInt64 uint64_t

void writeHeader(EbmlGlobal *glob) {
  EbmlLoc start;
  Ebml_StartSubElement(glob, &start, EBML);
  Ebml_SerializeUnsigned(glob, EBMLVersion, 1);
  Ebml_SerializeUnsigned(glob, EBMLReadVersion, 1); 
  Ebml_SerializeUnsigned(glob, EBMLMaxIDLength, 4); 
  Ebml_SerializeUnsigned(glob, EBMLMaxSizeLength, 8); 
  Ebml_SerializeString(glob, DocType, "webm"); 
  Ebml_SerializeUnsigned(glob, DocTypeVersion, 2); 
  Ebml_SerializeUnsigned(glob, DocTypeReadVersion, 2); 
  Ebml_EndSubElement(glob, &start);
}

void writeSimpleBlock(EbmlGlobal *glob, unsigned char trackNumber, short timeCode,
                      int isKeyframe, unsigned char lacingFlag, int discardable,
                      unsigned char *data, unsigned long dataLength) {
  unsigned long blockLength = 4 + dataLength;
  unsigned char flags = 0x00 | (isKeyframe ? 0x80 : 0x00) | (lacingFlag << 1) | discardable;
  Ebml_WriteID(glob, SimpleBlock);
  blockLength |= 0x10000000; 
  Ebml_Serialize(glob, &blockLength, sizeof(blockLength), 4);
  trackNumber |= 0x80;  
  Ebml_Write(glob, &trackNumber, 1);
  
  Ebml_Serialize(glob, &timeCode, sizeof(timeCode), 2);
  flags = 0x00 | (isKeyframe ? 0x80 : 0x00) | (lacingFlag << 1) | discardable;
  Ebml_Write(glob, &flags, 1);
  Ebml_Write(glob, data, dataLength);
}

static UInt64 generateTrackID(unsigned int trackNumber) {
  UInt64 t = time(NULL) * trackNumber;
  UInt64 r = rand();
  r = r << 32;
  r +=  rand();

  return t ^ r;
}

void writeVideoTrack(EbmlGlobal *glob, unsigned int trackNumber, int flagLacing,
                     const char *codecId, unsigned int pixelWidth, unsigned int pixelHeight,
                     unsigned int displayWidth, unsigned int displayHeight,
                     double frameRate) {
  EbmlLoc start;
  UInt64 trackID;
  Ebml_StartSubElement(glob, &start, TrackEntry);
  Ebml_SerializeUnsigned(glob, TrackNumber, trackNumber);
  trackID = generateTrackID(trackNumber);
  Ebml_SerializeUnsigned(glob, TrackUID, trackID);
  Ebml_SerializeString(glob, CodecName, "VP8");  

  Ebml_SerializeUnsigned(glob, TrackType, 1); 
  Ebml_SerializeString(glob, CodecID, codecId);
  {
    EbmlLoc videoStart;
    Ebml_StartSubElement(glob, &videoStart, Video);
    Ebml_SerializeUnsigned(glob, PixelWidth, pixelWidth);
    Ebml_SerializeUnsigned(glob, PixelHeight, pixelHeight);
    if (pixelWidth != displayWidth) {
      Ebml_SerializeUnsigned(glob, DisplayWidth, displayWidth);
    }
    if (pixelHeight != displayHeight) {
      Ebml_SerializeUnsigned(glob, DisplayHeight, displayHeight);
    }
    Ebml_SerializeFloat(glob, FrameRate, frameRate);
    Ebml_EndSubElement(glob, &videoStart); 
  }
  Ebml_EndSubElement(glob, &start); 
}
void writeAudioTrack(EbmlGlobal *glob, unsigned int trackNumber, int flagLacing,
                     const char *codecId, double samplingFrequency, unsigned int channels,
                     unsigned char *private, unsigned long privateSize) {
  EbmlLoc start;
  UInt64 trackID;
  Ebml_StartSubElement(glob, &start, TrackEntry);
  Ebml_SerializeUnsigned(glob, TrackNumber, trackNumber);
  trackID = generateTrackID(trackNumber);
  Ebml_SerializeUnsigned(glob, TrackUID, trackID);
  Ebml_SerializeUnsigned(glob, TrackType, 2); 
  
  



  Ebml_SerializeString(glob, CodecID, codecId);
  Ebml_SerializeData(glob, CodecPrivate, private, privateSize);

  Ebml_SerializeString(glob, CodecName, "VORBIS");  
  {
    EbmlLoc AudioStart;
    Ebml_StartSubElement(glob, &AudioStart, Audio);
    Ebml_SerializeFloat(glob, SamplingFrequency, samplingFrequency);
    Ebml_SerializeUnsigned(glob, Channels, channels);
    Ebml_EndSubElement(glob, &AudioStart);
  }
  Ebml_EndSubElement(glob, &start);
}
void writeSegmentInformation(EbmlGlobal *ebml, EbmlLoc *startInfo, unsigned long timeCodeScale, double duration) {
  Ebml_StartSubElement(ebml, startInfo, Info);
  Ebml_SerializeUnsigned(ebml, TimecodeScale, timeCodeScale);
  Ebml_SerializeFloat(ebml, Segment_Duration, duration * 1000.0); 
  Ebml_SerializeString(ebml, 0x4D80, "QTmuxingAppLibWebM-0.0.1");
  Ebml_SerializeString(ebml, 0x5741, "QTwritingAppLibWebM-0.0.1");
  Ebml_EndSubElement(ebml, startInfo);
}







































































































