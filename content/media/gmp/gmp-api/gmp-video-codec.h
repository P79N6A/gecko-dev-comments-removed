
































#ifndef GMP_VIDEO_CODEC_h_
#define GMP_VIDEO_CODEC_h_

#include <stdint.h>
#include <stddef.h>

enum { kGMPPayloadNameSize = 32};
enum { kGMPMaxSimulcastStreams = 4};

enum GMPVideoCodecComplexity
{
  kGMPComplexityNormal = 0,
  kGMPComplexityHigh = 1,
  kGMPComplexityHigher = 2,
  kGMPComplexityMax = 3,
  kGMPComplexityInvalid 
};

enum GMPVP8ResilienceMode {
  kResilienceOff,    
                     
                     
  kResilientStream,  
                     
                     
  kResilientFrames,  
                     
  kResilienceInvalid 
};


struct GMPVideoCodecVP8
{
  bool mPictureLossIndicationOn;
  bool mFeedbackModeOn;
  GMPVideoCodecComplexity mComplexity;
  GMPVP8ResilienceMode mResilience;
  uint32_t mNumberOfTemporalLayers;
  bool mDenoisingOn;
  bool mErrorConcealmentOn;
  bool mAutomaticResizeOn;
};





struct GMPVideoCodecH264AVCC
{
  uint8_t        mVersion; 
  uint8_t        mProfile; 
  uint8_t        mConstraints;
  uint8_t        mLevel;
  uint8_t        mLengthSizeMinusOne; 

  
  
  uint8_t        mNumSPS; 

  
  
  
  
};




struct GMPVideoCodecH264
{
  uint8_t        mPacketizationMode; 
  struct GMPVideoCodecH264AVCC mAVCC; 
};

enum GMPVideoCodecType
{
  kGMPVideoCodecVP8,

  
  
  
  kGMPVideoCodecH264,
  kGMPVideoCodecInvalid 
};



struct GMPSimulcastStream
{
  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mNumberOfTemporalLayers;
  uint32_t mMaxBitrate; 
  uint32_t mTargetBitrate; 
  uint32_t mMinBitrate; 
  uint32_t mQPMax; 
};

enum GMPVideoCodecMode {
  kGMPRealtimeVideo,
  kGMPScreensharing,
  kGMPStreamingVideo,
  kGMPCodecModeInvalid 
};

enum GMPApiVersion {
  kGMPVersion32 = 1, 
  kGMPVersion33 = 33,
};

struct GMPVideoCodec
{
  uint32_t mGMPApiVersion;

  GMPVideoCodecType mCodecType;
  char mPLName[kGMPPayloadNameSize]; 
  uint32_t mPLType;

  uint32_t mWidth;
  uint32_t mHeight;

  uint32_t mStartBitrate; 
  uint32_t mMaxBitrate; 
  uint32_t mMinBitrate; 
  uint32_t mMaxFramerate;

  bool mFrameDroppingOn;
  int32_t mKeyFrameInterval;

  uint32_t mQPMax;
  uint32_t mNumberOfSimulcastStreams;
  GMPSimulcastStream mSimulcastStream[kGMPMaxSimulcastStreams];

  GMPVideoCodecMode mMode;
};





enum GMPBufferType {
  GMP_BufferSingle = 0,
  GMP_BufferLength8,
  GMP_BufferLength16,
  GMP_BufferLength24,
  GMP_BufferLength32,
  GMP_BufferInvalid,
};

struct GMPCodecSpecificInfoGeneric {
  uint8_t mSimulcastIdx;
};

struct GMPCodecSpecificInfoH264 {
  uint8_t mSimulcastIdx;
};



struct GMPCodecSpecificInfoVP8
{
  bool mHasReceivedSLI;
  uint8_t mPictureIdSLI;
  bool mHasReceivedRPSI;
  uint64_t mPictureIdRPSI;
  int16_t mPictureId; 
  bool mNonReference;
  uint8_t mSimulcastIdx;
  uint8_t mTemporalIdx;
  bool mLayerSync;
  int32_t mTL0PicIdx; 
  int8_t mKeyIdx; 
};

union GMPCodecSpecificInfoUnion
{
  GMPCodecSpecificInfoGeneric mGeneric;
  GMPCodecSpecificInfoVP8 mVP8;
  GMPCodecSpecificInfoH264 mH264;
};




struct GMPCodecSpecificInfo
{
  GMPVideoCodecType mCodecType;
  GMPBufferType mBufferType;
  GMPCodecSpecificInfoUnion mCodecSpecific;
};

#endif 
