



















#ifndef AVUTIL_ADLER32_H
#define AVUTIL_ADLER32_H

#include <stdint.h>
#include "attributes.h"














unsigned long av_adler32_update(unsigned long adler, const uint8_t *buf,
                                unsigned int len) av_pure;

#endif 
