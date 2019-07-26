










































#ifndef TDStretch_H
#define TDStretch_H

#include <stddef.h>
#include "STTypes.h"
#include "RateTransposer.h"
#include "FIFOSamplePipe.h"

namespace soundtouch
{


















#define DEFAULT_SEQUENCE_MS         USE_AUTO_SEQUENCE_LEN



#define USE_AUTO_SEQUENCE_LEN       0














#define DEFAULT_SEEKWINDOW_MS       USE_AUTO_SEEKWINDOW_LEN



#define USE_AUTO_SEEKWINDOW_LEN     0









#define DEFAULT_OVERLAP_MS      8




class TDStretch : public FIFOProcessor
{
protected:
    int channels;
    int sampleReq;
    float tempo;

    SAMPLETYPE *pMidBuffer;
    SAMPLETYPE *pMidBufferUnaligned;
    int overlapLength;
    int seekLength;
    int seekWindowLength;
    int overlapDividerBits;
    int slopingDivider;
    float nominalSkip;
    float skipFract;
    FIFOSampleBuffer outputBuffer;
    FIFOSampleBuffer inputBuffer;
    bool bQuickSeek;

    int sampleRate;
    int sequenceMs;
    int seekWindowMs;
    int overlapMs;
    bool bAutoSeqSetting;
    bool bAutoSeekSetting;

    void acceptNewOverlapLength(int newOverlapLength);

    virtual void clearCrossCorrState();
    void calculateOverlapLength(int overlapMs);

    virtual double calcCrossCorr(const SAMPLETYPE *mixingPos, const SAMPLETYPE *compare) const;

    virtual int seekBestOverlapPositionFull(const SAMPLETYPE *refPos);
    virtual int seekBestOverlapPositionQuick(const SAMPLETYPE *refPos);
    int seekBestOverlapPosition(const SAMPLETYPE *refPos);

    virtual void overlapStereo(SAMPLETYPE *output, const SAMPLETYPE *input) const;
    virtual void overlapMono(SAMPLETYPE *output, const SAMPLETYPE *input) const;

    void clearMidBuffer();
    void overlap(SAMPLETYPE *output, const SAMPLETYPE *input, uint ovlPos) const;

    void calcSeqParameters();

    
    
    
    
    void processSamples();
    
public:
    TDStretch();
    virtual ~TDStretch();

    
    
    static void *operator new(size_t s);

    
    
    
    static TDStretch *newInstance();
    
    
    FIFOSamplePipe *getOutput() { return &outputBuffer; };

    
    FIFOSamplePipe *getInput() { return &inputBuffer; };

    
    
    void setTempo(float newTempo);

    
    virtual void clear();

    
    void clearInput();

    
    void setChannels(int numChannels);

    
    
    void enableQuickSeek(bool enable);

    
    bool isQuickSeekEnabled() const;

    
    
    
    
    
    
    
    
    void setParameters(int sampleRate,          
                       int sequenceMS = -1,     
                       int seekwindowMS = -1,   
                       int overlapMS = -1       
                       );

    
    
    
    void getParameters(int *pSampleRate, int *pSequenceMs, int *pSeekWindowMs, int *pOverlapMs) const;

    
    
    virtual void putSamples(
            const SAMPLETYPE *samples,  
            uint numSamples                         
                                                    
            );

    
    int getInputSampleReq() const
    {
        return (int)(nominalSkip + 0.5);
    }

    
    int getOutputBatchSize() const
    {
        return seekWindowLength - overlapLength;
    }
};





#ifdef SOUNDTOUCH_ALLOW_MMX
    
    class TDStretchMMX : public TDStretch
    {
    protected:
        double calcCrossCorr(const short *mixingPos, const short *compare) const;
        virtual void overlapStereo(short *output, const short *input) const;
        virtual void clearCrossCorrState();
    };
#endif 


#ifdef SOUNDTOUCH_ALLOW_SSE
    
    class TDStretchSSE : public TDStretch
    {
    protected:
        double calcCrossCorr(const float *mixingPos, const float *compare) const;
    };

#endif 

}
#endif  
