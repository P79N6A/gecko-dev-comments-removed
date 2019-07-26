




#ifndef CODEC_CONFIG_H_
#define CODEC_CONFIG_H_

#include <string>


namespace mozilla {




struct AudioCodecConfig
{
  



  int mType;
  std::string mName;
  int mFreq;
  int mPacSize;
  int mChannels;
  int mRate;

  


  explicit AudioCodecConfig(int type, std::string name,
                            int freq,int pacSize,
                            int channels, int rate): mType(type),
                                                     mName(name),
                                                     mFreq(freq),
                                                     mPacSize(pacSize),
                                                     mChannels(channels),
                                                     mRate(rate)

  {
  }
};






struct VideoCodecConfig
{

  



  int mType;
  std::string mName;
  int mWidth;
  int mHeight;

  VideoCodecConfig(int type, std::string name,int width,
                    int height): mType(type),
                                 mName(name),
                                 mWidth(width),
                                 mHeight(height)

  {
  }

};
}
#endif