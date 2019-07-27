






































































#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <stdio.h>

#include "SoundTouch.h"
#include "TDStretch.h"
#include "RateTransposer.h"
#include "cpu_detect.h"

using namespace soundtouch;
    

#define TEST_FLOAT_EQUAL(a, b)  (fabs(a - b) < 1e-10)



extern "C" void soundtouch_ac_test()
{
    printf("SoundTouch Version: %s\n",SOUNDTOUCH_VERSION);
} 


SoundTouch::SoundTouch()
{
    

    pRateTransposer = new RateTransposer();
    pTDStretch = TDStretch::newInstance();

    setOutPipe(pTDStretch);

    rate = tempo = 0;

    virtualPitch = 
    virtualRate = 
    virtualTempo = 1.0;

    calcEffectiveRateAndTempo();

    channels = 0;
    bSrateSet = false;
}



SoundTouch::~SoundTouch()
{
    delete pRateTransposer;
    delete pTDStretch;
}




const char *SoundTouch::getVersionString()
{
    static const char *_version = SOUNDTOUCH_VERSION;

    return _version;
}



uint SoundTouch::getVersionId()
{
    return SOUNDTOUCH_VERSION_ID;
}



void SoundTouch::setChannels(uint numChannels)
{
    




    channels = numChannels;
    pRateTransposer->setChannels((int)numChannels);
    pTDStretch->setChannels((int)numChannels);
}





void SoundTouch::setRate(float newRate)
{
    virtualRate = newRate;
    calcEffectiveRateAndTempo();
}





void SoundTouch::setRateChange(float newRate)
{
    virtualRate = 1.0f + 0.01f * newRate;
    calcEffectiveRateAndTempo();
}





void SoundTouch::setTempo(float newTempo)
{
    virtualTempo = newTempo;
    calcEffectiveRateAndTempo();
}





void SoundTouch::setTempoChange(float newTempo)
{
    virtualTempo = 1.0f + 0.01f * newTempo;
    calcEffectiveRateAndTempo();
}





void SoundTouch::setPitch(float newPitch)
{
    virtualPitch = newPitch;
    calcEffectiveRateAndTempo();
}





void SoundTouch::setPitchOctaves(float newPitch)
{
    virtualPitch = (float)exp(0.69314718056f * newPitch);
    calcEffectiveRateAndTempo();
}





void SoundTouch::setPitchSemiTones(int newPitch)
{
    setPitchOctaves((float)newPitch / 12.0f);
}



void SoundTouch::setPitchSemiTones(float newPitch)
{
    setPitchOctaves(newPitch / 12.0f);
}




void SoundTouch::calcEffectiveRateAndTempo()
{
    float oldTempo = tempo;
    float oldRate = rate;

    tempo = virtualTempo / virtualPitch;
    rate = virtualPitch * virtualRate;

    if (!TEST_FLOAT_EQUAL(rate,oldRate)) pRateTransposer->setRate(rate);
    if (!TEST_FLOAT_EQUAL(tempo, oldTempo)) pTDStretch->setTempo(tempo);

#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
    if (rate <= 1.0f) 
    {
        if (output != pTDStretch) 
        {
            FIFOSamplePipe *tempoOut;

            assert(output == pRateTransposer);
            
            tempoOut = pTDStretch->getOutput();
            tempoOut->moveSamples(*output);
            
            

            output = pTDStretch;
        }
    }
    else
#endif
    {
        if (output != pRateTransposer) 
        {
            FIFOSamplePipe *transOut;

            assert(output == pTDStretch);
            
            transOut = pRateTransposer->getOutput();
            transOut->moveSamples(*output);
            
            pRateTransposer->moveSamples(*pTDStretch->getInput());

            output = pRateTransposer;
        }
    } 
}



void SoundTouch::setSampleRate(uint srate)
{
    bSrateSet = true;
    
    pTDStretch->setParameters((int)srate);
}




