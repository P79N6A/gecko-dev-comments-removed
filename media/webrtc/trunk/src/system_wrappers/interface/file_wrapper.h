









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_FILE_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_FILE_WRAPPER_H_

#include <stddef.h>

#include "common_types.h"
#include "typedefs.h"




namespace webrtc {

class FileWrapper : public InStream, public OutStream
{
public:
    static const size_t kMaxFileNameSize = 1024;

    
    static FileWrapper* Create();

    
    virtual bool Open() const = 0;

    
    virtual int OpenFile(const char* fileNameUTF8,
                         bool readOnly,
                         bool loop = false,
                         bool text = false) = 0;

    virtual int CloseFile() = 0;

    
    
    virtual int SetMaxFileSize(size_t bytes)  = 0;

    
    virtual int Flush() = 0;

    
    
    
    virtual int FileName(char* fileNameUTF8,
                         size_t size) const = 0;

    
    
    
    virtual int WriteText(const char* format, ...) = 0;

    
    
    
    virtual int Read(void* buf, int length) = 0;

    
    
    
    virtual bool Write(const void *buf, int length) = 0;

    
    
    
    virtual int Rewind() = 0;
};

} 

#endif 
