




































#ifndef _MORKSTREAM_
#define _MORKSTREAM_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKFILE_
#include "morkFile.h"
#endif






































#define morkStream_kPrintBufSize 512 /* buffer size used by printf() */ 

#define morkStream_kMinBufSize 512 /* buffer no fewer bytes */ 
#define morkStream_kMaxBufSize (32 * 1024) /* buffer no more bytes */ 

#define morkDerived_kStream 0x7A74 /* ascii 'zt' */

class morkStream  : public morkFile { 


protected: 
  mork_u1*    mStream_At;       
  mork_u1*    mStream_ReadEnd;  
  mork_u1*    mStream_WriteEnd; 

  nsIMdbFile* mStream_ContentFile;  

  mork_u1*    mStream_Buf;      
  mork_size   mStream_BufSize;  
  mork_pos    mStream_BufPos;   
  mork_bool   mStream_Dirty;    
  mork_bool   mStream_HitEof;   
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkStream(); 
  
public: 
  morkStream(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap,
      nsIMdbFile* ioContentFile, mork_size inBufSize, mork_bool inFrozen);
  void CloseStream(morkEnv* ev); 

private: 
  morkStream(const morkStream& other);
  morkStream& operator=(const morkStream& other);

public: 
  mork_bool IsStream() const
  { return IsNode() && mNode_Derived == morkDerived_kStream; }


public: 
  void NonStreamTypeError(morkEnv* ev);


public: 

  NS_IMETHOD Steal(nsIMdbEnv* ev, nsIMdbFile* ioThief);
  
  
  
  
  
  
  
  
  

  NS_IMETHOD BecomeTrunk(nsIMdbEnv* ev);
  
  
  

  NS_IMETHOD AcquireBud(nsIMdbEnv* ev, nsIMdbHeap* ioHeap, nsIMdbFile** acqBud);
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual mork_pos Length(morkEnv* ev) const; 
  NS_IMETHOD  Tell(nsIMdbEnv* ev, mork_pos *aOutPos  ) const;
  NS_IMETHOD  Read(nsIMdbEnv* ev, void* outBuf, mork_size inSize, mork_size *aOutCount);
  NS_IMETHOD  Seek(nsIMdbEnv* ev, mork_pos inPos, mork_pos *aOutPos);
  NS_IMETHOD  Write(nsIMdbEnv* ev, const void* inBuf, mork_size inSize, mork_size *aOutCount);
  NS_IMETHOD  Flush(nsIMdbEnv* ev);
    

protected: 

  int     fill_getc(morkEnv* ev);
  void    spill_putc(morkEnv* ev, int c);
  void    spill_buf(morkEnv* ev); 
      

public: 
    
  void NewBadCursorSlotsError(morkEnv* ev) const;
  void NewBadCursorOrderError(morkEnv* ev) const;
  void NewNullStreamBufferError(morkEnv* ev) const;
  void NewCantReadSinkError(morkEnv* ev) const;
  void NewCantWriteSourceError(morkEnv* ev) const;
  void NewPosBeyondEofError(morkEnv* ev) const;
      
  nsIMdbFile* GetStreamContentFile() const { return mStream_ContentFile; }
  mork_size   GetStreamBufferSize() const { return mStream_BufSize; }
  
  mork_size  PutIndent(morkEnv* ev, mork_count inDepth);
  
  
  
  mork_size  PutByteThenIndent(morkEnv* ev, int inByte, mork_count inDepth);
  
  
  
  mork_size  PutStringThenIndent(morkEnv* ev,
    const char* inString, mork_count inDepth);
  
  
  
  mork_size  PutString(morkEnv* ev, const char* inString);
  
  
  mork_size  PutStringThenNewline(morkEnv* ev, const char* inString);
  

  mork_size  PutByteThenNewline(morkEnv* ev, int inByte);
  

  
  void    Ungetc(int c) 
  { if ( mStream_At > mStream_Buf && c > 0 ) *--mStream_At = (mork_u1) c; }
  
  
  int     Getc(morkEnv* ev) 
  { return ( mStream_At < mStream_ReadEnd )? *mStream_At++ : fill_getc(ev); }
  
  void    Putc(morkEnv* ev, int c) 
  { 
    mStream_Dirty = morkBool_kTrue;
    if ( mStream_At < mStream_WriteEnd )
      *mStream_At++ = (mork_u1) c;
    else
      spill_putc(ev, c);
  }

  mork_size PutLineBreak(morkEnv* ev);
  
public: 
  static void SlotWeakStream(morkStream* me,
    morkEnv* ev, morkStream** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongStream(morkStream* me,
    morkEnv* ev, morkStream** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};




#endif 
