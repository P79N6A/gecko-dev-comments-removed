





#ifndef vm_Compression_h
#define vm_Compression_h

#ifdef USE_ZLIB

#include <zlib.h>

#include "jstypes.h"

namespace js {

class Compressor
{
    
    static const size_t CHUNKSIZE = 2048;
    z_stream zs;
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

    Compressor(const unsigned char *inp, size_t inplen);
    ~Compressor();
    bool init();
    void setOutput(unsigned char *out, size_t outlen);
    size_t outWritten() const { return outbytes; }
    
    Status compressMore();
};





bool DecompressString(const unsigned char *inp, size_t inplen,
                      unsigned char *out, size_t outlen);

} 

#endif
#endif
