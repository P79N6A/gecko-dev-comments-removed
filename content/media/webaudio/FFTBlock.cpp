





























#include "FFTBlock.h"

#include <complex>

namespace mozilla {

typedef std::complex<double> Complex;

FFTBlock* FFTBlock::CreateInterpolatedBlock(const FFTBlock& block0, const FFTBlock& block1, double interp)
{
    FFTBlock* newBlock = new FFTBlock(block0.FFTSize());

    newBlock->InterpolateFrequencyComponents(block0, block1, interp);

    
    int fftSize = newBlock->FFTSize();
    nsTArray<float> buffer;
    buffer.SetLength(fftSize);
    newBlock->GetInverseWithoutScaling(buffer.Elements());
    AudioBufferInPlaceScale(buffer.Elements(), 1.0f / fftSize, fftSize / 2);
    PodZero(buffer.Elements() + fftSize / 2, fftSize / 2);

    
    newBlock->PerformFFT(buffer.Elements());

    return newBlock;
}

void FFTBlock::InterpolateFrequencyComponents(const FFTBlock& block0, const FFTBlock& block1, double interp)
{
    

    kiss_fft_cpx* dft = mOutputBuffer.Elements();

    const kiss_fft_cpx* dft1 = block0.mOutputBuffer.Elements();
    const kiss_fft_cpx* dft2 = block1.mOutputBuffer.Elements();

    MOZ_ASSERT(mFFTSize == block0.FFTSize());
    MOZ_ASSERT(mFFTSize == block1.FFTSize());
    double s1base = (1.0 - interp);
    double s2base = interp;

    double phaseAccum = 0.0;
    double lastPhase1 = 0.0;
    double lastPhase2 = 0.0;

    int n = mFFTSize / 2;

    dft[0].r = static_cast<float>(s1base * dft1[0].r + s2base * dft2[0].r);
    dft[n].r = static_cast<float>(s1base * dft1[n].r + s2base * dft2[n].r);

    for (int i = 1; i < n; ++i) {
        Complex c1(dft1[i].r, dft1[i].i);
        Complex c2(dft2[i].r, dft2[i].i);

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

        
        if (deltaPhase1 > M_PI)
            deltaPhase1 -= 2.0 * M_PI;
        if (deltaPhase1 < -M_PI)
            deltaPhase1 += 2.0 * M_PI;
        if (deltaPhase2 > M_PI)
            deltaPhase2 -= 2.0 * M_PI;
        if (deltaPhase2 < -M_PI)
            deltaPhase2 += 2.0 * M_PI;

        
        double deltaPhaseBlend;

        if (deltaPhase1 - deltaPhase2 > M_PI)
            deltaPhaseBlend = s1 * deltaPhase1 + s2 * (2.0 * M_PI + deltaPhase2);
        else if (deltaPhase2 - deltaPhase1 > M_PI)
            deltaPhaseBlend = s1 * (2.0 * M_PI + deltaPhase1) + s2 * deltaPhase2;
        else
            deltaPhaseBlend = s1 * deltaPhase1 + s2 * deltaPhase2;

        phaseAccum += deltaPhaseBlend;

        
        if (phaseAccum > M_PI)
            phaseAccum -= 2.0 * M_PI;
        if (phaseAccum < -M_PI)
            phaseAccum += 2.0 * M_PI;

        dft[i].r = static_cast<float>(mag * cos(phaseAccum));
        dft[i].i = static_cast<float>(mag * sin(phaseAccum));
    }
}

double FFTBlock::ExtractAverageGroupDelay()
{
    kiss_fft_cpx* dft = mOutputBuffer.Elements();

    double aveSum = 0.0;
    double weightSum = 0.0;
    double lastPhase = 0.0;

    int halfSize = FFTSize() / 2;

    const double kSamplePhaseDelay = (2.0 * M_PI) / double(FFTSize());

    
    dft[0].r = 0.0f;

    
    for (int i = 1; i < halfSize; i++) {
        Complex c(dft[i].r, dft[i].i);
        double mag = abs(c);
        double phase = arg(c);

        double deltaPhase = phase - lastPhase;
        lastPhase = phase;

        
        if (deltaPhase < -M_PI)
            deltaPhase += 2.0 * M_PI;
        if (deltaPhase > M_PI)
            deltaPhase -= 2.0 * M_PI;

        aveSum += mag * deltaPhase;
        weightSum += mag;
    }

    
    double ave = aveSum / weightSum;
    double aveSampleDelay = -ave / kSamplePhaseDelay;

    
    aveSampleDelay -= 20.0;
    if (aveSampleDelay <= 0.0)
        return 0.0;

    
    AddConstantGroupDelay(-aveSampleDelay);

    return aveSampleDelay;
}

void FFTBlock::AddConstantGroupDelay(double sampleFrameDelay)
{
    int halfSize = FFTSize() / 2;

    kiss_fft_cpx* dft = mOutputBuffer.Elements();

    const double kSamplePhaseDelay = (2.0 * M_PI) / double(FFTSize());

    double phaseAdj = -sampleFrameDelay * kSamplePhaseDelay;

    
    for (int i = 1; i < halfSize; i++) {
        Complex c(dft[i].r, dft[i].i);
        double mag = abs(c);
        double phase = arg(c);

        phase += i * phaseAdj;

        dft[i].r = static_cast<float>(mag * cos(phase));
        dft[i].i = static_cast<float>(mag * sin(phase));
    }
}

} 
