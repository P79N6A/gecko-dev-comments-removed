




































#ifndef _MORKSINK_
#define _MORKSINK_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKBLOB_
#include "morkBlob.h"
#endif






















































class morkSink {
    

public: 

  virtual void FlushSink(morkEnv* ev) = 0;
  virtual void SpillPutc(morkEnv* ev, int c) = 0;


public: 

  mork_u1*     mSink_At;     
  mork_u1*     mSink_End;    



  
  
   

public: 

  virtual ~morkSink(); 
  morkSink() { } 

  void Putc(morkEnv* ev, int c)
  { 
    if ( mSink_At < mSink_End )
      *mSink_At++ = (mork_u1) c;
    else
      this->SpillPutc(ev, c);
  }
};











class morkSpool : public morkSink { 


public: 

  

  virtual void FlushSink(morkEnv* ev); 
  virtual void SpillPutc(morkEnv* ev, int c); 


public: 
  morkCoil*   mSpool_Coil; 
    

public: 

  static void BadSpoolCursorOrderError(morkEnv* ev);
  static void NilSpoolCoilError(morkEnv* ev);

  virtual ~morkSpool();
  
  
  
  
  morkSpool(morkEnv* ev, morkCoil* ioCoil);
  
  
  

  mork_bool Seek(morkEnv* ev, mork_pos inPos);
  
  

  mork_bool Write(morkEnv* ev, const void* inBuf, mork_size inSize);
  

  mork_bool PutBuf(morkEnv* ev, const morkBuf& inBuffer)
  { return this->Write(ev, inBuffer.mBuf_Body, inBuffer.mBuf_Fill); }
  
  mork_bool PutString(morkEnv* ev, const char* inString);
  
  
};



#endif 
