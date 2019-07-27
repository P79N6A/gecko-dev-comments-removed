






















#ifndef AVUTIL_ERROR_H
#define AVUTIL_ERROR_H

#include <errno.h>
#include "avutil.h"









#if EDOM > 0
#define AVERROR(e) (-(e))   ///< Returns a negative error code from a POSIX error code, to return from library functions.
#define AVUNERROR(e) (-(e)) ///< Returns a POSIX error code from a library function error return value.
#else

#define AVERROR(e) (e)
#define AVUNERROR(e) (e)
#endif

#define AVERROR_BSF_NOT_FOUND      (-MKTAG(0xF8,'B','S','F')) ///< Bitstream filter not found
#define AVERROR_DECODER_NOT_FOUND  (-MKTAG(0xF8,'D','E','C')) ///< Decoder not found
#define AVERROR_DEMUXER_NOT_FOUND  (-MKTAG(0xF8,'D','E','M')) ///< Demuxer not found
#define AVERROR_ENCODER_NOT_FOUND  (-MKTAG(0xF8,'E','N','C')) ///< Encoder not found
#define AVERROR_EOF                (-MKTAG( 'E','O','F',' ')) ///< End of file
#define AVERROR_EXIT               (-MKTAG( 'E','X','I','T')) ///< Immediate exit was requested; the called function should not be restarted
#define AVERROR_FILTER_NOT_FOUND   (-MKTAG(0xF8,'F','I','L')) ///< Filter not found
#define AVERROR_INVALIDDATA        (-MKTAG( 'I','N','D','A')) ///< Invalid data found when processing input
#define AVERROR_MUXER_NOT_FOUND    (-MKTAG(0xF8,'M','U','X')) ///< Muxer not found
#define AVERROR_OPTION_NOT_FOUND   (-MKTAG(0xF8,'O','P','T')) ///< Option not found
#define AVERROR_PATCHWELCOME       (-MKTAG( 'P','A','W','E')) ///< Not yet implemented in Libav, patches welcome
#define AVERROR_PROTOCOL_NOT_FOUND (-MKTAG(0xF8,'P','R','O')) ///< Protocol not found
#define AVERROR_STREAM_NOT_FOUND   (-MKTAG(0xF8,'S','T','R')) ///< Stream not found
#define AVERROR_BUG                (-MKTAG( 'B','U','G',' ')) ///< Bug detected, please report the issue
#define AVERROR_UNKNOWN            (-MKTAG( 'U','N','K','N')) ///< Unknown error, typically from an external library













int av_strerror(int errnum, char *errbuf, size_t errbuf_size);





#endif 
