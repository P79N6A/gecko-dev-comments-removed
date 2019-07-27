





#ifndef mozilla_image_src_imgITools_h
#define mozilla_image_src_imgITools_h

#include "imgITools.h"

#define NS_IMGTOOLS_CID \
{ /* 3d8fa16d-c9e1-4b50-bdef-2c7ae249967a */         \
     0x3d8fa16d,                                     \
     0xc9e1,                                         \
     0x4b50,                                         \
    {0xbd, 0xef, 0x2c, 0x7a, 0xe2, 0x49, 0x96, 0x7a} \
}

class imgTools final : public imgITools
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGITOOLS

  imgTools();

private:
  virtual ~imgTools();
};
#endif 
