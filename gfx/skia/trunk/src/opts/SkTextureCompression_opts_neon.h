






#ifndef SkTextureCompression_opts_neon_h_
#define SkTextureCompression_opts_neon_h_

bool CompressA8toR11EAC_NEON(uint8_t* dst, const uint8_t* src,
                             int width, int height, int rowBytes);

#endif  
