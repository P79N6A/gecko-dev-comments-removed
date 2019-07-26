



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "core/platform/audio/FFTFrame.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

#include "core/platform/Logging.h"
#include "core/platform/PlatformMemoryInstrumentation.h"
#include <wtf/Complex.h>
#include <wtf/MathExtras.h>
#include <wtf/MemoryObjectInfo.h>
#include <wtf/OwnPtr.h>

#if !USE_ACCELERATE_FFT && USE(WEBAUDIO_FFMPEG)
void reportMemoryUsage(const RDFTContext* const&, WTF::MemoryObjectInfo*);
#endif 

namespace WebCore {

void FFTFrame::doPaddedFFT(const float* data, size_t dataSize)
{
    
    AudioFloatArray paddedResponse(fftSize()); 
    paddedResponse.copyToRange(data, 0, dataSize);

    
    doFFT(paddedResponse.data());
}

PassOwnPtr<FFTFrame> FFTFrame::createInterpolatedFrame(const FFTFrame& frame1, const FFTFrame& frame2, double x)
{
    OwnPtr<FFTFrame> newFrame = adoptPtr(new FFTFrame(frame1.fftSize()));

    newFrame->interpolateFrequencyComponents(frame1, frame2, x);

    
    int fftSize = newFrame->fftSize();
    AudioFloatArray buffer(fftSize);
    newFrame->doInverseFFT(buffer.data());
    buffer.zeroRange(fftSize / 2, fftSize);

    
    newFrame->doFFT(buffer.data());

    return newFrame.release();
}

void FFTFrame::interpolateFrequencyComponents(const FFTFrame& frame1, const FFTFrame& frame2, double interp)
{
    

    float* realP = realData();
    float* imagP = imagData();

    const float* realP1 = frame1.realData();
    const float* imagP1 = frame1.imagData();
    const float* realP2 = frame2.realData();
    const float* imagP2 = frame2.imagData();

    m_FFTSize = frame1.fftSize();
    m_log2FFTSize = frame1.log2FFTSize();

    double s1base = (1.0 - interp);
    double s2base = interp;

    double phaseAccum = 0.0;
    double lastPhase1 = 0.0;
    double lastPhase2 = 0.0;

    realP[0] = static_cast<float>(s1base * realP1[0] + s2base * realP2[0]);
    imagP[0] = static_cast<float>(s1base * imagP1[0] + s2base * imagP2[0]);

    int n = m_FFTSize / 2;

    for (int i = 1; i < n; ++i) {
        Complex c1(realP1[i], imagP1[i]);
        Complex c2(realP2[i], imagP2[i]);

        double mag1 = abs(c1);
        double mag2 = abs(c2);

        
        double mag1db = 20.0 * log10(mag1);
        double mag2db = 20.0 * log10(mag2);

        double s1 = s1base;
        double s2 = s2base;

        double magdbdiff = mag1db - mag2db;

        
        double threshold =  (i > 16) ? 5.0 : 2.0;

        if (magdbdiff < -threshold && mag1db < 0.0) {
            s1 = pow(s1, 0.75);
            s2 = 1.0 - s1;
        } else if (magdbdiff > threshold && mag2db < 0.0) {
            s2 = pow(s2, 0.75);
            s1 = 1.0 - s2;
        }

        
        double magdb = s1 * mag1db + s2 * mag2db;
        double mag = pow(10.0, 0.05 * magdb);

        
        double phase1 = arg(c1);
        double phase2 = arg(c2);

        double deltaPhase1 = phase1 - lastPhase1;
        double deltaPhase2 = phase2 - lastPhase2;
        lastPhase1 = phase1;
        lastPhase2 = phase2;

        
        if (deltaPhase1 > piDouble)
            deltaPhase1 -= 2.0 * piDouble;
        if (deltaPhase1 < -piDouble)
            deltaPhase1 += 2.0 * piDouble;
        if (deltaPhase2 > piDouble)
            deltaPhase2 -= 2.0 * piDouble;
        if (deltaPhase2 < -piDouble)
            deltaPhase2 += 2.0 * piDouble;

        
        double deltaPhaseBlend;

        if (deltaPhase1 - deltaPhase2 > piDouble)
            deltaPhaseBlend = s1 * deltaPhase1 + s2 * (2.0 * piDouble + deltaPhase2);
        else if (deltaPhase2 - deltaPhase1 > piDouble)
            deltaPhaseBlend = s1 * (2.0 * piDouble + deltaPhase1) + s2 * deltaPhase2;
        else
            deltaPhaseBlend = s1 * deltaPhase1 + s2 * deltaPhase2;

        phaseAccum += deltaPhaseBlend;

        
        if (phaseAccum > piDouble)
            phaseAccum -= 2.0 * piDouble;
        if (phaseAccum < -piDouble)
            phaseAccum += 2.0 * piDouble;

        Complex c = complexFromMagnitudePhase(mag, phaseAccum);

        realP[i] = static_cast<float>(c.real());
        imagP[i] = static_cast<float>(c.imag());
    }
}

double FFTFrame::extractAverageGroupDelay()
{
    float* realP = realData();
    float* imagP = imagData();

    double aveSum = 0.0;
    double weightSum = 0.0;
    double lastPhase = 0.0;

    int halfSize = fftSize() / 2;

    const double kSamplePhaseDelay = (2.0 * piDouble) / double(fftSize());

    
    for (int i = 1; i < halfSize; i++) {
        Complex c(realP[i], imagP[i]);
        double mag = abs(c);
        double phase = arg(c);

        double deltaPhase = phase - lastPhase;
        lastPhase = phase;

        
        if (deltaPhase < -piDouble)
            deltaPhase += 2.0 * piDouble;
        if (deltaPhase > piDouble)
            deltaPhase -= 2.0 * piDouble;

        aveSum += mag * deltaPhase;
        weightSum += mag;
    }

    
    double ave = aveSum / weightSum;
    double aveSampleDelay = -ave / kSamplePhaseDelay;

    
    if (aveSampleDelay > 20.0)
        aveSampleDelay -= 20.0;

    
    addConstantGroupDelay(-aveSampleDelay);

    
    realP[0] = 0.0f;

    return aveSampleDelay;
}

void FFTFrame::addConstantGroupDelay(double sampleFrameDelay)
{
    int halfSize = fftSize() / 2;

    float* realP = realData();
    float* imagP = imagData();

    const double kSamplePhaseDelay = (2.0 * piDouble) / double(fftSize());

    double phaseAdj = -sampleFrameDelay * kSamplePhaseDelay;

    
    for (int i = 1; i < halfSize; i++) {
        Complex c(realP[i], imagP[i]);
        double mag = abs(c);
        double phase = arg(c);

        phase += i * phaseAdj;

        Complex c2 = complexFromMagnitudePhase(mag, phase);

        realP[i] = static_cast<float>(c2.real());
        imagP[i] = static_cast<float>(c2.imag());
    }
}

void FFTFrame::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, PlatformMemoryTypes::AudioSharedData);
#if USE_ACCELERATE_FFT
    info.addMember(m_frame, "frame");
    info.addMember(m_realData, "realData");
    info.addMember(m_imagData, "imagData");
