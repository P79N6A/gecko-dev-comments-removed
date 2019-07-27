



#ifndef OTS_WOFF2_H_
#define OTS_WOFF2_H_

namespace ots {


size_t ComputeWOFF2FinalSize(const uint8_t *data, size_t length);




bool ConvertWOFF2ToTTF(OpenTypeFile *file, uint8_t *result, size_t result_length,
                       const uint8_t *data, size_t length);
}

#endif  
