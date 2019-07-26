















#ifndef I420_COLOR_CONVERTER_H
#define I420_COLOR_CONVERTER_H

#include <II420ColorConverter.h>



class I420ColorConverter: public II420ColorConverter {
public:
    I420ColorConverter();
    ~I420ColorConverter();

    
    bool isLoaded();
private:
    void* mHandle;
};

#endif 
