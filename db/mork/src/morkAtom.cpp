




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKBLOB_
#include "morkBlob.h"
#endif

#ifndef _MORKATOM_
#include "morkAtom.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKATOMSPACE_
#include "morkAtomSpace.h"
#endif



mork_bool
morkAtom::GetYarn(mdbYarn* outYarn) const
{
  const void* source = 0;  
  mdb_fill fill = 0; 
  mdb_cscode form = 0;
  outYarn->mYarn_More = 0;

  if ( this )
  {
    if ( this->IsWeeBook() )
    {
      morkWeeBookAtom* weeBook = (morkWeeBookAtom*) this;
      source = weeBook->mWeeBookAtom_Body;
      fill = weeBook->mAtom_Size;
    }
    else if ( this->IsBigBook() )
    {
      morkBigBookAtom* bigBook = (morkBigBookAtom*) this;
      source = bigBook->mBigBookAtom_Body;
      fill = bigBook->mBigBookAtom_Size;
      form = bigBook->mBigBookAtom_Form;
    }
    else if ( this->IsWeeAnon() )
    {
      morkWeeAnonAtom* weeAnon = (morkWeeAnonAtom*) this;
      source = weeAnon->mWeeAnonAtom_Body;
      fill = weeAnon->mAtom_Size;
    }
    else if ( this->IsBigAnon() )
    {
      morkBigAnonAtom* bigAnon = (morkBigAnonAtom*) this;
      source = bigAnon->mBigAnonAtom_Body;
      fill = bigAnon->mBigAnonAtom_Size;
      form = bigAnon->mBigAnonAtom_Form;
    }
  }
  if ( source && fill ) 
  {
    
    if ( fill > outYarn->mYarn_Size && outYarn->mYarn_Grow ) 
      (*outYarn->mYarn_Grow)(outYarn, (mdb_size) fill); 
      
    mdb_size size = outYarn->mYarn_Size; 
    if ( fill > size ) 
    {
      outYarn->mYarn_More = fill - size; 
      fill = size; 
    }
    void* dest = outYarn->mYarn_Buf; 
    if ( !dest ) 
      fill = 0; 
      
    if ( fill ) 
      MORK_MEMCPY(dest, source, fill); 
      
    outYarn->mYarn_Fill = fill; 
  }
  else 
  {
    outYarn->mYarn_Fill = 0; 
  }
  outYarn->mYarn_Form = form; 
  
  return ( source != 0 );
}

mork_bool
morkAtom::AsBuf(morkBuf& outBuf) const
{
  const morkAtom* atom = this;
  if ( atom )
  {
    if ( atom->IsWeeBook() )
    {
      morkWeeBookAtom* weeBook = (morkWeeBookAtom*) atom;
      outBuf.mBuf_Body = weeBook->mWeeBookAtom_Body;
      outBuf.mBuf_Fill = weeBook->mAtom_Size;
    }
    else if ( atom->IsBigBook() )
    {
      morkBigBookAtom* bigBook = (morkBigBookAtom*) atom;
      outBuf.mBuf_Body = bigBook->mBigBookAtom_Body;
      outBuf.mBuf_Fill = bigBook->mBigBookAtom_Size;
    }
    else if ( atom->IsWeeAnon() )
    {
      morkWeeAnonAtom* weeAnon = (morkWeeAnonAtom*) atom;
      outBuf.mBuf_Body = weeAnon->mWeeAnonAtom_Body;
      outBuf.mBuf_Fill = weeAnon->mAtom_Size;
    }
    else if ( atom->IsBigAnon() )
    {
      morkBigAnonAtom* bigAnon = (morkBigAnonAtom*) atom;
      outBuf.mBuf_Body = bigAnon->mBigAnonAtom_Body;
      outBuf.mBuf_Fill = bigAnon->mBigAnonAtom_Size;
    }
    else
      atom = 0; 
  }
  
  if ( !atom ) 
  {
    outBuf.mBuf_Body = 0;
    outBuf.mBuf_Fill = 0;
  }
  return ( atom != 0 );
}