void SoundTouch::putSamples(const SAMPLETYPE *samples, uint nSamples)
{
    if (bSrateSet == false) 
    {
        ST_THROW_RT_ERROR("SoundTouch : Sample rate not defined");
    } 
    else if (channels == 0) 
    {
        ST_THROW_RT_ERROR("SoundTouch : Number of channels not defined");
    }

    
    













#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
    else if (rate <= 1.0f) 
    {
        
        assert(output == pTDStretch);
        pRateTransposer->putSamples(samples, nSamples);
        pTDStretch->moveSamples(*pRateTransposer);
    } 
    else 
#endif
    {
        
        assert(output == pRateTransposer);
        pTDStretch->putSamples(samples, nSamples);
        pRateTransposer->moveSamples(*pTDStretch);
    }
}









void SoundTouch::flush()
{
    int i;
    int nUnprocessed;
    int nOut;
    SAMPLETYPE *buff = new SAMPLETYPE[64 * channels];
    
    
    
    nUnprocessed = numUnprocessedSamples();
    nUnprocessed = (int)((double)nUnprocessed / (tempo * rate) + 0.5);

    nOut = numSamples();        
    nOut += nUnprocessed;       
    
    memset(buff, 0, 64 * channels * sizeof(SAMPLETYPE));
    
    
    
    
    for (i = 0; i < 128; i ++) 
    {
        putSamples(buff, 64);
        if ((int)numSamples() >= nOut) 
        {
            
            
            
            adjustAmountOfSamples(nOut);

            
            break;  
        }
    }

    delete[] buff;

    
    pRateTransposer->clear();
    pTDStretch->clearInput();
    
    
}




bool SoundTouch::setSetting(int settingId, int value)
{
    int sampleRate, sequenceMs, seekWindowMs, overlapMs;

    
    pTDStretch->getParameters(&sampleRate, &sequenceMs, &seekWindowMs, &overlapMs);

    switch (settingId) 
    {
        case SETTING_USE_AA_FILTER :
            
            pRateTransposer->enableAAFilter((value != 0) ? true : false);
            return true;

        case SETTING_AA_FILTER_LENGTH :
            
            pRateTransposer->getAAFilter()->setLength(value);
            return true;

        case SETTING_USE_QUICKSEEK :
            
            pTDStretch->enableQuickSeek((value != 0) ? true : false);
            return true;

        case SETTING_SEQUENCE_MS:
            
            pTDStretch->setParameters(sampleRate, value, seekWindowMs, overlapMs);
            return true;

        case SETTING_SEEKWINDOW_MS:
            
            pTDStretch->setParameters(sampleRate, sequenceMs, value, overlapMs);
            return true;

        case SETTING_OVERLAP_MS:
            
            pTDStretch->setParameters(sampleRate, sequenceMs, seekWindowMs, value);
            return true;

        default :
            return false;
    }
}






int SoundTouch::getSetting(int settingId) const
{
    int temp;

    switch (settingId) 
    {
        case SETTING_USE_AA_FILTER :
            return (uint)pRateTransposer->isAAFilterEnabled();

        case SETTING_AA_FILTER_LENGTH :
            return pRateTransposer->getAAFilter()->getLength();

        case SETTING_USE_QUICKSEEK :
            return (uint)   pTDStretch->isQuickSeekEnabled();

        case SETTING_SEQUENCE_MS:
            pTDStretch->getParameters(NULL, &temp, NULL, NULL);
            return temp;

        case SETTING_SEEKWINDOW_MS:
            pTDStretch->getParameters(NULL, NULL, &temp, NULL);
            return temp;

        case SETTING_OVERLAP_MS:
            pTDStretch->getParameters(NULL, NULL, NULL, &temp);
            return temp;

		case SETTING_NOMINAL_INPUT_SEQUENCE :
			return pTDStretch->getInputSampleReq();

		case SETTING_NOMINAL_OUTPUT_SEQUENCE :
			return pTDStretch->getOutputBatchSize();

		default :
            return 0;
    }
}




void SoundTouch::clear()
{
    pRateTransposer->clear();
    pTDStretch->clear();
}




uint SoundTouch::numUnprocessedSamples() const
{
    FIFOSamplePipe * psp;
    if (pTDStretch)
    {
        psp = pTDStretch->getInput();
        if (psp)
        {
            return psp->numSamples();
        }
    }
    return 0;
}
