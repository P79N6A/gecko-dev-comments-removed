








#include "SkImageDecoder.h"
#include "SkMovie.h"
#include "SkStream.h"
#include "SkTRegistry.h"

typedef SkTRegistry<SkImageDecoder*, SkStream*> DecodeReg;



template DecodeReg* SkTRegistry<SkImageDecoder*, SkStream*>::gHead;

#ifdef SK_ENABLE_LIBPNG
    extern SkImageDecoder* sk_libpng_dfactory(SkStream*);
#endif

SkImageDecoder* SkImageDecoder::Factory(SkStream* stream) {
    SkImageDecoder* codec = NULL;
    const DecodeReg* curr = DecodeReg::Head();
    while (curr) {
        codec = curr->factory()(stream);
        
        
        stream->rewind();
        if (codec) {
            return codec;
        }
        curr = curr->next();
    }
#ifdef SK_ENABLE_LIBPNG
    codec = sk_libpng_dfactory(stream);
    stream->rewind();
    if (codec) {
        return codec;
    }
#endif
    return NULL;
}



typedef SkTRegistry<SkMovie*, SkStream*> MovieReg;

SkMovie* SkMovie::DecodeStream(SkStream* stream) {
    const MovieReg* curr = MovieReg::Head();
    while (curr) {
        SkMovie* movie = curr->factory()(stream);
        if (movie) {
            return movie;
        }
        
        
        stream->rewind();
        curr = curr->next();
    }
    return NULL;
}