mork_bool
morkAtom::AliasYarn(mdbYarn* outYarn) const
{
  outYarn->mYarn_More = 0;
  outYarn->mYarn_Form = 0;
  const morkAtom* atom = this;
  
  if ( atom )
  {
    if ( atom->IsWeeBook() )
    {
      morkWeeBookAtom* weeBook = (morkWeeBookAtom*) atom;
      outYarn->mYarn_Buf = weeBook->mWeeBookAtom_Body;
      outYarn->mYarn_Fill = weeBook->mAtom_Size;
      outYarn->mYarn_Size = weeBook->mAtom_Size;
    }
    else if ( atom->IsBigBook() )
    {
      morkBigBookAtom* bigBook = (morkBigBookAtom*) atom;
      outYarn->mYarn_Buf = bigBook->mBigBookAtom_Body;
      outYarn->mYarn_Fill = bigBook->mBigBookAtom_Size;
      outYarn->mYarn_Size = bigBook->mBigBookAtom_Size;
      outYarn->mYarn_Form = bigBook->mBigBookAtom_Form;
    }
    else if ( atom->IsWeeAnon() )
    {
      morkWeeAnonAtom* weeAnon = (morkWeeAnonAtom*) atom;
      outYarn->mYarn_Buf = weeAnon->mWeeAnonAtom_Body;
      outYarn->mYarn_Fill = weeAnon->mAtom_Size;
      outYarn->mYarn_Size = weeAnon->mAtom_Size;
    }
    else if ( atom->IsBigAnon() )
    {
      morkBigAnonAtom* bigAnon = (morkBigAnonAtom*) atom;
      outYarn->mYarn_Buf = bigAnon->mBigAnonAtom_Body;
      outYarn->mYarn_Fill = bigAnon->mBigAnonAtom_Size;
      outYarn->mYarn_Size = bigAnon->mBigAnonAtom_Size;
      outYarn->mYarn_Form = bigAnon->mBigAnonAtom_Form;
    }
    else
      atom = 0; 
  }
  
  if ( !atom ) 
  {
    outYarn->mYarn_Buf = 0;
    outYarn->mYarn_Fill = 0;
    outYarn->mYarn_Size = 0;
    
  }
  return ( atom != 0 );
}

mork_aid
morkAtom::GetBookAtomAid() const 
{
  return ( this->IsBook() )? ((morkBookAtom*) this)->mBookAtom_Id : 0;
}

mork_scope
morkAtom::GetBookAtomSpaceScope(morkEnv* ev) const 
{
  mork_scope outScope = 0;
  if ( this->IsBook() )
  {
    const morkBookAtom* bookAtom = (const morkBookAtom*) this;
    morkAtomSpace* space = bookAtom->mBookAtom_Space;
    if ( space->IsAtomSpace() )
      outScope = space->SpaceScope();
    else
      space->NonAtomSpaceTypeError(ev);
  }
  
  return outScope;
}

void
morkAtom::MakeCellUseForever(morkEnv* ev)
{
  MORK_USED_1(ev); 
  mAtom_CellUses = morkAtom_kForeverCellUses;
}

mork_u1
morkAtom::AddCellUse(morkEnv* ev)
{
  MORK_USED_1(ev); 
  if ( mAtom_CellUses < morkAtom_kMaxCellUses ) 
    ++mAtom_CellUses;
    
  return mAtom_CellUses;
}

mork_u1
morkAtom::CutCellUse(morkEnv* ev)
{
  if ( mAtom_CellUses ) 
  {
    if ( mAtom_CellUses < morkAtom_kMaxCellUses ) 
      --mAtom_CellUses;
  }
  else
    this->CellUsesUnderflowWarning(ev);
    
  return mAtom_CellUses;
}

 void
morkAtom::CellUsesUnderflowWarning(morkEnv* ev)
{
  ev->NewWarning("mAtom_CellUses underflow");
}

 void
morkAtom::BadAtomKindError(morkEnv* ev)
{
  ev->NewError("bad mAtom_Kind");
}

 void
morkAtom::ZeroAidError(morkEnv* ev)
{
  ev->NewError("zero atom ID");
}

 void
morkAtom::AtomSizeOverflowError(morkEnv* ev)
{
  ev->NewError("atom mAtom_Size overflow");
}

void
morkOidAtom::InitRowOidAtom(morkEnv* ev, const mdbOid& inOid)
{
  MORK_USED_1(ev); 
  mAtom_CellUses = 0;
  mAtom_Kind = morkAtom_kKindRowOid;
  mAtom_Change = morkChange_kNil;
  mAtom_Size = 0;
  mOidAtom_Oid = inOid; 
}

void
morkOidAtom::InitTableOidAtom(morkEnv* ev, const mdbOid& inOid)
{
  MORK_USED_1(ev); 
  mAtom_CellUses = 0;
  mAtom_Kind = morkAtom_kKindTableOid;
  mAtom_Change = morkChange_kNil;
  mAtom_Size = 0;
  mOidAtom_Oid = inOid; 
}

