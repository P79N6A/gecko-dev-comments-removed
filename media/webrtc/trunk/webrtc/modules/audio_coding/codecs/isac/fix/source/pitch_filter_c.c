









#include "common_audio/signal_processing/include/signal_processing_library.h"
#include "modules/audio_coding/codecs/isac/fix/source/pitch_estimator.h"


static const int16_t kDampFilter[PITCH_DAMPORDER] = {
  -2294, 8192, 20972, 8192, -2294
};

void WebRtcIsacfix_PitchFilterCore(int loopNumber,
                                   int16_t gain,
                                   int index,
                                   int16_t sign,
                                   int16_t* inputState,
                                   int16_t* outputBuf2,
                                   const int16_t* coefficient,
                                   int16_t* inputBuf,
                                   int16_t* outputBuf,
                                   int* index2) {
  int i = 0, j = 0;  
  int16_t* ubufQQpos2 = &outputBuf2[PITCH_BUFFSIZE - (index + 2)];
  int16_t tmpW16 = 0;

  for (i = 0; i < loopNumber; i++) {
    int32_t tmpW32 = 0;

    
    for (j = 0; j < PITCH_FRACORDER; j++) {
      tmpW32 += WEBRTC_SPL_MUL_16_16(ubufQQpos2[*index2 + j], coefficient[j]);
    }

    
    tmpW32 = WEBRTC_SPL_SAT(536862719, tmpW32, -536879104);
    tmpW32 += 8192;
    tmpW16 = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmpW32, 14);

    
    memmove(&inputState[1], &inputState[0],
            (PITCH_DAMPORDER - 1) * sizeof(int16_t));
    inputState[0] = (int16_t)WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(
                      gain, tmpW16, 12);

    
    tmpW32 = 0;
    

    for (j = 0; j < PITCH_DAMPORDER; j++) {
      tmpW32 += WEBRTC_SPL_MUL_16_16(inputState[j], kDampFilter[j]);
    }

    
    tmpW32 = WEBRTC_SPL_SAT(1073725439, tmpW32, -1073758208);
    tmpW32 += 16384;
    tmpW16 = (int16_t)WEBRTC_SPL_RSHIFT_W32(tmpW32, 15);

    
    tmpW32 = inputBuf[*index2] - WEBRTC_SPL_MUL_16_16(sign, tmpW16);
    outputBuf[*index2] = WebRtcSpl_SatW32ToW16(tmpW32);
    tmpW32 = inputBuf[*index2] + outputBuf[*index2];
    outputBuf2[*index2 + PITCH_BUFFSIZE] = WebRtcSpl_SatW32ToW16(tmpW32);

    (*index2)++;
  }
}

