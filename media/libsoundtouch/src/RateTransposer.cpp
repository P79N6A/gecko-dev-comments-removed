







































#include <memory.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "RateTransposer.h"
#include "AAFilter.h"

using namespace soundtouch;




class RateTransposerInteger : public RateTransposer
{
protected:
    int iSlopeCount;
    int iRate;
    SAMPLETYPE sPrevSampleL, sPrevSampleR;

    virtual void resetRegisters();

    virtual uint transposeStereo(SAMPLETYPE *dest, 
                         const SAMPLETYPE *src, 
                         uint numSamples);
    virtual uint transposeMono(SAMPLETYPE *dest, 
                       const SAMPLETYPE *src, 
                       uint numSamples);

public:
    RateTransposerInteger();
    virtual ~RateTransposerInteger();

    
    
    virtual void setRate(float newRate);

};




class RateTransposerFloat : public RateTransposer
{
protected:
    float fSlopeCount;
    SAMPLETYPE sPrevSampleL, sPrevSampleR;

    virtual void resetRegisters();

    virtual uint transposeStereo(SAMPLETYPE *dest, 
                         const SAMPLETYPE *src, 
                         uint numSamples);
    virtual uint transposeMono(SAMPLETYPE *dest, 
                       const SAMPLETYPE *src, 
                       uint numSamples);

public:
    RateTransposerFloat();
    virtual ~RateTransposerFloat();
};






void * RateTransposer::operator new(size_t s)
{
    ST_THROW_RT_ERROR("Error in RateTransoser::new: don't use \"new TDStretch\" directly, use \"newInstance\" to create a new instance instead!");
    return newInstance();
}


RateTransposer *RateTransposer::newInstance()
{
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
    return ::new RateTransposerInteger;
#else
    return ::new RateTransposerFloat;
#endif
}



RateTransposer::RateTransposer() : FIFOProcessor(&outputBuffer)
{
    numChannels = 2;
    bUseAAFilter = true;
    fRate = 0;

    
    
    pAAFilter = new AAFilter(32);
}



