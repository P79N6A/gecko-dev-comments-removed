






















#ifndef AVUTIL_ERROR_H
#define AVUTIL_ERROR_H

#include <errno.h>
#include <stddef.h>
#include "avutil.h"









#if EDOM > 0
#define AVERROR(e) (-(e))   ///< Returns a negative error code from a POSIX error code, to return from library functions.
#define AVUNERROR(e) (-(e)) ///< Returns a POSIX error code from a library function error return value.
#else

#define AVERROR(e) (e)
#define AVUNERROR(e) (e)
#endif

#define AVERROR_BSF_NOT_FOUND      (-0x39acbd08) ///< Bitstream filter not found
#define AVERROR_DECODER_NOT_FOUND  (-0x3cbabb08) ///< Decoder not found
#define AVERROR_DEMUXER_NOT_FOUND  (-0x32babb08) ///< Demuxer not found
#define AVERROR_ENCODER_NOT_FOUND  (-0x3cb1ba08) ///< Encoder not found
#define AVERROR_EOF                (-0x5fb9b0bb) ///< End of file
#define AVERROR_EXIT               (-0x2bb6a7bb) ///< Immediate exit was requested; the called function should not be restarted
#define AVERROR_FILTER_NOT_FOUND   (-0x33b6b908) ///< Filter not found
#define AVERROR_INVALIDDATA        (-0x3ebbb1b7) ///< Invalid data found when processing input
#define AVERROR_MUXER_NOT_FOUND    (-0x27aab208) ///< Muxer not found
#define AVERROR_OPTION_NOT_FOUND   (-0x2bafb008) ///< Option not found
#define AVERROR_PATCHWELCOME       (-0x3aa8beb0) ///< Not yet implemented in Libav, patches welcome
#define AVERROR_PROTOCOL_NOT_FOUND (-0x30adaf08) ///< Protocol not found
#define AVERROR_STREAM_NOT_FOUND   (-0x2dabac08) ///< Stream not found
#define AVERROR_BUG                (-0x5fb8aabe) ///< Bug detected, please report the issue
#define AVERROR_UNKNOWN            (-0x31b4b1ab) ///< Unknown error, typically from an external library
#define AVERROR_EXPERIMENTAL       (-0x2bb2afa8) ///< Requested feature is flagged experimental. Set strict_std_compliance if you really want to use it.













int av_strerror(int errnum, char *errbuf, size_t errbuf_size);





#endif 
