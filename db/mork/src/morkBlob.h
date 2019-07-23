




































#ifndef _MORKBLOB_
#define _MORKBLOB_ 1

#ifndef _MORK_
#include "mork.h"
#endif







class morkBuf { 
public:
  void*         mBuf_Body;  
  mork_fill     mBuf_Fill;  

public:
  morkBuf() { }
  morkBuf(const void* ioBuf, mork_fill inFill)
  : mBuf_Body((void*) ioBuf), mBuf_Fill(inFill) { }

  void ClearBufFill() { mBuf_Fill = 0; }

  static void NilBufBodyError(morkEnv* ev);

private: 
  morkBuf(const morkBuf& other);
  morkBuf& operator=(const morkBuf& other);
};







class morkBlob : public morkBuf { 

  
  
public:
  mork_size      mBlob_Size;  

public:
  morkBlob() { }
  morkBlob(const void* ioBuf, mork_fill inFill, mork_size inSize)
  : morkBuf(ioBuf, inFill), mBlob_Size(inSize) { }
 
  static void BlobFillOverSizeError(morkEnv* ev);
 
public:
  mork_bool GrowBlob(morkEnv* ev, nsIMdbHeap* ioHeap,
    mork_size inNewSize);

private: 
  morkBlob(const morkBlob& other);
  morkBlob& operator=(const morkBlob& other);
  
};








class morkText : public morkBlob { 

  
  
  

public:
  mork_cscode    mText_Form;  

  morkText() { }

private: 
  morkText(const morkText& other);
  morkText& operator=(const morkText& other);
};



























class morkCoil : public morkText { 

  
  
  
  
public:
  nsIMdbHeap*      mCoil_Heap;  

public:
  morkCoil(morkEnv* ev, nsIMdbHeap* ioHeap);
  
  void CloseCoil(morkEnv* ev);

  mork_bool GrowCoil(morkEnv* ev, mork_size inNewSize)
  { return this->GrowBlob(ev, mCoil_Heap, inNewSize); }

private: 
  morkCoil(const morkCoil& other);
  morkCoil& operator=(const morkCoil& other);
};



#endif 
