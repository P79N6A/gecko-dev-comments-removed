















#ifndef __ClearKeyBase64_h__
#define __ClearKeyBase64_h__

#include <vector>
#include <string>
#include <stdint.h>





bool
DecodeBase64KeyOrId(const std::string& aEncoded, std::vector<uint8_t>& aOutDecoded);

#endif
