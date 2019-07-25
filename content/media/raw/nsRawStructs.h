





































#if !defined(nsRawStructs_h_)
#define nsRawStructs_h_

static const PRUint32 RAW_ID = 0x595556;

struct nsRawVideo_PRUint24 {
  operator PRUint32() const { return value[2] << 16 | value[1] << 8 | value[0]; }
  nsRawVideo_PRUint24& operator= (const PRUint32& rhs)
  { value[2] = (rhs & 0x00FF0000) >> 16;
    value[1] = (rhs & 0x0000FF00) >> 8;
    value[0] = (rhs & 0x000000FF);
    return *this; }
private:
  PRUint8 value[3];
};

struct nsRawPacketHeader {
  typedef nsRawVideo_PRUint24 PRUint24;
  PRUint8 packetID;
  PRUint24 codecID;
};


struct nsRawVideoHeader {
  typedef nsRawVideo_PRUint24 PRUint24;
  PRUint8 headerPacketID;          
  PRUint24 codecID;                
  PRUint8 majorVersion;            
  PRUint8 minorVersion;            
  PRUint16 options;                
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   
                                   

  PRUint8 alphaChannelBpp;
  PRUint8 lumaChannelBpp;
  PRUint8 chromaChannelBpp;
  PRUint8 colorspace;

  PRUint24 frameWidth;
  PRUint24 frameHeight;
  PRUint24 aspectNumerator;
  PRUint24 aspectDenominator;

  PRUint32 framerateNumerator;
  PRUint32 framerateDenominator;
};

#endif 