void
morkWeeAnonAtom::InitWeeAnonAtom(morkEnv* ev, const morkBuf& inBuf)
{
  mAtom_Kind = 0;
  mAtom_Change = morkChange_kNil;
  if ( inBuf.mBuf_Fill <= morkAtom_kMaxByteSize )
  {
    mAtom_CellUses = 0;
    mAtom_Kind = morkAtom_kKindWeeAnon;
    mork_size size = inBuf.mBuf_Fill;
    mAtom_Size = (mork_u1) size;
    if ( size && inBuf.mBuf_Body )
      MORK_MEMCPY(mWeeAnonAtom_Body, inBuf.mBuf_Body, size);
        
    mWeeAnonAtom_Body[ size ] = 0;
  }
  else
    this->AtomSizeOverflowError(ev);
}

void
morkBigAnonAtom::InitBigAnonAtom(morkEnv* ev, const morkBuf& inBuf,
  mork_cscode inForm)
{
  MORK_USED_1(ev); 
  mAtom_CellUses = 0;
  mAtom_Kind = morkAtom_kKindBigAnon;
  mAtom_Change = morkChange_kNil;
  mAtom_Size = 0;
  mBigAnonAtom_Form = inForm;
  mork_size size = inBuf.mBuf_Fill;
  mBigAnonAtom_Size = size;
  if ( size && inBuf.mBuf_Body )
    MORK_MEMCPY(mBigAnonAtom_Body, inBuf.mBuf_Body, size);
        
  mBigAnonAtom_Body[ size ] = 0;
}

 void
morkBookAtom::NonBookAtomTypeError(morkEnv* ev)
{
  ev->NewError("non morkBookAtom");
}

mork_u4
morkBookAtom::HashFormAndBody(morkEnv* ev) const
{
  
  
  
  register mork_u4 outHash = 0; 
  register unsigned char c; 
  register const mork_u1* body; 
  mork_size size = 0; 

  if ( this->IsWeeBook() )
  {
    size = mAtom_Size;
    body = ((const morkWeeBookAtom*) this)->mWeeBookAtom_Body;
  }
  else if ( this->IsBigBook() )
  {
    size = ((const morkBigBookAtom*) this)->mBigBookAtom_Size;
    body = ((const morkBigBookAtom*) this)->mBigBookAtom_Body;
  }
  else if ( this->IsFarBook() )
  {
    size = ((const morkFarBookAtom*) this)->mFarBookAtom_Size;
    body = ((const morkFarBookAtom*) this)->mFarBookAtom_Body;
  }
  else
  {
    this->NonBookAtomTypeError(ev);
    return 0;
  }
  
  const mork_u1* end = body + size;
  while ( body < end )
  {
    c = *body++;
    outHash <<= 4;
    outHash += c;
    mork_u4 top = outHash & 0xF0000000L; 
    if ( top ) 
    {
      outHash ^= (top >> 24); 
      outHash ^= top; 
    }
  }
    
  return outHash;
}

mork_bool
morkBookAtom::EqualFormAndBody(morkEnv* ev, const morkBookAtom* inAtom) const
{
  mork_bool outEqual = morkBool_kFalse;
  
  const mork_u1* body = 0; 
  mork_size size; 
  mork_cscode form; 

  if ( inAtom->IsWeeBook() )
  {
    size = inAtom->mAtom_Size;
    body = ((const morkWeeBookAtom*) inAtom)->mWeeBookAtom_Body;
    form = 0;
  }
  else if ( inAtom->IsBigBook() )
  {
    size = ((const morkBigBookAtom*) inAtom)->mBigBookAtom_Size;
    body = ((const morkBigBookAtom*) inAtom)->mBigBookAtom_Body;
    form = ((const morkBigBookAtom*) inAtom)->mBigBookAtom_Form;
  }
  else if ( inAtom->IsFarBook() )
  {
    size = ((const morkFarBookAtom*) inAtom)->mFarBookAtom_Size;
    body = ((const morkFarBookAtom*) inAtom)->mFarBookAtom_Body;
    form = ((const morkFarBookAtom*) inAtom)->mFarBookAtom_Form;
  }
  else
  {
    inAtom->NonBookAtomTypeError(ev);
    return morkBool_kFalse;
  }

  const mork_u1* thisBody = 0; 
  mork_size thisSize; 
  mork_cscode thisForm; 
  
  if ( this->IsWeeBook() )
  {
    thisSize = mAtom_Size;
    thisBody = ((const morkWeeBookAtom*) this)->mWeeBookAtom_Body;
    thisForm = 0;
  }
  else if ( this->IsBigBook() )
  {
    thisSize = ((const morkBigBookAtom*) this)->mBigBookAtom_Size;
    thisBody = ((const morkBigBookAtom*) this)->mBigBookAtom_Body;
    thisForm = ((const morkBigBookAtom*) this)->mBigBookAtom_Form;
  }
  else if ( this->IsFarBook() )
  {
    thisSize = ((const morkFarBookAtom*) this)->mFarBookAtom_Size;
    thisBody = ((const morkFarBookAtom*) this)->mFarBookAtom_Body;
    thisForm = ((const morkFarBookAtom*) this)->mFarBookAtom_Form;
  }
  else
  {
    this->NonBookAtomTypeError(ev);
    return morkBool_kFalse;
  }
  
  
  if ( body && thisBody && size == thisSize && (!size || form == thisForm ))
    outEqual = (MORK_MEMCMP(body, thisBody, size) == 0);
  
  return outEqual;
}


