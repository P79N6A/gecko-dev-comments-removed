









#ifndef WEBRTC_VOICE_ENGINE_VOE_CALL_REPORT_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_CALL_REPORT_IMPL_H

#include "voe_call_report.h"

#include "shared_data.h"


namespace webrtc
{
class FileWrapper;

class VoECallReportImpl: public VoECallReport
{
public:
    virtual int ResetCallReportStatistics(int channel);

    virtual int GetEchoMetricSummary(EchoStatistics& stats);

    virtual int GetRoundTripTimeSummary(int channel,
                                        StatVal& delaysMs);

    virtual int GetDeadOrAliveSummary(int channel, int& numOfDeadDetections,
                                      int& numOfAliveDetections);

    virtual int WriteReportToFile(const char* fileNameUTF8);

protected:
    VoECallReportImpl(voe::SharedData* shared);
    virtual ~VoECallReportImpl();

private:
    int GetDeadOrAliveSummaryInternal(int channel,
                                      int& numOfDeadDetections,
                                      int& numOfAliveDetections);

    int GetEchoMetricSummaryInternal(EchoStatistics& stats);

    int GetSpeechAndNoiseSummaryInternal(LevelStatistics& stats);

    FileWrapper& _file;
    voe::SharedData* _shared;
};

} 

#endif  
