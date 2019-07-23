




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKFILE_
#include "morkFile.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKSTREAM_
#include "morkStream.h"
#endif






 void
morkStream::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseStream(ev);
    this->MarkShut();
  }
}


morkStream::~morkStream() 
{
  MORK_ASSERT(mStream_ContentFile==0);
  MORK_ASSERT(mStream_Buf==0);
}


morkStream::morkStream(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioHeap,
  nsIMdbFile* ioContentFile, mork_size inBufSize, mork_bool inFrozen)
: morkFile(ev, inUsage, ioHeap, ioHeap)
, mStream_At( 0 )
, mStream_ReadEnd( 0 )
, mStream_WriteEnd( 0 )

, mStream_ContentFile( 0 )

, mStream_Buf( 0 )
, mStream_BufSize( inBufSize )
, mStream_BufPos( 0 )
, mStream_Dirty( morkBool_kFalse )
, mStream_HitEof( morkBool_kFalse )
{
  if ( ev->Good() )
  {
    if ( inBufSize < morkStream_kMinBufSize )
      mStream_BufSize = inBufSize = morkStream_kMinBufSize;
    else if ( inBufSize > morkStream_kMaxBufSize )
      mStream_BufSize = inBufSize = morkStream_kMaxBufSize;
    
    if ( ioContentFile && ioHeap )
    {
      
      
        
      nsIMdbFile_SlotStrongFile(ioContentFile, ev, &mStream_ContentFile);
      if ( ev->Good() )
      {
        mork_u1* buf = 0;
        ioHeap->Alloc(ev->AsMdbEnv(), inBufSize, (void**) &buf);
        if ( buf )
        {
          mStream_At = mStream_Buf = buf;
          
          if ( !inFrozen )
          {
            
            mStream_WriteEnd = buf + inBufSize;
          }
          else
            mStream_WriteEnd = 0; 
          
          if ( inFrozen )
          {
            
            mStream_ReadEnd = buf;
            this->SetFileFrozen(inFrozen);
          }
          else
            mStream_ReadEnd = 0; 
          
          this->SetFileActive(morkBool_kTrue);
          this->SetFileIoOpen(morkBool_kTrue);
        }
        if ( ev->Good() )
          mNode_Derived = morkDerived_kStream;
      }
    }
    else ev->NilPointerError();
  }
}

 void
