



























#ifndef ZeroPole_h
#define ZeroPole_h

namespace WebCore {



class ZeroPole {
public:
    ZeroPole()
        : m_zero(0)
        , m_pole(0)
        , m_lastX(0)
        , m_lastY(0)
    {
    }

    void process(const float *source, float *destination, int framesToProcess);

    
    void reset() { m_lastX = 0; m_lastY = 0; }
    
    void setZero(float zero) { m_zero = zero; }
    void setPole(float pole) { m_pole = pole; }
    
    float zero() const { return m_zero; }
    float pole() const { return m_pole; }

private:
    float m_zero;
    float m_pole;
    float m_lastX;
    float m_lastY;
};

} 

#endif 
