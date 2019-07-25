
































#ifndef WEBRTC_VOICE_ENGINE_VOE_CALL_REPORT_H
#define WEBRTC_VOICE_ENGINE_VOE_CALL_REPORT_H

#include "common_types.h"

namespace webrtc {

class VoiceEngine;


class WEBRTC_DLLEXPORT VoECallReport
{
public:
    
    
    
    static VoECallReport* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    
    
    virtual int ResetCallReportStatistics(int channel) = 0;

    
    virtual int GetEchoMetricSummary(EchoStatistics& stats) = 0;

    
    
    virtual int GetRoundTripTimeSummary(int channel,
                                        StatVal& delaysMs) = 0;

    
    
    virtual int GetDeadOrAliveSummary(int channel, int& numOfDeadDetections,
                                      int& numOfAliveDetections) = 0;

    
    
    virtual int WriteReportToFile(const char* fileNameUTF8) = 0;

protected:
    VoECallReport() { }
    virtual ~VoECallReport() { }
};

}  

#endif  
