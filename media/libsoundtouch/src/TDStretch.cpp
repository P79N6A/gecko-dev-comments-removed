










































#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include "STTypes.h"
#include "cpu_detect.h"
#include "TDStretch.h"

using namespace soundtouch;

#define max(x, y) (((x) > (y)) ? (x) : (y))









static const short _scanOffsets[5][24]={
    { 124,  186,  248,  310,  372,  434,  496,  558,  620,  682,  744, 806,
      868,  930,  992, 1054, 1116, 1178, 1240, 1302, 1364, 1426, 1488,   0},
    {-100,  -75,  -50,  -25,   25,   50,   75,  100,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    { -20,  -15,  -10,   -5,    5,   10,   15,   20,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    {  -4,   -3,   -2,   -1,    1,    2,    3,    4,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    { 121,  114,   97,  114,   98,  105,  108,   32,  104,   99,  117,  111,
      116,  100,  110,  117,  111,  115,    0,    0,    0,    0,    0,   0}};








TDStretch::TDStretch() : FIFOProcessor(&outputBuffer)
{
    bQuickSeek = false;
    channels = 2;

    pMidBuffer = NULL;
    pMidBufferUnaligned = NULL;
    overlapLength = 0;

    bAutoSeqSetting = true;
    bAutoSeekSetting = true;


    skipFract = 0;

    tempo = 1.0f;
    setParameters(44100, DEFAULT_SEQUENCE_MS, DEFAULT_SEEKWINDOW_MS, DEFAULT_OVERLAP_MS);
    setTempo(1.0f);

    clear();
}



TDStretch::~TDStretch()
{
    delete[] pMidBufferUnaligned;
}












void TDStretch::setParameters(int aSampleRate, int aSequenceMS, 
                              int aSeekWindowMS, int aOverlapMS)
{
    
    if (aSampleRate > 0)   this->sampleRate = aSampleRate;
    if (aOverlapMS > 0)    this->overlapMs = aOverlapMS;

    if (aSequenceMS > 0)
    {
        this->sequenceMs = aSequenceMS;
        bAutoSeqSetting = false;
    } 
    else if (aSequenceMS == 0)
    {
        
        bAutoSeqSetting = true;
    }

    if (aSeekWindowMS > 0) 
    {
        this->seekWindowMs = aSeekWindowMS;
        bAutoSeekSetting = false;
    } 
    else if (aSeekWindowMS == 0) 
    {
        
        bAutoSeekSetting = true;
    }

    calcSeqParameters();

    calculateOverlapLength(overlapMs);

    
    setTempo(tempo);
}






void TDStretch::getParameters(int *pSampleRate, int *pSequenceMs, int *pSeekWindowMs, int *pOverlapMs) const
{
    if (pSampleRate)
    {
        *pSampleRate = sampleRate;
    }

    if (pSequenceMs)
    {
        *pSequenceMs = (bAutoSeqSetting) ? (USE_AUTO_SEQUENCE_LEN) : sequenceMs;
    }

    if (pSeekWindowMs)
    {
        *pSeekWindowMs = (bAutoSeekSetting) ? (USE_AUTO_SEEKWINDOW_LEN) : seekWindowMs;
    }

    if (pOverlapMs)
    {
        *pOverlapMs = overlapMs;
    }
}



void TDStretch::overlapMono(SAMPLETYPE *pOutput, const SAMPLETYPE *pInput) const
{
    int i;
    SAMPLETYPE m1, m2;

    m1 = (SAMPLETYPE)0;
    m2 = (SAMPLETYPE)overlapLength;

    for (i = 0; i < overlapLength ; i ++) 
    {
        pOutput[i] = (pInput[i] * m1 + pMidBuffer[i] * m2 ) / overlapLength;
        m1 += 1;
        m2 -= 1;
    }
}



void TDStretch::clearMidBuffer()
{
    memset(pMidBuffer, 0, channels * sizeof(SAMPLETYPE) * overlapLength);
}


void TDStretch::clearInput()
{
    inputBuffer.clear();
    clearMidBuffer();
}



void TDStretch::clear()
{
    outputBuffer.clear();
    clearInput();
}





void TDStretch::enableQuickSeek(bool enable)
{
    bQuickSeek = enable;
}



bool TDStretch::isQuickSeekEnabled() const
{
    return bQuickSeek;
}



int TDStretch::seekBestOverlapPosition(const SAMPLETYPE *refPos)
{
    if (bQuickSeek) 
    {
        return seekBestOverlapPositionQuick(refPos);
    } 
    else 
    {
        return seekBestOverlapPositionFull(refPos);
    }
}




inline void TDStretch::overlap(SAMPLETYPE *pOutput, const SAMPLETYPE *pInput, uint ovlPos) const
{
#ifndef USE_MULTICH_ALWAYS
    if (channels == 1)
    {
        
        overlapMono(pOutput, pInput + ovlPos);
    }
    else if (channels == 2)
    {
        
        overlapStereo(pOutput, pInput + 2 * ovlPos);
    } 
    else 
#endif 
    {
        assert(channels > 0);
        overlapMulti(pOutput, pInput + channels * ovlPos);
    }
}









int TDStretch::seekBestOverlapPositionFull(const SAMPLETYPE *refPos) 
{
    int bestOffs;
    double bestCorr;
    int i;
    double norm;

    bestCorr = FLT_MIN;
    bestOffs = 0;

    
    
    bestCorr = calcCrossCorr(refPos, pMidBuffer, norm);

    #pragma omp parallel for
    for (i = 1; i < seekLength; i ++) 
    {
        double corr;
        
#ifdef _OPENMP
        
        
        corr = calcCrossCorr(refPos + channels * i, pMidBuffer, norm);
#else
        
        
        
        corr = calcCrossCorrAccumulate(refPos + channels * i, pMidBuffer, norm);
#endif
        
        double tmp = (double)(2 * i - seekLength) / (double)seekLength;
        corr = ((corr + 0.1) * (1.0 - 0.25 * tmp * tmp));

        
        if (corr > bestCorr) 
        {
            
            
            
            #pragma omp critical
            if (corr > bestCorr)
            {
                bestCorr = corr;
                bestOffs = i;
            }
        }
    }
    
    clearCrossCorrState();

    return bestOffs;
}








int TDStretch::seekBestOverlapPositionQuick(const SAMPLETYPE *refPos) 
{
    int j;
    int bestOffs;
    double bestCorr, corr;
    int scanCount, corrOffset, tempOffset;

    bestCorr = FLT_MIN;
    bestOffs = _scanOffsets[0][0];
    corrOffset = 0;
    tempOffset = 0;

    
    
    
    
    
    
    for (scanCount = 0;scanCount < 4; scanCount ++) 
    {
        j = 0;
        while (_scanOffsets[scanCount][j]) 
        {
            double norm;
            tempOffset = corrOffset + _scanOffsets[scanCount][j];
            if (tempOffset >= seekLength) break;

            
            
            corr = (double)calcCrossCorr(refPos + channels * tempOffset, pMidBuffer, norm);
            
            double tmp = (double)(2 * tempOffset - seekLength) / seekLength;
            corr = ((corr + 0.1) * (1.0 - 0.25 * tmp * tmp));

            
            if (corr > bestCorr) 
            {
                bestCorr = corr;
                bestOffs = tempOffset;
            }
            j ++;
        }
        corrOffset = bestOffs;
    }
    
    clearCrossCorrState();

    return bestOffs;
}




void TDStretch::clearCrossCorrState()
{
    
}



void TDStretch::calcSeqParameters()
{
    
    
    #define AUTOSEQ_TEMPO_LOW   0.5     // auto setting low tempo range (-50%)
    #define AUTOSEQ_TEMPO_TOP   2.0     // auto setting top tempo range (+100%)

    
    #define AUTOSEQ_AT_MIN      125.0
    #define AUTOSEQ_AT_MAX      50.0
    #define AUTOSEQ_K           ((AUTOSEQ_AT_MAX - AUTOSEQ_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
    #define AUTOSEQ_C           (AUTOSEQ_AT_MIN - (AUTOSEQ_K) * (AUTOSEQ_TEMPO_LOW))

    
    #define AUTOSEEK_AT_MIN     25.0
    #define AUTOSEEK_AT_MAX     15.0
    #define AUTOSEEK_K          ((AUTOSEEK_AT_MAX - AUTOSEEK_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
    #define AUTOSEEK_C          (AUTOSEEK_AT_MIN - (AUTOSEEK_K) * (AUTOSEQ_TEMPO_LOW))

    #define CHECK_LIMITS(x, mi, ma) (((x) < (mi)) ? (mi) : (((x) > (ma)) ? (ma) : (x)))

    double seq, seek;
    
    if (bAutoSeqSetting)
    {
        seq = AUTOSEQ_C + AUTOSEQ_K * tempo;
        seq = CHECK_LIMITS(seq, AUTOSEQ_AT_MAX, AUTOSEQ_AT_MIN);
        sequenceMs = (int)(seq + 0.5);
    }

    if (bAutoSeekSetting)
    {
        seek = AUTOSEEK_C + AUTOSEEK_K * tempo;
        seek = CHECK_LIMITS(seek, AUTOSEEK_AT_MAX, AUTOSEEK_AT_MIN);
        seekWindowMs = (int)(seek + 0.5);
    }

    
    seekWindowLength = (sampleRate * sequenceMs) / 1000;
    if (seekWindowLength < 2 * overlapLength) 
    {
        seekWindowLength = 2 * overlapLength;
    }
    seekLength = (sampleRate * seekWindowMs) / 1000;
}





void TDStretch::setTempo(float newTempo)
{
    int intskip;

    tempo = newTempo;

    
    calcSeqParameters();

    
    nominalSkip = tempo * (seekWindowLength - overlapLength);
    intskip = (int)(nominalSkip + 0.5f);

    
    
    
    sampleReq = max(intskip + overlapLength, seekWindowLength) + seekLength;
}




void TDStretch::setChannels(int numChannels)
{
    assert(numChannels > 0);
    if (channels == numChannels) return;


    channels = numChannels;
    inputBuffer.setChannels(channels);
    outputBuffer.setChannels(channels);

    
    overlapLength=0;
    setParameters(sampleRate);
}





































void TDStretch::processSamples()
{
    int ovlSkip, offset;
    int temp;

    









    
    
    while ((int)inputBuffer.numSamples() >= sampleReq) 
    {
        
        
        offset = seekBestOverlapPosition(inputBuffer.ptrBegin());

        
        
        
        
        overlap(outputBuffer.ptrEnd((uint)overlapLength), inputBuffer.ptrBegin(), (uint)offset);
        outputBuffer.putSamples((uint)overlapLength);

        

        
        temp = (seekWindowLength - 2 * overlapLength);

        
        if ((int)inputBuffer.numSamples() < (offset + temp + overlapLength * 2))
        {
            continue;    
        }

        outputBuffer.putSamples(inputBuffer.ptrBegin() + channels * (offset + overlapLength), (uint)temp);

        
        
        
        assert((offset + temp + overlapLength * 2) <= (int)inputBuffer.numSamples());
        memcpy(pMidBuffer, inputBuffer.ptrBegin() + channels * (offset + temp + overlapLength), 
            channels * sizeof(SAMPLETYPE) * overlapLength);

        
        
        
        skipFract += nominalSkip;   
        ovlSkip = (int)skipFract;   
        skipFract -= ovlSkip;       
        inputBuffer.receiveSamples((uint)ovlSkip);
    }
}




void TDStretch::putSamples(const SAMPLETYPE *samples, uint nSamples)
{
    
    inputBuffer.putSamples(samples, nSamples);
    
    processSamples();
}




void TDStretch::acceptNewOverlapLength(int newOverlapLength)
{
    int prevOvl;

    assert(newOverlapLength >= 0);
    prevOvl = overlapLength;
    overlapLength = newOverlapLength;

    if (overlapLength > prevOvl)
    {
        delete[] pMidBufferUnaligned;

        pMidBufferUnaligned = new SAMPLETYPE[overlapLength * channels + 16 / sizeof(SAMPLETYPE)];
        
        pMidBuffer = (SAMPLETYPE *)SOUNDTOUCH_ALIGN_POINTER_16(pMidBufferUnaligned);

        clearMidBuffer();
    }
}




void * TDStretch::operator new(size_t s)
{
    
    ST_THROW_RT_ERROR("Error in TDStretch::new: Don't use 'new TDStretch' directly, use 'newInstance' member instead!");
    return newInstance();
}


TDStretch * TDStretch::newInstance()
{
#if defined(SOUNDTOUCH_ALLOW_MMX) || defined(SOUNDTOUCH_ALLOW_SSE)
    uint uExtensions;

    uExtensions = detectCPUextensions();
#endif

    

#ifdef SOUNDTOUCH_ALLOW_MMX
    
    if (uExtensions & SUPPORT_MMX)
    {
        return ::new TDStretchMMX;
    }
    else
#endif 


#ifdef SOUNDTOUCH_ALLOW_SSE
    if (uExtensions & SUPPORT_SSE)
    {
        
        return ::new TDStretchSSE;
    }
    else
#endif 

    {
        
        return ::new TDStretch;
    }
}








#ifdef SOUNDTOUCH_INTEGER_SAMPLES



void TDStretch::overlapStereo(short *poutput, const short *input) const
{
    int i;
    short temp;
    int cnt2;

    for (i = 0; i < overlapLength ; i ++) 
    {
        temp = (short)(overlapLength - i);
        cnt2 = 2 * i;
        poutput[cnt2] = (input[cnt2] * i + pMidBuffer[cnt2] * temp )  / overlapLength;
        poutput[cnt2 + 1] = (input[cnt2 + 1] * i + pMidBuffer[cnt2 + 1] * temp ) / overlapLength;
    }
}




void TDStretch::overlapMulti(SAMPLETYPE *poutput, const SAMPLETYPE *input) const
{
    SAMPLETYPE m1=(SAMPLETYPE)0;
    SAMPLETYPE m2;
    int i=0;

    for (m2 = (SAMPLETYPE)overlapLength; m2; m2 --)
    {
        for (int c = 0; c < channels; c ++)
        {
            poutput[i] = (input[i] * m1 + pMidBuffer[i] * m2)  / overlapLength;
            i++;
        }

        m1++;
    }
}


static int _getClosest2Power(double value)
{
    return (int)(log(value) / log(2.0) + 0.5);
}





void TDStretch::calculateOverlapLength(int aoverlapMs)
{
    int newOvl;

    assert(aoverlapMs >= 0);

    
    
    
    overlapDividerBits = _getClosest2Power((sampleRate * aoverlapMs) / 1000.0) - 1;
    if (overlapDividerBits > 9) overlapDividerBits = 9;
    if (overlapDividerBits < 3) overlapDividerBits = 3;
    newOvl = (int)pow(2.0, (int)overlapDividerBits + 1);    

    acceptNewOverlapLength(newOvl);

    
    
    
    slopingDivider = (newOvl * newOvl - 1) / 3;
}


double TDStretch::calcCrossCorr(const short *mixingPos, const short *compare, double &norm) const
{
    long corr;
    long lnorm;
    int i;

    corr = lnorm = 0;
    
    
    
    for (i = 0; i < channels * overlapLength; i += 4) 
    {
        corr += (mixingPos[i] * compare[i] + 
                 mixingPos[i + 1] * compare[i + 1]) >> overlapDividerBits;  
        corr += (mixingPos[i + 2] * compare[i + 2] + 
                 mixingPos[i + 3] * compare[i + 3]) >> overlapDividerBits;
        lnorm += (mixingPos[i] * mixingPos[i] + 
                  mixingPos[i + 1] * mixingPos[i + 1]) >> overlapDividerBits; 
        lnorm += (mixingPos[i + 2] * mixingPos[i + 2] + 
                  mixingPos[i + 3] * mixingPos[i + 3]) >> overlapDividerBits;
    }

    
    
    norm = (double)lnorm;
    return (double)corr / sqrt((norm < 1e-9) ? 1.0 : norm);
}



double TDStretch::calcCrossCorrAccumulate(const short *mixingPos, const short *compare, double &norm) const
{
    long corr;
    long lnorm;
    int i;

    
    lnorm = 0;
    for (i = 1; i <= channels; i ++)
    {
        lnorm -= (mixingPos[-i] * mixingPos[-i]) >> overlapDividerBits;
    }

    corr = 0;
    
    
    
    for (i = 0; i < channels * overlapLength; i += 4) 
    {
        corr += (mixingPos[i] * compare[i] + 
                 mixingPos[i + 1] * compare[i + 1]) >> overlapDividerBits;  
        corr += (mixingPos[i + 2] * compare[i + 2] + 
                 mixingPos[i + 3] * compare[i + 3]) >> overlapDividerBits;
    }

    
    for (int j = 0; j < channels; j ++)
    {
        i --;
        lnorm += (mixingPos[i] * mixingPos[i]) >> overlapDividerBits;
    }
    norm += (double)lnorm;

    
    
    return (double)corr / sqrt((norm < 1e-9) ? 1.0 : norm);
}

#endif 






#ifdef SOUNDTOUCH_FLOAT_SAMPLES


void TDStretch::overlapStereo(float *pOutput, const float *pInput) const
{
    int i;
    float fScale;
    float f1;
    float f2;

    fScale = 1.0f / (float)overlapLength;

    f1 = 0;
    f2 = 1.0f;

    for (i = 0; i < 2 * (int)overlapLength ; i += 2) 
    {
        pOutput[i + 0] = pInput[i + 0] * f1 + pMidBuffer[i + 0] * f2;
        pOutput[i + 1] = pInput[i + 1] * f1 + pMidBuffer[i + 1] * f2;

        f1 += fScale;
        f2 -= fScale;
    }
}



void TDStretch::overlapMulti(float *pOutput, const float *pInput) const
{
    int i;
    float fScale;
    float f1;
    float f2;

    fScale = 1.0f / (float)overlapLength;

    f1 = 0;
    f2 = 1.0f;

    i=0;
    for (int i2 = 0; i2 < overlapLength; i2 ++)
    {
        
        for (int c = 0; c < channels; c ++)
        {
            pOutput[i] = pInput[i] * f1 + pMidBuffer[i] * f2;
            i++;
        }
        f1 += fScale;
        f2 -= fScale;
    }
}



void TDStretch::calculateOverlapLength(int overlapInMsec)
{
    int newOvl;

    assert(overlapInMsec >= 0);
    newOvl = (sampleRate * overlapInMsec) / 1000;
    if (newOvl < 16) newOvl = 16;

    
    newOvl -= newOvl % 8;

    acceptNewOverlapLength(newOvl);
}



double TDStretch::calcCrossCorr(const float *mixingPos, const float *compare, double &anorm) const
{
    double corr;
    double norm;
    int i;

    corr = norm = 0;
    
    
    for (i = 0; i < channels * overlapLength; i += 4) 
    {
        corr += mixingPos[i] * compare[i] +
                mixingPos[i + 1] * compare[i + 1];

        norm += mixingPos[i] * mixingPos[i] + 
                mixingPos[i + 1] * mixingPos[i + 1];

        
        corr += mixingPos[i + 2] * compare[i + 2] +
                mixingPos[i + 3] * compare[i + 3];

        norm += mixingPos[i + 2] * mixingPos[i + 2] +
                mixingPos[i + 3] * mixingPos[i + 3];
    }

    anorm = norm;
    return corr / sqrt((norm < 1e-9 ? 1.0 : norm));
}



double TDStretch::calcCrossCorrAccumulate(const float *mixingPos, const float *compare, double &norm) const
{
    double corr;
    int i;

    corr = 0;

    
    for (i = 1; i <= channels; i ++)
    {
        norm -= mixingPos[-i] * mixingPos[-i];
    }

    
    
    for (i = 0; i < channels * overlapLength; i += 4) 
    {
        corr += mixingPos[i] * compare[i] +
                mixingPos[i + 1] * compare[i + 1] +
                mixingPos[i + 2] * compare[i + 2] +
                mixingPos[i + 3] * compare[i + 3];
    }

    
    for (int j = 0; j < channels; j ++)
    {
        i --;
        norm += mixingPos[i] * mixingPos[i];
    }

    return corr / sqrt((norm < 1e-9 ? 1.0 : norm));
}


#endif 