void
morkBookAtom::CutBookAtomFromSpace(morkEnv* ev)
{
  morkAtomSpace* space = mBookAtom_Space;
  if ( space )
  {
    mBookAtom_Space = 0;
    space->mAtomSpace_AtomBodies.CutAtom(ev, this);
    space->mAtomSpace_AtomAids.CutAtom(ev, this);
  }
  else
    ev->NilPointerError();
}

morkWeeBookAtom::morkWeeBookAtom(mork_aid inAid)
{
  mAtom_Kind = morkAtom_kKindWeeBook;
  mAtom_CellUses = 0;
  mAtom_Change = morkChange_kNil;
  mAtom_Size = 0;

  mBookAtom_Space = 0;
  mBookAtom_Id = inAid;

  mWeeBookAtom_Body[ 0 ] = 0;
}

void
morkWeeBookAtom::InitWeeBookAtom(morkEnv* ev, const morkBuf& inBuf,
  morkAtomSpace* ioSpace, mork_aid inAid)
{
  mAtom_Kind = 0;
  mAtom_Change = morkChange_kNil;
  if ( ioSpace )
  {
    if ( inAid )
    {
      if ( inBuf.mBuf_Fill <= morkAtom_kMaxByteSize )
      {
        mAtom_CellUses = 0;
        mAtom_Kind = morkAtom_kKindWeeBook;
        mBookAtom_Space = ioSpace;
        mBookAtom_Id = inAid;
        mork_size size = inBuf.mBuf_Fill;
        mAtom_Size = (mork_u1) size;
        if ( size && inBuf.mBuf_Body )
          MORK_MEMCPY(mWeeBookAtom_Body, inBuf.mBuf_Body, size);
        
        mWeeBookAtom_Body[ size ] = 0;
      }
      else
        this->AtomSizeOverflowError(ev);
    }
    else
      this->ZeroAidError(ev);
  }
  else
    ev->NilPointerError();
}

void
morkBigBookAtom::InitBigBookAtom(morkEnv* ev, const morkBuf& inBuf,
  mork_cscode inForm, morkAtomSpace* ioSpace, mork_aid inAid)
{
  mAtom_Kind = 0;
  mAtom_Change = morkChange_kNil;
  if ( ioSpace )
  {
    if ( inAid )
    {
      mAtom_CellUses = 0;
      mAtom_Kind = morkAtom_kKindBigBook;
      mAtom_Size = 0;
      mBookAtom_Space = ioSpace;
      mBookAtom_Id = inAid;
      mBigBookAtom_Form = inForm;
      mork_size size = inBuf.mBuf_Fill;
      mBigBookAtom_Size = size;
      if ( size && inBuf.mBuf_Body )
        MORK_MEMCPY(mBigBookAtom_Body, inBuf.mBuf_Body, size);
        
      mBigBookAtom_Body[ size ] = 0;
    }
    else
      this->ZeroAidError(ev);
  }
  else
    ev->NilPointerError();
}

void morkFarBookAtom::InitFarBookAtom(morkEnv* ev, const morkBuf& inBuf,
  mork_cscode inForm, morkAtomSpace* ioSpace, mork_aid inAid)
{
  mAtom_Kind = 0;
  mAtom_Change = morkChange_kNil;
  if ( ioSpace )
  {
    if ( inAid )
    {
      mAtom_CellUses = 0;
      mAtom_Kind = morkAtom_kKindFarBook;
      mAtom_Size = 0;
      mBookAtom_Space = ioSpace;
      mBookAtom_Id = inAid;
      mFarBookAtom_Form = inForm;
      mFarBookAtom_Size = inBuf.mBuf_Fill;
      mFarBookAtom_Body = (mork_u1*) inBuf.mBuf_Body;
    }
    else
      this->ZeroAidError(ev);
  }
  else
    ev->NilPointerError();
}