#else 

#if USE(WEBAUDIO_FFMPEG)
    info.addMember(m_forwardContext, "forwardContext");
    info.addMember(m_inverseContext, "inverseContext");
    info.addMember(m_complexData, "complexData");
    info.addMember(m_realData, "realData");
    info.addMember(m_imagData, "imagData");
#endif 

#if USE(WEBAUDIO_IPP)
    int size = 0;
    ippsDFTGetBufSize_R_32f(m_DFTSpec, &size);
    info.addRawBuffer(m_buffer, size * sizeof(Ipp8u), "buffer");
    ippsDFTGetSize_R_32f(m_FFTSize, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &size, 0, 0);
    info.addRawBuffer(m_DFTSpec, size, "DFTSpec");
    info.addMember(m_complexData, "complexData");
    info.addMember(m_realData, "realData");
    info.addMember(m_imagData, "imagData");
#endif 

#endif 
}

#ifndef NDEBUG
void FFTFrame::print()
{
    FFTFrame& frame = *this;
    float* realP = frame.realData();
    float* imagP = frame.imagData();
    LOG(WebAudio, "**** \n");
    LOG(WebAudio, "DC = %f : nyquist = %f\n", realP[0], imagP[0]);

    int n = m_FFTSize / 2;

    for (int i = 1; i < n; i++) {
        double mag = sqrt(realP[i] * realP[i] + imagP[i] * imagP[i]);
        double phase = atan2(realP[i], imagP[i]);

        LOG(WebAudio, "[%d] (%f %f)\n", i, mag, phase);
    }
    LOG(WebAudio, "****\n");
}
#endif 

} 

#endif 
