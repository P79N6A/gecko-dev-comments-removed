









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_FILE_IMPL_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_FILE_IMPL_H_

#include "file_wrapper.h"

#include <stdio.h>

namespace webrtc {

class FileWrapperImpl : public FileWrapper
{
public:
    FileWrapperImpl();
    virtual ~FileWrapperImpl();

    virtual int FileName(char* fileNameUTF8,
                         size_t size) const;

    virtual bool Open() const;

    virtual int OpenFile(const char* fileNameUTF8,
                         bool readOnly,
                         bool loop = false,
                         bool text = false);

    virtual int CloseFile();
    virtual int SetMaxFileSize(size_t bytes);
    virtual int Flush();

    virtual int Read(void* buf, int length);
    virtual bool Write(const void *buf, int length);
    virtual int WriteText(const char* format, ...);
    virtual int Rewind();

private:
    FILE* _id;
    bool _open;
    bool _looping;
    bool _readOnly;
    size_t _maxSizeInBytes; 
    size_t _sizeInBytes;
    char _fileNameUTF8[kMaxFileNameSize];
};

} 

#endif 
