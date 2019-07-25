



#if !defined(nsRawStructs_h_)
#define nsRawStructs_h_

static const uint32_t RAW_ID = 0x595556;

struct nsRawVideo_PRUint24 {
  operator uint32_t() const { return value[2] << 16 | value[1] << 8 | value[0]; }
  nsRawVideo_PRUint24& operator= (const uint32_t& rhs)
  { value[2] = (rhs & 0x00FF0000) >> 16;
    value[1] = (rhs & 0x0000FF00) >> 8;
    value[0] = (rhs & 0x000000FF);
    return *this; }
private:
  uint8_t value[3];
};

struct nsRawPacketHeader {
  typedef nsRawVideo_PRUint24 PRUint24;
  uint8_t packetID;
  PRUint24 codecID;
};


struct nsRawVideoHeader {
  typedef nsRawVideo_PRUint24 PRUint24;
  uint8_t headerPacketID;          
  PRUint24 codecID;                
  uint8_t majorVersion;            
  uint8_t minorVersion;            
  uint16_t options;                
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   

  uint8_t alphaChannelBpp;
  uint8_t lumaChannelBpp;
  uint8_t chromaChannelBpp;
  uint8_t colorspace;

  PRUint24 frameWidth;
  PRUint24 frameHeight;
  PRUint24 aspectNumerator;
  PRUint24 aspectDenominator;

  uint32_t framerateNumerator;
  uint32_t framerateDenominator;
};

#endif 