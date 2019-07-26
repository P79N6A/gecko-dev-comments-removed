









#include "utility.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/common.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"

#define NUM_CODECS_WITH_FIXED_PAYLOAD_TYPE 13

namespace webrtc {

ACMTestTimer::ACMTestTimer()
    : _msec(0),
      _sec(0),
      _min(0),
      _hour(0) {
  return;
}

ACMTestTimer::~ACMTestTimer() {
  return;
}

void ACMTestTimer::Reset() {
  _msec = 0;
  _sec = 0;
  _min = 0;
  _hour = 0;
  return;
}
void ACMTestTimer::Tick10ms() {
  _msec += 10;
  Adjust();
  return;
}

void ACMTestTimer::Tick1ms() {
  _msec++;
  Adjust();
  return;
}

void ACMTestTimer::Tick100ms() {
  _msec += 100;
  Adjust();
  return;
}

void ACMTestTimer::Tick1sec() {
  _sec++;
  Adjust();
  return;
}

void ACMTestTimer::CurrentTimeHMS(char* currTime) {
  sprintf(currTime, "%4lu:%02u:%06.3f", _hour, _min,
          (double) _sec + (double) _msec / 1000.);
  return;
}

void ACMTestTimer::CurrentTime(unsigned long& h, unsigned char& m,
                               unsigned char& s, unsigned short& ms) {
  h = _hour;
  m = _min;
  s = _sec;
  ms = _msec;
  return;
}

void ACMTestTimer::Adjust() {
  unsigned int n;
  if (_msec >= 1000) {
    n = _msec / 1000;
    _msec -= (1000 * n);
    _sec += n;
  }
  if (_sec >= 60) {
    n = _sec / 60;
    _sec -= (n * 60);
    _min += n;
  }
  if (_min >= 60) {
    n = _min / 60;
    _min -= (n * 60);
    _hour += n;
  }
}

int16_t ChooseCodec(CodecInst& codecInst) {

  PrintCodecs();
  
  uint8_t noCodec = AudioCodingModule::NumberOfCodecs();
  int8_t codecID;
  bool outOfRange = false;
  char myStr[15] = "";
  do {
    printf("\nChoose a codec [0]: ");
    EXPECT_TRUE(fgets(myStr, 10, stdin) != NULL);
    codecID = atoi(myStr);
    if ((codecID < 0) || (codecID >= noCodec)) {
      printf("\nOut of range.\n");
      outOfRange = true;
    }
  } while (outOfRange);

  CHECK_ERROR(AudioCodingModule::Codec((uint8_t )codecID, &codecInst));
  return 0;
}

void PrintCodecs() {
  uint8_t noCodec = AudioCodingModule::NumberOfCodecs();

  CodecInst codecInst;
  printf("No  Name                [Hz]    [bps]\n");
  for (uint8_t codecCntr = 0; codecCntr < noCodec; codecCntr++) {
    AudioCodingModule::Codec(codecCntr, &codecInst);
    printf("%2d- %-18s %5d   %6d\n", codecCntr, codecInst.plname,
           codecInst.plfreq, codecInst.rate);
  }

}

CircularBuffer::CircularBuffer(uint32_t len)
    : _buff(NULL),
      _idx(0),
      _buffIsFull(false),
      _calcAvg(false),
      _calcVar(false),
      _sum(0),
      _sumSqr(0) {
  _buff = new double[len];
  if (_buff == NULL) {
    _buffLen = 0;
  } else {
    for (uint32_t n = 0; n < len; n++) {
      _buff[n] = 0;
    }
    _buffLen = len;
  }
}

CircularBuffer::~CircularBuffer() {
  if (_buff != NULL) {
    delete[] _buff;
    _buff = NULL;
  }
}

void CircularBuffer::Update(const double newVal) {
  assert(_buffLen > 0);

  
  double oldVal = _buff[_idx];
  
  _buff[_idx] = newVal;
  
  
  _idx++;
  
  
  if (_idx >= _buffLen) {
    
    _buffIsFull = true;
    _idx = 0;
  }

  

  if (_calcAvg) {
    
    
    _sum += (newVal - oldVal);
  }

  if (_calcVar) {
    
    
    _sumSqr += (double) (newVal - oldVal) * (double) (newVal + oldVal);
  }
}

void CircularBuffer::SetArithMean(bool enable) {
  assert(_buffLen > 0);

  if (enable && !_calcAvg) {
    uint32_t lim;
    if (_buffIsFull) {
      lim = _buffLen;
    } else {
      lim = _idx;
    }
    _sum = 0;
    for (uint32_t n = 0; n < lim; n++) {
      _sum += _buff[n];
    }
  }
  _calcAvg = enable;
}

void CircularBuffer::SetVariance(bool enable) {
  assert(_buffLen > 0);

  if (enable && !_calcVar) {
    uint32_t lim;
    if (_buffIsFull) {
      lim = _buffLen;
    } else {
      lim = _idx;
    }
    _sumSqr = 0;
    for (uint32_t n = 0; n < lim; n++) {
      _sumSqr += _buff[n] * _buff[n];
    }
  }
  _calcAvg = enable;
}

int16_t CircularBuffer::ArithMean(double& mean) {
  assert(_buffLen > 0);

  if (_buffIsFull) {

    mean = _sum / (double) _buffLen;
    return 0;
  } else {
    if (_idx > 0) {
      mean = _sum / (double) _idx;
      return 0;
    } else {
      return -1;
    }

  }
}

int16_t CircularBuffer::Variance(double& var) {
  assert(_buffLen > 0);

  if (_buffIsFull) {
    var = _sumSqr / (double) _buffLen;
    return 0;
  } else {
    if (_idx > 0) {
      var = _sumSqr / (double) _idx;
      return 0;
    } else {
      return -1;
    }
  }
}

bool FixedPayloadTypeCodec(const char* payloadName) {
  char fixPayloadTypeCodecs[NUM_CODECS_WITH_FIXED_PAYLOAD_TYPE][32] = { "PCMU",
      "PCMA", "GSM", "G723", "DVI4", "LPC", "PCMA", "G722", "QCELP", "CN",
      "MPA", "G728", "G729" };

  for (int n = 0; n < NUM_CODECS_WITH_FIXED_PAYLOAD_TYPE; n++) {
    if (!STR_CASE_CMP(payloadName, fixPayloadTypeCodecs[n])) {
      return true;
    }
  }
  return false;
}

DTMFDetector::DTMFDetector() {
  for (int16_t n = 0; n < 1000; n++) {
    _toneCntr[n] = 0;
  }
}

DTMFDetector::~DTMFDetector() {
}

int32_t DTMFDetector::IncomingDtmf(const uint8_t digitDtmf,
                                   const bool ) {
  fprintf(stdout, "%d-", digitDtmf);
  _toneCntr[digitDtmf]++;
  return 0;
}

void DTMFDetector::PrintDetectedDigits() {
  for (int16_t n = 0; n < 1000; n++) {
    if (_toneCntr[n] > 0) {
      fprintf(stdout, "%d %u  msec, \n", n, _toneCntr[n] * 10);
    }
  }
  fprintf(stdout, "\n");
  return;
}

void VADCallback::Reset() {
  for (int n = 0; n < 6; n++) {
    _numFrameTypes[n] = 0;
  }
}

VADCallback::VADCallback() {
  for (int n = 0; n < 6; n++) {
    _numFrameTypes[n] = 0;
  }
}

void VADCallback::PrintFrameTypes() {
  fprintf(stdout, "No encoding.................. %d\n", _numFrameTypes[0]);
  fprintf(stdout, "Active normal encoded........ %d\n", _numFrameTypes[1]);
  fprintf(stdout, "Passive normal encoded....... %d\n", _numFrameTypes[2]);
  fprintf(stdout, "Passive DTX wideband......... %d\n", _numFrameTypes[3]);
  fprintf(stdout, "Passive DTX narrowband....... %d\n", _numFrameTypes[4]);
  fprintf(stdout, "Passive DTX super-wideband... %d\n", _numFrameTypes[5]);
}

int32_t VADCallback::InFrameType(int16_t frameType) {
  _numFrameTypes[frameType]++;
  return 0;
}

void UseLegacyAcm(webrtc::Config* config) {
  config->Set<webrtc::AudioCodingModuleFactory>(
      new webrtc::AudioCodingModuleFactory());
}

void UseNewAcm(webrtc::Config* config) {
  config->Set<webrtc::AudioCodingModuleFactory>(
      new webrtc::NewAudioCodingModuleFactory());
}

}  
