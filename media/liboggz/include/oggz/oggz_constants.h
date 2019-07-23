































#ifndef __OGGZ_CONSTANTS_H__
#define __OGGZ_CONSTANTS_H__











enum OggzFlags {
  
  OGGZ_READ         = 0x00,

  
  OGGZ_WRITE        = 0x01,

  

  OGGZ_NONSTRICT    = 0x10,

  





  OGGZ_AUTO         = 0x20,

  




  OGGZ_PREFIX       = 0x40,

  




  OGGZ_SUFFIX       = 0x80

};

enum OggzStopCtl {
  
  OGGZ_CONTINUE     = 0,

  
  OGGZ_STOP_OK      = 1,

  
  OGGZ_STOP_ERR     = -1
};




enum OggzFlushOpts {
  
  OGGZ_FLUSH_BEFORE = 0x01,

  
  OGGZ_FLUSH_AFTER  = 0x02
};




typedef enum OggzStreamContent {
  OGGZ_CONTENT_THEORA = 0,
  OGGZ_CONTENT_VORBIS,
  OGGZ_CONTENT_SPEEX,
  OGGZ_CONTENT_PCM,
  OGGZ_CONTENT_CMML,
  OGGZ_CONTENT_ANX2,
  OGGZ_CONTENT_SKELETON,
  OGGZ_CONTENT_FLAC0,
  OGGZ_CONTENT_FLAC,
  OGGZ_CONTENT_ANXDATA,
  OGGZ_CONTENT_CELT,
  OGGZ_CONTENT_KATE,
  OGGZ_CONTENT_DIRAC,
  OGGZ_CONTENT_UNKNOWN
} OggzStreamContent;




enum OggzError {
  
  OGGZ_ERR_OK                           = 0,

  
  OGGZ_ERR_GENERIC                      = -1,

  
  OGGZ_ERR_BAD_OGGZ                     = -2,

  
  OGGZ_ERR_INVALID                      = -3,

  
  OGGZ_ERR_NO_STREAMS                   = -4,

  
  OGGZ_ERR_BOS                          = -5,

  
  OGGZ_ERR_EOS                          = -6,

  
  OGGZ_ERR_BAD_METRIC                   = -7,

  
  OGGZ_ERR_SYSTEM                       = -10,

  
  OGGZ_ERR_DISABLED                     = -11,

  
  OGGZ_ERR_NOSEEK                       = -13,

  


  OGGZ_ERR_STOP_OK                      = -14,

  


  OGGZ_ERR_STOP_ERR                     = -15,

  
  OGGZ_ERR_IO_AGAIN                     = -16,

  
  OGGZ_ERR_HOLE_IN_DATA                 = -17,

  
  OGGZ_ERR_OUT_OF_MEMORY                = -18,

  
  OGGZ_ERR_BAD_SERIALNO                 = -20,

  
  OGGZ_ERR_BAD_BYTES                    = -21,

  
  OGGZ_ERR_BAD_B_O_S                    = -22,

  
  OGGZ_ERR_BAD_E_O_S                    = -23,

  
  OGGZ_ERR_BAD_GRANULEPOS               = -24,

  
  OGGZ_ERR_BAD_PACKETNO                 = -25,

  
  
  OGGZ_ERR_COMMENT_INVALID              = -129,

  
  OGGZ_ERR_BAD_GUARD                    = -210,

  

  OGGZ_ERR_RECURSIVE_WRITE              = -266
};

#endif 
