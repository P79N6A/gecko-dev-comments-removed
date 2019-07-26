






#include "SkErrorInternals.h"
#include "SkImageDecoder.h"
#include "SkStream.h"
#include "SkTRegistry.h"





template SkImageDecoder_DecodeReg* SkImageDecoder_DecodeReg::gHead;

SkImageDecoder* image_decoder_from_stream(SkStreamRewindable*);

SkImageDecoder* image_decoder_from_stream(SkStreamRewindable* stream) {
    SkImageDecoder* codec = NULL;
    const SkImageDecoder_DecodeReg* curr = SkImageDecoder_DecodeReg::Head();
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

template SkImageDecoder_FormatReg* SkImageDecoder_FormatReg::gHead;

SkImageDecoder::Format SkImageDecoder::GetStreamFormat(SkStreamRewindable* stream) {
    const SkImageDecoder_FormatReg* curr = SkImageDecoder_FormatReg::Head();
    while (curr != NULL) {
        Format format = curr->factory()(stream);
        if (!stream->rewind()) {
            SkErrorInternals::SetError(kInvalidOperation_SkError,
                                       "Unable to rewind the image stream\n");
            return kUnknown_Format;
        }
        if (format != kUnknown_Format) {
            return format;
        }
        curr = curr->next();
    }
    return kUnknown_Format;
}
