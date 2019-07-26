









#ifndef WEBRTC_MODULES_VIDEO_CODING_SOURCE_QM_SELECT_DATA_H_
#define WEBRTC_MODULES_VIDEO_CODING_SOURCE_QM_SELECT_DATA_H_






#include "typedefs.h"

namespace webrtc {





const float kInitBufferLevel = 0.5f;


const float kPercBufferThr = 0.10f;


const float kMaxBufferLow = 0.30f;


const float kMaxRateMisMatch = 0.5f;


const float kRateOverShoot = 0.75f;
const float kRateUnderShoot = 0.75f;


const float kWeightRate = 0.70f;


const float kTransRateScaleUpSpatial = 1.25f;
const float kTransRateScaleUpTemp = 1.25f;
const float kTransRateScaleUpSpatialTemp = 1.25f;


const float kPacketLossThr = 0.1f;


const float kPacketLossRateFac = 1.0f;



const uint16_t kMaxRateQm[9] = {
    0,     
    50,    
    125,   
    200,   
    280,   
    400,   
    700,   
    1000,  
    1500   
};


const float kFrameRateFac[4] = {
    0.5f,    
    0.7f,    
    0.85f,   
    1.0f,    
};



const float kScaleTransRateQm[18] = {
    
    0.40f,       
    0.50f,       
    0.40f,       
    0.60f,       
    0.60f,       
    0.60f,       
    0.50f,       
    0.50f,       
    0.50f,       

    
    0.40f,       
    0.50f,       
    0.40f,       
    0.60f,       
    0.60f,       
    0.60f,       
    0.50f,       
    0.50f,       
    0.50f,       
};


const float kFacLowRate = 0.5f;




const uint8_t kSpatialAction[27] = {

    1,       
    1,       
    1,       
    4,       
    1,       
    4,       
    4,       
    1,       
    2,       


    1,       
    1,       
    1,       
    2,       
    1,       
    2,       
    2,       
    1,       
    2,       


    1,       
    1,       
    1,       
    2,       
    1,       
    2,       
    2,       
    1,       
    2,       
};

const uint8_t kTemporalAction[27] = {

    3,       
    2,       
    2,       
    1,       
    3,       
    1,       
    1,       
    2,       
    1,       


    3,       
    3,       
    3,       
    1,       
    3,       
    1,       
    1,       
    3,       
    1,       


    1,       
    3,       
    3,       
    1,       
    3,       
    1,       
    1,       
    3,       
    1,       
};


const float kMaxSpatialDown = 8.0f;
const float kMaxTempDown = 3.0f;
const float kMaxTotalDown = 9.0f;


const int kMinImageSize = 176 * 144;



const int kMinFrameRate = 8;










const int kLowFrameRate = 10;
const int kMiddleFrameRate = 15;
const int kHighFrameRate = 25;


const float kHighMotionNfd = 0.075f;
const float kLowMotionNfd = 0.03f;



const float kHighTexture = 0.035f;
const float kLowTexture = 0.020f;



const float kScaleTexture = 0.9f;


const float kRateRedSpatial2X2 = 0.6f;

const float kSpatialErr2x2VsHoriz = 0.1f;   
const float kSpatialErr2X2VsVert = 0.1f;    
const float kSpatialErrVertVsHoriz = 0.1f;  

}  

#endif  

