









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_UTIL_UTILITY_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_UTIL_UTILITY_H_

#include <stdlib.h>
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define OPEN_FILE_WB(filePtr, fullPath)                         \
  do                                                            \
  {                                                             \
    if(fullPath != NULL)                                        \
    {                                                           \
      filePtr = fopen(fullPath, "wb");                          \
      if(filePtr == NULL)                                       \
      {                                                         \
        printf("could not open %s to write to.", fullPath);     \
        return -1;                                              \
      }                                                         \
    }                                                           \
    else                                                        \
    {                                                           \
      filePtr = NULL;                                           \
    }                                                           \
  }while(0)

#define OPEN_FILE_AB(filePtr, fullPath)                         \
  do                                                            \
  {                                                             \
    if(fullPath != NULL)                                        \
    {                                                           \
      filePtr = fopen(fullPath, "ab");                          \
      if(filePtr == NULL)                                       \
      {                                                         \
        printf("could not open %s to write to.", fullPath);     \
        return -1;                                              \
      }                                                         \
    }                                                           \
    else                                                        \
    {                                                           \
      filePtr = NULL;                                           \
    }                                                           \
  }while(0)

#define OPEN_FILE_RB(filePtr, fullPath)                         \
  do                                                            \
  {                                                             \
    if(fullPath != NULL)                                        \
    {                                                           \
      filePtr = fopen(fullPath, "rb");                          \
      if(filePtr == NULL)                                       \
      {                                                         \
        printf("could not open %s to read from.", fullPath);    \
        return -1;                                              \
      }                                                         \
    }                                                           \
    else                                                        \
    {                                                           \
      filePtr = NULL;                                           \
    }                                                           \
  }while(0)

#define WRITE_FILE_D(bufferPtr, len, filePtr)           \
  do                                                    \
  {                                                     \
    if(filePtr != NULL)                                 \
    {                                                   \
      double dummy[1000];                               \
      int cntr;                                         \
      for(cntr = 0; cntr < (len); cntr++)               \
      {                                                 \
        dummy[cntr] = (double)bufferPtr[cntr];          \
      }                                                 \
      fwrite(dummy, sizeof(double), len, filePtr);      \
      fflush(filePtr);                                  \
    }                                                   \
  } while(0)

  typedef struct {
    unsigned int whenPackGeneratedMs;
    unsigned int whenPrevPackLeftMs;
    unsigned int sendTimeMs ;          
    unsigned int arrival_time;         
    unsigned int sample_count;         
    unsigned int rtp_number;
  } BottleNeckModel;

  void get_arrival_time(
      int              current_framesamples,   
      int              packet_size,            
      int              bottleneck,             
      BottleNeckModel* BN_data,
      short            senderSampFreqHz,
      short            receiverSampFreqHz);

  
  int readframe(
      short* data,
      FILE*  inp,
      int    length);

  short readSwitch(
      int   argc,
      char* argv[],
      char* strID);

  double readParamDouble(
      int    argc,
      char*  argv[],
      char*  strID,
      double defaultVal);

  int readParamInt(
      int   argc,
      char* argv[],
      char* strID,
      int   defaultVal);

  int readParamString(
      int   argc,
      char* argv[],
      char* strID,
      char* stringParam,
      int   maxSize);

#if defined(__cplusplus)
}
#endif



#endif
