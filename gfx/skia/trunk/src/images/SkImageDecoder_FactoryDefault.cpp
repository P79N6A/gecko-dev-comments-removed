







#include "SkImageDecoder.h"
#include "SkMovie.h"
#include "SkStream.h"

extern SkImageDecoder* image_decoder_from_stream(SkStreamRewindable*);

SkImageDecoder* SkImageDecoder::Factory(SkStreamRewindable* stream) {
    return image_decoder_from_stream(stream);
}



typedef SkTRegistry<SkMovie*(*)(SkStreamRewindable*)> MovieReg;

SkMovie* SkMovie::DecodeStream(SkStreamRewindable* stream) {
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
