







































#include <memory.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "RateTransposer.h"
#include "InterpolateLinear.h"
#include "InterpolateCubic.h"
#include "InterpolateShannon.h"
#include "AAFilter.h"

using namespace soundtouch;


TransposerBase::ALGORITHM TransposerBase::algorithm = TransposerBase::CUBIC;



RateTransposer::RateTransposer() : FIFOProcessor(&outputBuffer)
{
    bUseAAFilter = true;

    
    pAAFilter = new AAFilter(64);
    pTransposer = TransposerBase::newInstance();
}



RateTransposer::~RateTransposer()
{
    delete pAAFilter;
    delete pTransposer;
}




void RateTransposer::enableAAFilter(bool newMode)
{
    bUseAAFilter = newMode;
}



bool RateTransposer::isAAFilterEnabled() const
{
    return bUseAAFilter;
}


AAFilter *RateTransposer::getAAFilter()
{
    return pAAFilter;
}





void RateTransposer::setRate(float newRate)
{
    double fCutoff;

    pTransposer->setRate(newRate);

    
    if (newRate > 1.0f) 
    {
        fCutoff = 0.5f / newRate;
    } 
    else 
    {
        fCutoff = 0.5f * newRate;
    }
    pAAFilter->setCutoffFreq(fCutoff);
}




void RateTransposer::putSamples(const SAMPLETYPE *samples, uint nSamples)
{
    processSamples(samples, nSamples);
}






void RateTransposer::processSamples(const SAMPLETYPE *src, uint nSamples)
{
    uint count;

    if (nSamples == 0) return;

    
    inputBuffer.putSamples(src, nSamples);

    
    
    if (bUseAAFilter == false) 
    {
        count = pTransposer->transpose(outputBuffer, inputBuffer);
        return;
    }

    assert(pAAFilter);

    
    if (pTransposer->rate < 1.0f) 
    {
        
        

        
        pTransposer->transpose(midBuffer, inputBuffer);

        
        pAAFilter->evaluate(outputBuffer, midBuffer);
    } 
    else  
    {
        
        
        

        
        pAAFilter->evaluate(midBuffer, inputBuffer);

        
        pTransposer->transpose(outputBuffer, midBuffer);
    }
}



void RateTransposer::setChannels(int nChannels)
{
    assert(nChannels > 0);

    if (pTransposer->numChannels == nChannels) return;
    pTransposer->setChannels(nChannels);

    inputBuffer.setChannels(nChannels);
    midBuffer.setChannels(nChannels);
    outputBuffer.setChannels(nChannels);
}



void RateTransposer::clear()
{
    outputBuffer.clear();
    midBuffer.clear();
    inputBuffer.clear();
}



int RateTransposer::isEmpty() const
{
    int res;

    res = FIFOProcessor::isEmpty();
    if (res == 0) return 0;
    return inputBuffer.isEmpty();
}








void TransposerBase::setAlgorithm(TransposerBase::ALGORITHM a)
{
    TransposerBase::algorithm = a;
}




int TransposerBase::transpose(FIFOSampleBuffer &dest, FIFOSampleBuffer &src)
{
    int numSrcSamples = src.numSamples();
    int sizeDemand = (int)((float)numSrcSamples / rate) + 8;
    int numOutput;
    SAMPLETYPE *psrc = src.ptrBegin();
    SAMPLETYPE *pdest = dest.ptrEnd(sizeDemand);

#ifndef USE_MULTICH_ALWAYS
    if (numChannels == 1)
    {
        numOutput = transposeMono(pdest, psrc, numSrcSamples);
    }
    else if (numChannels == 2) 
    {
        numOutput = transposeStereo(pdest, psrc, numSrcSamples);
    } 
    else 
#endif 
    {
        assert(numChannels > 0);
        numOutput = transposeMulti(pdest, psrc, numSrcSamples);
    }
    dest.putSamples(numOutput);
    src.receiveSamples(numSrcSamples);
    return numOutput;
}


TransposerBase::TransposerBase()
{
    numChannels = 0;
    rate = 1.0f;
}


TransposerBase::~TransposerBase()
{
}


void TransposerBase::setChannels(int channels)
{
    numChannels = channels;
    resetRegisters();
}


void TransposerBase::setRate(float newRate)
{
    rate = newRate;
}



TransposerBase *TransposerBase::newInstance()
{
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
    
    return ::new InterpolateLinearInteger;
#else
    switch (algorithm)
    {
        case LINEAR:
            return new InterpolateLinearFloat;

        case CUBIC:
            return new InterpolateCubic;

        case SHANNON:
            return new InterpolateShannon;

        default:
            assert(false);
            return NULL;
    }
#endif
}
