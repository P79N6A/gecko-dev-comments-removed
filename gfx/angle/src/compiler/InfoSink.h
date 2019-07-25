





#ifndef _INFOSINK_INCLUDED_
#define _INFOSINK_INCLUDED_

#include <math.h>
#include "compiler/Common.h"


inline float fractionalPart(float f) {
  float intPart = 0.0f;
  return modff(f, &intPart);
}





enum TPrefixType {
    EPrefixNone,
    EPrefixWarning,
    EPrefixError,
    EPrefixInternalError,
    EPrefixUnimplemented,
    EPrefixNote
};







class TInfoSinkBase {
public:
    TInfoSinkBase() {}

    template <typename T>
    TInfoSinkBase& operator<<(const T& t) {
        TPersistStringStream stream;
        stream << t;
        sink.append(stream.str());
        return *this;
    }
    
    
    TInfoSinkBase& operator<<(char c) {
        sink.append(1, c);
        return *this;
    }
    TInfoSinkBase& operator<<(const char* str) {
        sink.append(str);
        return *this;
    }
    TInfoSinkBase& operator<<(const TPersistString& str) {
        sink.append(str);
        return *this;
    }
    TInfoSinkBase& operator<<(const TString& str) {
        sink.append(str.c_str());
        return *this;
    }
    
    TInfoSinkBase& operator<<(float f) {
        
        
        
        
        TPersistStringStream stream;
        if (fractionalPart(f) == 0.0f) {
            stream.precision(1);
            stream << std::showpoint << std::fixed << f;
        } else {
            stream.unsetf(std::ios::fixed);
            stream.unsetf(std::ios::scientific);
            stream.precision(8);
            stream << f;
        }
        sink.append(stream.str());
        return *this;
    }
    
    TInfoSinkBase& operator<<(bool b) {
        const char* str = b ? "true" : "false";
        sink.append(str);
        return *this;
    }

    void erase() { sink.clear(); }

    const TPersistString& str() const { return sink; }
    const char* c_str() const { return sink.c_str(); }

    void prefix(TPrefixType message);
    void location(TSourceLoc loc);
    void message(TPrefixType message, const char* s);
    void message(TPrefixType message, const char* s, TSourceLoc loc);

private:
    TPersistString sink;
};

class TInfoSink {
public:
    TInfoSinkBase info;
    TInfoSinkBase debug;
    TInfoSinkBase obj;
};

#endif 
