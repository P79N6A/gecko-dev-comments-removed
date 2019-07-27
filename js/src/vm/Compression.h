





#ifndef vm_Compression_h
#define vm_Compression_h

#include "mozilla/NullPtr.h"

#include "jstypes.h"

struct z_stream_s; 

namespace js {

class Compressor
{
    
    static const size_t CHUNKSIZE = 2048;
    struct z_stream_s *zs;
    const unsigned char *inp;
    size_t inplen;
    size_t outbytes;
    bool initialized;

  public:
    enum Status {
        MOREOUTPUT,
        DONE,
        CONTINUE,
        OOM
    };

    Compressor()
      : zs(nullptr),
        initialized(false)
    {}
    ~Compressor();
    






    bool prepare(const unsigned char *inp, size_t inplen);
    void setOutput(unsigned char *out, size_t outlen);
    size_t outWritten() const { return outbytes; }
    
    Status compressMore();
};





bool DecompressString(const unsigned char *inp, size_t inplen,
                      unsigned char *out, size_t outlen);

} 

#endif