morkStream::CloseStream(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      nsIMdbFile_SlotStrongFile((nsIMdbFile*) 0, ev, &mStream_ContentFile);
      nsIMdbHeap* heap = mFile_SlotHeap;
      mork_u1* buf = mStream_Buf;
      mStream_Buf = 0;
      
      if ( heap && buf )
        heap->Free(ev->AsMdbEnv(), buf);

      this->CloseFile(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}



  
#define morkStream_kSpacesPerIndent 1 /* one space per indent */
#define morkStream_kMaxIndentDepth 70 /* max indent of 70 space bytes */
static const char morkStream_kSpaces[] 
= "                                                                        ";


 
mork_size
morkStream::PutIndent(morkEnv* ev, mork_count inDepth)
  
  
{
  mork_size outLength = 0;
  nsIMdbEnv *mev = ev->AsMdbEnv();
  if ( ev->Good() )
  {
    this->PutLineBreak(ev);
    if ( ev->Good() )
    {
      outLength = inDepth;
      mdb_size bytesWritten;
      if ( inDepth )
        this->Write(mev, morkStream_kSpaces, inDepth, &bytesWritten);
    }
  }
  return outLength;
}

mork_size
morkStream::PutByteThenIndent(morkEnv* ev, int inByte, mork_count inDepth)
  
  
{
  mork_size outLength = 0;
  nsIMdbEnv *mev = ev->AsMdbEnv();
  
  if ( inDepth > morkStream_kMaxIndentDepth )
    inDepth = morkStream_kMaxIndentDepth;
  
  this->Putc(ev, inByte);
  if ( ev->Good() )
  {
    this->PutLineBreak(ev);
    if ( ev->Good() )
    {
      outLength = inDepth;
      mdb_size bytesWritten;
      if ( inDepth )
        this->Write(mev, morkStream_kSpaces, inDepth, &bytesWritten);
    }
  }
  return outLength;
}
  
mork_size
morkStream::PutStringThenIndent(morkEnv* ev,
  const char* inString, mork_count inDepth)


{
  mork_size outLength = 0;
  mdb_size bytesWritten;
  nsIMdbEnv *mev = ev->AsMdbEnv();
  
  if ( inDepth > morkStream_kMaxIndentDepth )
    inDepth = morkStream_kMaxIndentDepth;
  
  if ( inString )
  {
    mork_size length = MORK_STRLEN(inString);
    if ( length && ev->Good() ) 
      this->Write(mev, inString, length, &bytesWritten);
  }
  
  if ( ev->Good() )
  {
    this->PutLineBreak(ev);
    if ( ev->Good() )
    {
      outLength = inDepth;
      if ( inDepth )
        this->Write(mev, morkStream_kSpaces, inDepth, &bytesWritten);
    }
  }
  return outLength;
}

mork_size
morkStream::PutString(morkEnv* ev, const char* inString)
{
  nsIMdbEnv *mev = ev->AsMdbEnv();
  mork_size outSize = 0;
  mdb_size bytesWritten;
  if ( inString )
  {
    outSize = MORK_STRLEN(inString);
    if ( outSize && ev->Good() ) 
    {
      this->Write(mev, inString, outSize, &bytesWritten);
    }
  }
  return outSize;
}

mork_size
morkStream::PutStringThenNewline(morkEnv* ev, const char* inString)
  
{
  nsIMdbEnv *mev = ev->AsMdbEnv();
  mork_size outSize = 0;
  mdb_size bytesWritten;
  if ( inString )
  {
    outSize = MORK_STRLEN(inString);
    if ( outSize && ev->Good() ) 
    {
      this->Write(mev, inString, outSize, &bytesWritten);
      if ( ev->Good() )
        outSize += this->PutLineBreak(ev);
    }
  }
  return outSize;
}

mork_size
morkStream::PutByteThenNewline(morkEnv* ev, int inByte)
  
{
  mork_size outSize = 1; 
  this->Putc(ev, inByte);
  if ( ev->Good() )
    outSize += this->PutLineBreak(ev);
  return outSize;
}

mork_size
morkStream::PutLineBreak(morkEnv* ev)
{
#if defined(MORK_MAC)

  this->Putc(ev, mork_kCR);
  return 1;
  
#else
#  if defined(MORK_WIN) || defined(MORK_OS2)
  
  this->Putc(ev, mork_kCR);
  this->Putc(ev, mork_kLF);
  return 2;
  
#  else
#    if defined(MORK_UNIX) || defined(MORK_BEOS)
  
  this->Putc(ev, mork_kLF);
  return 1;
  
#    endif 
#  endif 
#endif 
}




NS_IMETHODIMP
morkStream::Steal(nsIMdbEnv* mev, nsIMdbFile* ioThief)
  
  
  
  
  
  
  
  
  
{
  MORK_USED_1(ioThief);
  morkEnv *ev = morkEnv::FromMdbEnv(mev);
  ev->StubMethodOnlyError();
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkStream::BecomeTrunk(nsIMdbEnv* mev)
  
  
  
{
  morkEnv *ev = morkEnv::FromMdbEnv(mev);
  ev->StubMethodOnlyError();
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkStream::AcquireBud(nsIMdbEnv* mev, nsIMdbHeap* ioHeap, nsIMdbFile **acqBud)
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  MORK_USED_1(ioHeap);
  morkFile* outFile = 0;
  nsIMdbFile* file = mStream_ContentFile;
  morkEnv *ev = morkEnv::FromMdbEnv(mev);
  if ( this->IsOpenAndActiveFile() && file )
  {
    
    ev->StubMethodOnlyError();
  }
  else this->NewFileDownError(ev);
  
  *acqBud = outFile;
  return NS_ERROR_NOT_IMPLEMENTED;
}

mork_pos 
morkStream::Length(morkEnv* ev) const 
{
  mork_pos outPos = 0;

  nsIMdbFile* file = mStream_ContentFile;
  if ( this->IsOpenAndActiveFile() && file )
  {
    mork_pos contentEof = 0;
    file->Eof(ev->AsMdbEnv(), &contentEof);
    if ( ev->Good() )
    {
      if ( mStream_WriteEnd ) 
      {
        
        if ( ev->Good() ) 
        {
          mork_u1* at = mStream_At;
          mork_u1* buf = mStream_Buf;
          if ( at >= buf ) 
          {
            mork_pos localContent = mStream_BufPos + (at - buf);
            if ( localContent > contentEof ) 
              contentEof = localContent; 

            outPos = contentEof;
          }
          else this->NewBadCursorOrderError(ev);
        }
      }
      else
        outPos = contentEof; 
    }
  }
  else this->NewFileDownError(ev);

  return outPos;
}

void morkStream::NewBadCursorSlotsError(morkEnv* ev) const
{ ev->NewError("bad stream cursor slots"); }

void morkStream::NewNullStreamBufferError(morkEnv* ev) const
{ ev->NewError("null stream buffer"); }

void morkStream::NewCantReadSinkError(morkEnv* ev) const
{ ev->NewError("cant read stream sink"); }

void morkStream::NewCantWriteSourceError(morkEnv* ev) const
{ ev->NewError("cant write stream source"); }

void morkStream::NewPosBeyondEofError(morkEnv* ev) const
{ ev->NewError("stream pos beyond eof"); }

void morkStream::NewBadCursorOrderError(morkEnv* ev) const
{ ev->NewError("bad stream cursor order"); }

NS_IMETHODIMP 
morkStream::Tell(nsIMdbEnv* mdbev, mork_pos *aOutPos) const
{
  nsresult rv = NS_OK;
  morkEnv *ev = morkEnv::FromMdbEnv(mdbev);

  NS_ENSURE_ARG_POINTER(aOutPos);
  
  nsIMdbFile* file = mStream_ContentFile;
  if ( this->IsOpenAndActiveFile() && file )
  {
    mork_u1* buf = mStream_Buf;
    mork_u1* at = mStream_At;
    
    mork_u1* readEnd = mStream_ReadEnd;   
    mork_u1* writeEnd = mStream_WriteEnd; 
    
    if ( writeEnd )
    {
      if ( buf && at >= buf && at <= writeEnd ) 
      {
        *aOutPos = mStream_BufPos + (at - buf);
      }
      else this->NewBadCursorOrderError(ev);
    }
    else if ( readEnd )
    {
      if ( buf && at >= buf && at <= readEnd ) 
      {
        *aOutPos = mStream_BufPos + (at - buf);
      }
      else this->NewBadCursorOrderError(ev);
    }
  }
  else this->NewFileDownError(ev);

  return rv;
}

NS_IMETHODIMP 
morkStream::Read(nsIMdbEnv* mdbev, void* outBuf, mork_size inSize, mork_size *aOutSize)
{
  NS_ENSURE_ARG_POINTER(aOutSize);
  
  
  
  

  morkEnv *ev = morkEnv::FromMdbEnv(mdbev);
  nsresult rv = NS_OK;

  nsIMdbFile* file = mStream_ContentFile;
  if ( this->IsOpenAndActiveFile() && file )
  {
    mork_u1* end = mStream_ReadEnd; 
    if ( end ) 
    {
      if ( inSize ) 
      {
        mork_u1* sink = (mork_u1*) outBuf; 
        if ( sink ) 
        {
          mork_u1* at = mStream_At;
          mork_u1* buf = mStream_Buf;
          if ( at >= buf && at <= end ) 
          {
            mork_num remaining = (mork_num) (end - at); 
            
            mork_num quantum = inSize; 
            if ( quantum > remaining ) 
              quantum = remaining; 
              
            if ( quantum ) 
            {
              MORK_MEMCPY(sink, at, quantum); 
              
              at += quantum; 
              mStream_At = at;
              *aOutSize += quantum;  

              sink += quantum;   
              inSize -= quantum; 
              mStream_HitEof = morkBool_kFalse;
            }
            
            if ( inSize ) 
            {
              
              
              
              
              
              mork_num posDelta = (mork_num) (at - buf); 
              mStream_BufPos += posDelta;   
              
              mStream_At = mStream_ReadEnd = buf; 
              
              
              
              
              
              
              mork_num actual = 0;
              nsIMdbEnv* menv = ev->AsMdbEnv();
              file->Get(menv, sink, inSize, mStream_BufPos, &actual);
              if ( ev->Good() ) 
              {
                if ( actual )
                {
                  *aOutSize += actual;
                  mStream_BufPos += actual;
                  mStream_HitEof = morkBool_kFalse;
                }
                else if ( !*aOutSize )
                  mStream_HitEof = morkBool_kTrue;
              }
            }
          }
          else this->NewBadCursorOrderError(ev);
        }
        else this->NewNullStreamBufferError(ev);
      }
    }
    else this->NewCantReadSinkError(ev);
  }
  else this->NewFileDownError(ev);
  
  if ( ev->Bad() )
    *aOutSize = 0;

  return rv;
}

NS_IMETHODIMP 
morkStream::Seek(nsIMdbEnv * mdbev, mork_pos inPos, mork_pos *aOutPos)
{
  NS_ENSURE_ARG_POINTER(aOutPos);
  morkEnv *ev = morkEnv::FromMdbEnv(mdbev);
  *aOutPos = 0;
  nsresult rv = NS_OK;
  nsIMdbFile* file = mStream_ContentFile;
  if ( this->IsOpenOrClosingNode() && this->FileActive() && file )
  {
    mork_u1* at = mStream_At;             
    mork_u1* buf = mStream_Buf;           
    mork_u1* readEnd = mStream_ReadEnd;   
    mork_u1* writeEnd = mStream_WriteEnd; 
    
    if ( writeEnd ) 
    {
      if ( mStream_Dirty ) 
        this->Flush(mdbev);

      if ( ev->Good() ) 
      {
        if ( at == buf ) 
        {
          if ( mStream_BufPos != inPos ) 
          {
            mork_pos eof = 0;
            nsIMdbEnv* menv = ev->AsMdbEnv();
            file->Eof(menv, &eof);
            if ( ev->Good() ) 
            {
              if ( inPos <= eof ) 
              {
                mStream_BufPos = inPos; 
                *aOutPos = inPos;
              }
              else this->NewPosBeyondEofError(ev);
            }
          }
        }
        else this->NewBadCursorOrderError(ev);
      }
    }
    else if ( readEnd ) 
    {
      if ( at >= buf && at <= readEnd ) 
      {
        mork_pos eof = 0;
        nsIMdbEnv* menv = ev->AsMdbEnv();
        file->Eof(menv, &eof);
        if ( ev->Good() ) 
        {
          if ( inPos <= eof ) 
          {
            *aOutPos = inPos;
            mStream_BufPos = inPos; 
            mStream_At = mStream_ReadEnd = buf; 
            if ( inPos == eof ) 
              mStream_HitEof = morkBool_kTrue;
          }
          else this->NewPosBeyondEofError(ev);
        }
      }
      else this->NewBadCursorOrderError(ev);
    }
      
  }
  else this->NewFileDownError(ev);

  return rv;
}

NS_IMETHODIMP 
morkStream::Write(nsIMdbEnv* menv, const void* inBuf, mork_size inSize, mork_size  *aOutSize)
{
  mork_num outActual = 0;
  morkEnv *ev = morkEnv::FromMdbEnv(menv);

  nsIMdbFile* file = mStream_ContentFile;
  if ( this->IsOpenActiveAndMutableFile() && file )
  {
    mork_u1* end = mStream_WriteEnd; 
    if ( end ) 
    {
      if ( inSize ) 
      {
        const mork_u1* source = (const mork_u1*) inBuf; 
        if ( source ) 
        {
          mork_u1* at = mStream_At;
          mork_u1* buf = mStream_Buf;
          if ( at >= buf && at <= end ) 
          {
            mork_num space = (mork_num) (end - at); 
            
            mork_num quantum = inSize; 
            if ( quantum > space ) 
              quantum = space; 
              
            if ( quantum ) 
            {
              mStream_Dirty = morkBool_kTrue; 
              MORK_MEMCPY(at, source, quantum); 
              
              mStream_At += quantum; 
              outActual += quantum;  

              source += quantum; 
              inSize -= quantum; 
            }
            
            if ( inSize ) 
            {
              
              
              
              
              
              
              
              
              if ( mStream_Dirty )
                this->Flush(menv); 

              at = mStream_At;
              if ( at < buf || at > end ) 
                this->NewBadCursorOrderError(ev);
                
              if ( ev->Good() ) 
              {
                space = (mork_num) (end - at); 
                if ( space > inSize ) 
                {
                  mStream_Dirty = morkBool_kTrue; 
                  MORK_MEMCPY(at, source, inSize); 
                  
                  mStream_At += inSize; 
                  outActual += inSize;  
                }
                else 
                {
                  
                  
                  
                  

                  mork_num actual = 0;
                  file->Put(menv, source, inSize, mStream_BufPos, &actual);
                  if ( ev->Good() ) 
                  {
                    outActual += actual;
                    mStream_BufPos += actual;
                  }
                }
              }
            }
          }
          else this->NewBadCursorOrderError(ev);
        }
        else this->NewNullStreamBufferError(ev);
      }
    }
    else this->NewCantWriteSourceError(ev);
  }
  else this->NewFileDownError(ev);
  
  if ( ev->Bad() )
    outActual = 0;

  *aOutSize = outActual;
  return ev->AsErr();
}

NS_IMETHODIMP     
morkStream::Flush(nsIMdbEnv* ev)
{
  morkEnv *mev = morkEnv::FromMdbEnv(ev);
  nsresult rv = NS_ERROR_FAILURE;
  nsIMdbFile* file = mStream_ContentFile;
  if ( this->IsOpenOrClosingNode() && this->FileActive() && file )
  {
    if ( mStream_Dirty )
      this->spill_buf(mev);

    rv = file->Flush(ev);
  }
  else this->NewFileDownError(mev);
  return rv;
}




int
morkStream::fill_getc(morkEnv* ev)
{
  int c = EOF;
  
  nsIMdbFile* file = mStream_ContentFile;
  if ( this->IsOpenAndActiveFile() && file )
  {
    mork_u1* buf = mStream_Buf;
    mork_u1* end = mStream_ReadEnd; 
    if ( end > buf ) 
    {
      mStream_BufPos += ( end - buf ); 
    }
      
    if ( ev->Good() ) 
    {
      
      
      
      

      nsIMdbEnv* menv = ev->AsMdbEnv();
      mork_num actual = 0;
      file->Get(menv, buf, mStream_BufSize, mStream_BufPos, &actual);
      if ( ev->Good() ) 
      {
        if ( actual > mStream_BufSize ) 
          actual = mStream_BufSize;
        
        mStream_At = buf;
        mStream_ReadEnd = buf + actual;
        if ( actual ) 
        {
          c = *mStream_At++; 
          mStream_HitEof = morkBool_kFalse;
        }
        else
          mStream_HitEof = morkBool_kTrue;
      }
    }
  }
  else this->NewFileDownError(ev);
  
  return c;
}

void
morkStream::spill_putc(morkEnv* ev, int c)
{
  this->spill_buf(ev);
  if ( ev->Good() && mStream_At < mStream_WriteEnd )
    this->Putc(ev, c);
}

void
morkStream::spill_buf(morkEnv* ev) 
{
  nsIMdbFile* file = mStream_ContentFile;
  if ( this->IsOpenOrClosingNode() && this->FileActive() && file )
  {
    mork_u1* buf = mStream_Buf;
    if ( mStream_Dirty )
    {
      mork_u1* at = mStream_At;
      if ( at >= buf && at <= mStream_WriteEnd ) 
      {
        mork_num count = (mork_num) (at - buf); 
        if ( count ) 
        {
          if ( count > mStream_BufSize ) 
          {
            count = mStream_BufSize;
            mStream_WriteEnd = buf + mStream_BufSize;
            this->NewBadCursorSlotsError(ev);
          }
          if ( ev->Good() )
          {
            
            
            
            
            nsIMdbEnv* menv = ev->AsMdbEnv();
            mork_num actual = 0;
            
            file->Put(menv, buf, count, mStream_BufPos, &actual);
            if ( ev->Good() )
            {
              mStream_BufPos += actual; 
              mStream_At = buf; 
              mStream_Dirty = morkBool_kFalse;
            }
          }
        }
      }
      else this->NewBadCursorOrderError(ev);
    }
    else
    {
#ifdef MORK_DEBUG
      ev->NewWarning("stream:spill:not:dirty");
#endif 
    }
  }
  else this->NewFileDownError(ev);

}



