



























#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "Biquad.h"

#include "DenormalDisabler.h"
#include <algorithm>
#include <stdio.h>
#include <wtf/MathExtras.h>

#if OS(DARWIN)
#include <Accelerate/Accelerate.h>
#endif

namespace WebCore {

const int kBufferSize = 1024;

Biquad::Biquad()
{
#if OS(DARWIN)
    
    m_inputBuffer.allocate(kBufferSize + 2);
    m_outputBuffer.allocate(kBufferSize + 2);
#endif

#if USE(WEBAUDIO_IPP)
    int bufferSize;
    ippsIIRGetStateSize64f_BiQuad_32f(1, &bufferSize);
    m_ippInternalBuffer = ippsMalloc_8u(bufferSize);
#endif 

    
    setNormalizedCoefficients(1, 0, 0, 1, 0, 0);

    reset(); 
}

Biquad::~Biquad()
{
#if USE(WEBAUDIO_IPP)
    ippsFree(m_ippInternalBuffer);
#endif 
}

void Biquad::process(const float* sourceP, float* destP, size_t framesToProcess)
{
#if OS(DARWIN)
    
    processFast(sourceP, destP, framesToProcess);

#elif USE(WEBAUDIO_IPP)
    ippsIIR64f_32f(sourceP, destP, static_cast<int>(framesToProcess), m_biquadState);
#else 

    int n = framesToProcess;

    
    double x1 = m_x1;
    double x2 = m_x2;
    double y1 = m_y1;
    double y2 = m_y2;

    double b0 = m_b0;
    double b1 = m_b1;
    double b2 = m_b2;
    double a1 = m_a1;
    double a2 = m_a2;

    while (n--) {
        
        float x = *sourceP++;
        float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;

        *destP++ = y;

        
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
    }

    
    
    m_x1 = DenormalDisabler::flushDenormalFloatToZero(x1);
    m_x2 = DenormalDisabler::flushDenormalFloatToZero(x2);
    m_y1 = DenormalDisabler::flushDenormalFloatToZero(y1);
    m_y2 = DenormalDisabler::flushDenormalFloatToZero(y2);

    m_b0 = b0;
    m_b1 = b1;
    m_b2 = b2;
    m_a1 = a1;
    m_a2 = a2;
#endif
}

#if OS(DARWIN)



void Biquad::processFast(const float* sourceP, float* destP, size_t framesToProcess)
{
    double filterCoefficients[5];
    filterCoefficients[0] = m_b0;
    filterCoefficients[1] = m_b1;
    filterCoefficients[2] = m_b2;
    filterCoefficients[3] = m_a1;
    filterCoefficients[4] = m_a2;

    double* inputP = m_inputBuffer.data();
    double* outputP = m_outputBuffer.data();

    double* input2P = inputP + 2;
    double* output2P = outputP + 2;

    

    int n = framesToProcess;

    while (n > 0) {
        int framesThisTime = n < kBufferSize ? n : kBufferSize;

        
        for (int i = 0; i < framesThisTime; ++i)
            input2P[i] = *sourceP++;

        processSliceFast(inputP, outputP, filterCoefficients, framesThisTime);

        
        for (int i = 0; i < framesThisTime; ++i)
            *destP++ = static_cast<float>(output2P[i]);

        n -= framesThisTime;
    }
}

void Biquad::processSliceFast(double* sourceP, double* destP, double* coefficientsP, size_t framesToProcess)
{
    
    vDSP_deq22D(sourceP, 1, coefficientsP, destP, 1, framesToProcess);

    
    
    
    sourceP[0] = sourceP[framesToProcess - 2 + 2];
    sourceP[1] = sourceP[framesToProcess - 1 + 2];
    destP[0] = destP[framesToProcess - 2 + 2];
    destP[1] = destP[framesToProcess - 1 + 2];
}

#endif 


void Biquad::reset()
{
#if OS(DARWIN)
    
    double* inputP = m_inputBuffer.data();
    inputP[0] = 0;
    inputP[1] = 0;

    double* outputP = m_outputBuffer.data();
    outputP[0] = 0;
    outputP[1] = 0;

#elif USE(WEBAUDIO_IPP)
    int bufferSize;
    ippsIIRGetStateSize64f_BiQuad_32f(1, &bufferSize);
    ippsZero_8u(m_ippInternalBuffer, bufferSize);

#else
    m_x1 = m_x2 = m_y1 = m_y2 = 0;
#endif
}

void Biquad::setLowpassParams(double cutoff, double resonance)
{
    
    cutoff = std::max(0.0, std::min(cutoff, 1.0));
    
    if (cutoff == 1) {
        
        setNormalizedCoefficients(1, 0, 0,
                                  1, 0, 0);
    } else if (cutoff > 0) {
        
        resonance = std::max(0.0, resonance); 
        double g = pow(10.0, 0.05 * resonance);
        double d = sqrt((4 - sqrt(16 - 16 / (g * g))) / 2);

        double theta = piDouble * cutoff;
        double sn = 0.5 * d * sin(theta);
        double beta = 0.5 * (1 - sn) / (1 + sn);
        double gamma = (0.5 + beta) * cos(theta);
        double alpha = 0.25 * (0.5 + beta - gamma);

        double b0 = 2 * alpha;
        double b1 = 2 * 2 * alpha;
        double b2 = 2 * alpha;
        double a1 = 2 * -gamma;
        double a2 = 2 * beta;

        setNormalizedCoefficients(b0, b1, b2, 1, a1, a2);
    } else {
        
        
        setNormalizedCoefficients(0, 0, 0,
                                  1, 0, 0);
    }
}

void Biquad::setHighpassParams(double cutoff, double resonance)
{
    
    cutoff = std::max(0.0, std::min(cutoff, 1.0));

    if (cutoff == 1) {
        
        setNormalizedCoefficients(0, 0, 0,
                                  1, 0, 0);
    } else if (cutoff > 0) {
        
        resonance = std::max(0.0, resonance); 
        double g = pow(10.0, 0.05 * resonance);
        double d = sqrt((4 - sqrt(16 - 16 / (g * g))) / 2);

        double theta = piDouble * cutoff;
        double sn = 0.5 * d * sin(theta);
        double beta = 0.5 * (1 - sn) / (1 + sn);
        double gamma = (0.5 + beta) * cos(theta);
        double alpha = 0.25 * (0.5 + beta + gamma);

        double b0 = 2 * alpha;
        double b1 = 2 * -2 * alpha;
        double b2 = 2 * alpha;
        double a1 = 2 * -gamma;
        double a2 = 2 * beta;

        setNormalizedCoefficients(b0, b1, b2, 1, a1, a2);
    } else {
      
      
      
      
        setNormalizedCoefficients(1, 0, 0,
                                  1, 0, 0);
    }
}

void Biquad::setNormalizedCoefficients(double b0, double b1, double b2, double a0, double a1, double a2)
{
    double a0Inverse = 1 / a0;
    
    m_b0 = b0 * a0Inverse;
    m_b1 = b1 * a0Inverse;
    m_b2 = b2 * a0Inverse;
    m_a1 = a1 * a0Inverse;
    m_a2 = a2 * a0Inverse;

#if USE(WEBAUDIO_IPP)
    Ipp64f taps[6];
    taps[0] = m_b0;
    taps[1] = m_b1;
    taps[2] = m_b2;
    taps[3] = 1;
    taps[4] = m_a1;
    taps[5] = m_a2;
    m_biquadState = 0;

    ippsIIRInit64f_BiQuad_32f(&m_biquadState, taps, 1, 0, m_ippInternalBuffer);
#endif 
}

void Biquad::setLowShelfParams(double frequency, double dbGain)
{
    
    frequency = std::max(0.0, std::min(frequency, 1.0));
    
    double A = pow(10.0, dbGain / 40);

    if (frequency == 1) {
        
        setNormalizedCoefficients(A * A, 0, 0,
                                  1, 0, 0);
    } else if (frequency > 0) {
        double w0 = piDouble * frequency;
        double S = 1; 
        double alpha = 0.5 * sin(w0) * sqrt((A + 1 / A) * (1 / S - 1) + 2);
        double k = cos(w0);
        double k2 = 2 * sqrt(A) * alpha;
        double aPlusOne = A + 1;
        double aMinusOne = A - 1;

        double b0 = A * (aPlusOne - aMinusOne * k + k2);
        double b1 = 2 * A * (aMinusOne - aPlusOne * k);
        double b2 = A * (aPlusOne - aMinusOne * k - k2);
        double a0 = aPlusOne + aMinusOne * k + k2;
        double a1 = -2 * (aMinusOne + aPlusOne * k);
        double a2 = aPlusOne + aMinusOne * k - k2;

        setNormalizedCoefficients(b0, b1, b2, a0, a1, a2);
    } else {
        
        setNormalizedCoefficients(1, 0, 0,
                                  1, 0, 0);
    }
}

void Biquad::setHighShelfParams(double frequency, double dbGain)
{
    
    frequency = std::max(0.0, std::min(frequency, 1.0));

    double A = pow(10.0, dbGain / 40);

    if (frequency == 1) {
        
        setNormalizedCoefficients(1, 0, 0,
                                  1, 0, 0);
    } else if (frequency > 0) {
        double w0 = piDouble * frequency;
        double S = 1; 
        double alpha = 0.5 * sin(w0) * sqrt((A + 1 / A) * (1 / S - 1) + 2);
        double k = cos(w0);
        double k2 = 2 * sqrt(A) * alpha;
        double aPlusOne = A + 1;
        double aMinusOne = A - 1;

        double b0 = A * (aPlusOne + aMinusOne * k + k2);
        double b1 = -2 * A * (aMinusOne + aPlusOne * k);
        double b2 = A * (aPlusOne + aMinusOne * k - k2);
        double a0 = aPlusOne - aMinusOne * k + k2;
        double a1 = 2 * (aMinusOne - aPlusOne * k);
        double a2 = aPlusOne - aMinusOne * k - k2;

        setNormalizedCoefficients(b0, b1, b2, a0, a1, a2);
    } else {
        
        setNormalizedCoefficients(A * A, 0, 0,
                                  1, 0, 0);
    }
}

void Biquad::setPeakingParams(double frequency, double Q, double dbGain)
{
    
    frequency = std::max(0.0, std::min(frequency, 1.0));

    
    Q = std::max(0.0, Q);

    double A = pow(10.0, dbGain / 40);

    if (frequency > 0 && frequency < 1) {
        if (Q > 0) {
            double w0 = piDouble * frequency;
            double alpha = sin(w0) / (2 * Q);
            double k = cos(w0);

            double b0 = 1 + alpha * A;
            double b1 = -2 * k;
            double b2 = 1 - alpha * A;
            double a0 = 1 + alpha / A;
            double a1 = -2 * k;
            double a2 = 1 - alpha / A;

            setNormalizedCoefficients(b0, b1, b2, a0, a1, a2);
        } else {
            
            
            
            setNormalizedCoefficients(A * A, 0, 0,
                                      1, 0, 0);
        }
    } else {
        
        setNormalizedCoefficients(1, 0, 0,
                                  1, 0, 0);
    }
}

void Biquad::setAllpassParams(double frequency, double Q)
{
    
    frequency = std::max(0.0, std::min(frequency, 1.0));

    
    Q = std::max(0.0, Q);

    if (frequency > 0 && frequency < 1) {
        if (Q > 0) {
            double w0 = piDouble * frequency;
            double alpha = sin(w0) / (2 * Q);
            double k = cos(w0);

            double b0 = 1 - alpha;
            double b1 = -2 * k;
            double b2 = 1 + alpha;
            double a0 = 1 + alpha;
            double a1 = -2 * k;
            double a2 = 1 - alpha;

            setNormalizedCoefficients(b0, b1, b2, a0, a1, a2);
        } else {
            
            
            
            setNormalizedCoefficients(-1, 0, 0,
                                      1, 0, 0);
        }
    } else {
        
        setNormalizedCoefficients(1, 0, 0,
                                  1, 0, 0);
    }
}

void Biquad::setNotchParams(double frequency, double Q)
{
    
    frequency = std::max(0.0, std::min(frequency, 1.0));

    
    Q = std::max(0.0, Q);

    if (frequency > 0 && frequency < 1) {
        if (Q > 0) {
            double w0 = piDouble * frequency;
            double alpha = sin(w0) / (2 * Q);
            double k = cos(w0);

            double b0 = 1;
            double b1 = -2 * k;
            double b2 = 1;
            double a0 = 1 + alpha;
            double a1 = -2 * k;
            double a2 = 1 - alpha;

            setNormalizedCoefficients(b0, b1, b2, a0, a1, a2);
        } else {
            
            
            
            setNormalizedCoefficients(0, 0, 0,
                                      1, 0, 0);
        }
    } else {
        
        setNormalizedCoefficients(1, 0, 0,
                                  1, 0, 0);
    }
}

void Biquad::setBandpassParams(double frequency, double Q)
{
    
    frequency = std::max(0.0, frequency);

    
    Q = std::max(0.0, Q);

    if (frequency > 0 && frequency < 1) {
        double w0 = piDouble * frequency;
        if (Q > 0) {
            double alpha = sin(w0) / (2 * Q);
            double k = cos(w0);
    
            double b0 = alpha;
            double b1 = 0;
            double b2 = -alpha;
            double a0 = 1 + alpha;
            double a1 = -2 * k;
            double a2 = 1 - alpha;

            setNormalizedCoefficients(b0, b1, b2, a0, a1, a2);
        } else {
            
            
            
            setNormalizedCoefficients(1, 0, 0,
                                      1, 0, 0);
        }
    } else {
        
        
        
        
        
        setNormalizedCoefficients(0, 0, 0,
                                  1, 0, 0);
    }
}

void Biquad::setZeroPolePairs(const Complex &zero, const Complex &pole)
{
    double b0 = 1;
    double b1 = -2 * zero.real();

    double zeroMag = abs(zero);
    double b2 = zeroMag * zeroMag;

    double a1 = -2 * pole.real();

    double poleMag = abs(pole);
    double a2 = poleMag * poleMag;
    setNormalizedCoefficients(b0, b1, b2, 1, a1, a2);
}

void Biquad::setAllpassPole(const Complex &pole)
{
    Complex zero = Complex(1, 0) / pole;
    setZeroPolePairs(zero, pole);
}

void Biquad::getFrequencyResponse(int nFrequencies,
                                  const float* frequency,
                                  float* magResponse,
                                  float* phaseResponse)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    double b0 = m_b0;
    double b1 = m_b1;
    double b2 = m_b2;
    double a1 = m_a1;
    double a2 = m_a2;
    
    for (int k = 0; k < nFrequencies; ++k) {
        double omega = -piDouble * frequency[k];
        Complex z = Complex(cos(omega), sin(omega));
        Complex numerator = b0 + (b1 + b2 * z) * z;
        Complex denominator = Complex(1, 0) + (a1 + a2 * z) * z;
        Complex response = numerator / denominator;
        magResponse[k] = static_cast<float>(abs(response));
        phaseResponse[k] = static_cast<float>(atan2(imag(response), real(response)));
    }
}

} 

#endif 