RateTransposer::~RateTransposer()
{
    delete pAAFilter;
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

    fRate = newRate;

    
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





void RateTransposer::upsample(const SAMPLETYPE *src, uint nSamples)
{
    uint count, sizeTemp, num;

    
    

    
    
    sizeTemp = (uint)((float)nSamples / fRate + 16.0f);

    
    count = transpose(storeBuffer.ptrEnd(sizeTemp), src, nSamples);
    storeBuffer.putSamples(count);

    
    
    num = storeBuffer.numSamples();
    count = pAAFilter->evaluate(outputBuffer.ptrEnd(num), 
        storeBuffer.ptrBegin(), num, (uint)numChannels);
    outputBuffer.putSamples(count);

    
    storeBuffer.receiveSamples(count);
}




void RateTransposer::downsample(const SAMPLETYPE *src, uint nSamples)
{
    uint count, sizeTemp;

    
    
    

    
    storeBuffer.putSamples(src, nSamples);

    
    
    
    assert(tempBuffer.isEmpty());
    sizeTemp = storeBuffer.numSamples();

    count = pAAFilter->evaluate(tempBuffer.ptrEnd(sizeTemp), 
        storeBuffer.ptrBegin(), sizeTemp, (uint)numChannels);

	if (count == 0) return;

    
    storeBuffer.receiveSamples(count);

    
    sizeTemp = (uint)((float)nSamples / fRate + 16.0f);
    count = transpose(outputBuffer.ptrEnd(sizeTemp), tempBuffer.ptrBegin(), count);
    outputBuffer.putSamples(count);
}






void RateTransposer::processSamples(const SAMPLETYPE *src, uint nSamples)
{
    uint count;
    uint sizeReq;

    if (nSamples == 0) return;
    assert(pAAFilter);

    
    
    if (bUseAAFilter == false) 
    {
        sizeReq = (uint)((float)nSamples / fRate + 1.0f);
        count = transpose(outputBuffer.ptrEnd(sizeReq), src, nSamples);
        outputBuffer.putSamples(count);
        return;
    }

    
    if (fRate < 1.0f) 
    {
        upsample(src, nSamples);
    } 
    else  
    {
        downsample(src, nSamples);
    }
}




inline uint RateTransposer::transpose(SAMPLETYPE *dest, const SAMPLETYPE *src, uint nSamples)
{
    if (numChannels == 2) 
    {
        return transposeStereo(dest, src, nSamples);
    } 
    else 
    {
        return transposeMono(dest, src, nSamples);
    }
}



void RateTransposer::setChannels(int nChannels)
{
    assert(nChannels > 0);
    if (numChannels == nChannels) return;

    assert(nChannels == 1 || nChannels == 2);
    numChannels = nChannels;

    storeBuffer.setChannels(numChannels);
    tempBuffer.setChannels(numChannels);
    outputBuffer.setChannels(numChannels);

    
    resetRegisters();
}



void RateTransposer::clear()
{
    outputBuffer.clear();
    storeBuffer.clear();
}



int RateTransposer::isEmpty() const
{
    int res;

    res = FIFOProcessor::isEmpty();
    if (res == 0) return 0;
    return storeBuffer.isEmpty();
}








#define SCALE    65536


RateTransposerInteger::RateTransposerInteger() : RateTransposer()
{
    
    
    RateTransposerInteger::resetRegisters();
    RateTransposerInteger::setRate(1.0f);
}


RateTransposerInteger::~RateTransposerInteger()
{
}


void RateTransposerInteger::resetRegisters()
{
    iSlopeCount = 0;
    sPrevSampleL = 
    sPrevSampleR = 0;
}






uint RateTransposerInteger::transposeMono(SAMPLETYPE *dest, const SAMPLETYPE *src, uint nSamples)
{
    unsigned int i, used;
    LONG_SAMPLETYPE temp, vol1;

    if (nSamples == 0) return 0;  

	used = 0;    
    i = 0;

    
    while (iSlopeCount <= SCALE) 
    {
        vol1 = (LONG_SAMPLETYPE)(SCALE - iSlopeCount);
        temp = vol1 * sPrevSampleL + iSlopeCount * src[0];
        dest[i] = (SAMPLETYPE)(temp / SCALE);
        i++;
        iSlopeCount += iRate;
    }
    
    iSlopeCount -= SCALE;

    while (1)
    {
        while (iSlopeCount > SCALE) 
        {
            iSlopeCount -= SCALE;
            used ++;
            if (used >= nSamples - 1) goto end;
        }
        vol1 = (LONG_SAMPLETYPE)(SCALE - iSlopeCount);
        temp = src[used] * vol1 + iSlopeCount * src[used + 1];
        dest[i] = (SAMPLETYPE)(temp / SCALE);

        i++;
        iSlopeCount += iRate;
    }
end:
    
    sPrevSampleL = src[nSamples - 1];

    return i;
}





uint RateTransposerInteger::transposeStereo(SAMPLETYPE *dest, const SAMPLETYPE *src, uint nSamples)
{
    unsigned int srcPos, i, used;
    LONG_SAMPLETYPE temp, vol1;

    if (nSamples == 0) return 0;  

    used = 0;    
    i = 0;

    
    while (iSlopeCount <= SCALE) 
    {
        vol1 = (LONG_SAMPLETYPE)(SCALE - iSlopeCount);
        temp = vol1 * sPrevSampleL + iSlopeCount * src[0];
        dest[2 * i] = (SAMPLETYPE)(temp / SCALE);
        temp = vol1 * sPrevSampleR + iSlopeCount * src[1];
        dest[2 * i + 1] = (SAMPLETYPE)(temp / SCALE);
        i++;
        iSlopeCount += iRate;
    }
    
    iSlopeCount -= SCALE;

    while (1)
    {
        while (iSlopeCount > SCALE) 
        {
            iSlopeCount -= SCALE;
            used ++;
            if (used >= nSamples - 1) goto end;
        }
        srcPos = 2 * used;
        vol1 = (LONG_SAMPLETYPE)(SCALE - iSlopeCount);
        temp = src[srcPos] * vol1 + iSlopeCount * src[srcPos + 2];
        dest[2 * i] = (SAMPLETYPE)(temp / SCALE);
        temp = src[srcPos + 1] * vol1 + iSlopeCount * src[srcPos + 3];
        dest[2 * i + 1] = (SAMPLETYPE)(temp / SCALE);

        i++;
        iSlopeCount += iRate;
    }
end:
    
    sPrevSampleL = src[2 * nSamples - 2];
    sPrevSampleR = src[2 * nSamples - 1];

    return i;
}




void RateTransposerInteger::setRate(float newRate)
{
    iRate = (int)(newRate * SCALE + 0.5f);
    RateTransposer::setRate(newRate);
}









RateTransposerFloat::RateTransposerFloat() : RateTransposer()
{
    
    
    RateTransposerFloat::resetRegisters();
    RateTransposerFloat::setRate(1.0f);
}


RateTransposerFloat::~RateTransposerFloat()
{
}


void RateTransposerFloat::resetRegisters()
{
    fSlopeCount = 0;
    sPrevSampleL = 
    sPrevSampleR = 0;
}






uint RateTransposerFloat::transposeMono(SAMPLETYPE *dest, const SAMPLETYPE *src, uint nSamples)
{
    unsigned int i, used;

    used = 0;    
    i = 0;

    
    while (fSlopeCount <= 1.0f) 
    {
        dest[i] = (SAMPLETYPE)((1.0f - fSlopeCount) * sPrevSampleL + fSlopeCount * src[0]);
        i++;
        fSlopeCount += fRate;
    }
    fSlopeCount -= 1.0f;

    if (nSamples > 1)
    {
        while (1)
        {
            while (fSlopeCount > 1.0f) 
            {
                fSlopeCount -= 1.0f;
                used ++;
                if (used >= nSamples - 1) goto end;
            }
            dest[i] = (SAMPLETYPE)((1.0f - fSlopeCount) * src[used] + fSlopeCount * src[used + 1]);
            i++;
            fSlopeCount += fRate;
        }
    }
end:
    
    sPrevSampleL = src[nSamples - 1];

    return i;
}





uint RateTransposerFloat::transposeStereo(SAMPLETYPE *dest, const SAMPLETYPE *src, uint nSamples)
{
    unsigned int srcPos, i, used;

    if (nSamples == 0) return 0;  

    used = 0;    
    i = 0;

    
    while (fSlopeCount <= 1.0f) 
    {
        dest[2 * i] = (SAMPLETYPE)((1.0f - fSlopeCount) * sPrevSampleL + fSlopeCount * src[0]);
        dest[2 * i + 1] = (SAMPLETYPE)((1.0f - fSlopeCount) * sPrevSampleR + fSlopeCount * src[1]);
        i++;
        fSlopeCount += fRate;
    }
    
    fSlopeCount -= 1.0f;

    if (nSamples > 1)
    {
        while (1)
        {
            while (fSlopeCount > 1.0f) 
            {
                fSlopeCount -= 1.0f;
                used ++;
                if (used >= nSamples - 1) goto end;
            }
            srcPos = 2 * used;

            dest[2 * i] = (SAMPLETYPE)((1.0f - fSlopeCount) * src[srcPos] 
                + fSlopeCount * src[srcPos + 2]);
            dest[2 * i + 1] = (SAMPLETYPE)((1.0f - fSlopeCount) * src[srcPos + 1] 
                + fSlopeCount * src[srcPos + 3]);

            i++;
            fSlopeCount += fRate;
        }
    }
end:
    
    sPrevSampleL = src[2 * nSamples - 2];
    sPrevSampleR = src[2 * nSamples - 1];

    return i;
}
