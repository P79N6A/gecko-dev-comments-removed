



























#ifndef Biquad_h
#define Biquad_h

#include <complex>

namespace WebCore {

typedef std::complex<double> Complex;






class Biquad {
public:
    Biquad();
    ~Biquad();

    void process(const float* sourceP, float* destP, size_t framesToProcess);

    
    
    void setLowpassParams(double frequency, double resonance);
    void setHighpassParams(double frequency, double resonance);
    void setBandpassParams(double frequency, double Q);
    void setLowShelfParams(double frequency, double dbGain);
    void setHighShelfParams(double frequency, double dbGain);
    void setPeakingParams(double frequency, double Q, double dbGain);
    void setAllpassParams(double frequency, double Q);
    void setNotchParams(double frequency, double Q);

    
    
    void setZeroPolePairs(const Complex& zero, const Complex& pole);

    
    
    void setAllpassPole(const Complex& pole);

    
    
    bool hasTail() const { return m_y1 || m_y2 || m_x1 || m_x2; }

    
    void reset();

    
    
    
    void getFrequencyResponse(int nFrequencies,
                              const float* frequency,
                              float* magResponse,
                              float* phaseResponse);
private:
    void setNormalizedCoefficients(double b0, double b1, double b2, double a0, double a1, double a2);

    
    
    
    double m_b0;
    double m_b1;
    double m_b2;
    double m_a1;
    double m_a2;

    
    double m_x1; 
    double m_x2; 
    double m_y1; 
    double m_y2; 
};

} 

#endif 
