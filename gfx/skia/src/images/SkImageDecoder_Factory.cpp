








#include "SkImageDecoder.h"
#include "SkMovie.h"
#include "SkStream.h"
#include "SkTRegistry.h"

typedef SkTRegistry<SkImageDecoder*, SkStream*> DecodeReg;



template DecodeReg* SkTRegistry<SkImageDecoder*, SkStream*>::gHead;

SkImageDecoder* SkImageDecoder::Factory(SkStream* stream) {
    SkImageDecoder* codec = NULL;
    const DecodeReg* curr = DecodeReg::Head();
    while (curr) {
        codec = curr->factory()(stream);
        
        
        bool rewindSuceeded = stream->rewind();

        
        
        if (!rewindSuceeded) {
            SkDEBUGF(("Unable to rewind the image stream."));
            SkDELETE(codec);
            return NULL;
        }

        if (codec) {
            return codec;
        }
        curr = curr->next();
    }
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
