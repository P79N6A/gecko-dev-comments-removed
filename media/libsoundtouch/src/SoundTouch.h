






































































#ifndef SoundTouch_H
#define SoundTouch_H

#include "FIFOSamplePipe.h"
#include "STTypes.h"

namespace soundtouch
{


#define SOUNDTOUCH_VERSION          "1.9.0"


#define SOUNDTOUCH_VERSION_ID       (10900)





#define SETTING_USE_AA_FILTER       0


#define SETTING_AA_FILTER_LENGTH    1




#define SETTING_USE_QUICKSEEK       2




#define SETTING_SEQUENCE_MS         3





#define SETTING_SEEKWINDOW_MS       4





#define SETTING_OVERLAP_MS          5












#define SETTING_NOMINAL_INPUT_SEQUENCE		6












#define SETTING_NOMINAL_OUTPUT_SEQUENCE		7

class EXPORT SoundTouch : public FIFOProcessor
{
private:
    
    class RateTransposer *pRateTransposer;

    
    class TDStretch *pTDStretch;

    
    float virtualRate;

    
    float virtualTempo;

    
    float virtualPitch;

    
    bool  bSrateSet;

    
    
    void calcEffectiveRateAndTempo();

protected :
    
    uint  channels;

    
    float rate;

    
    float tempo;

public:
    SoundTouch();
    virtual ~SoundTouch();

    
    static const char *getVersionString();

    
    static uint getVersionId();

    
    
    void setRate(float newRate);

    
    
    void setTempo(float newTempo);

    
    
    void setRateChange(float newRate);

    
    
    void setTempoChange(float newTempo);

    
    
    void setPitch(float newPitch);

    
    
    void setPitchOctaves(float newPitch);

    
    
    void setPitchSemiTones(int newPitch);
    void setPitchSemiTones(float newPitch);

    
    void setChannels(uint numChannels);

    
    void setSampleRate(uint srate);

    
    
    
    
    
    
    
    void flush();

    
    
    
    virtual void putSamples(
            const SAMPLETYPE *samples,  
            uint numSamples                         
                                                    
                                                    
            );

    
    
    virtual void clear();

    
    
    
    
    bool setSetting(int settingId,   
                    int value        
                    );

    
    
    
    
    int getSetting(int settingId    
                   ) const;

    
    virtual uint numUnprocessedSamples() const;


    
    
    
    
    
    
    
    
};

}
#endif
