















#ifndef __AnnexB_h__
#define __AnnexB_h__

#include <cstdint>
#include <vector>

class AnnexB
{
public:
  static void ConvertFrameInPlace(std::vector<uint8_t>& aBuffer);

  static void ConvertConfig(const std::vector<uint8_t>& aBuffer,
                            std::vector<uint8_t>& aOutAnnexB);
};

#endif 
