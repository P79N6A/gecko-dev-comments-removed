



































  
#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKBLOB_
#include "morkBlob.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKFACTORY_
#include "morkFactory.h"
#endif

#ifndef _MORKNODEMAP_
#include "morkNodeMap.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _MORKTHUMB_
#include "morkThumb.h"
#endif




#ifndef _MORKBUILDER_
#include "morkBuilder.h"
#endif

#ifndef _MORKATOMSPACE_
#include "morkAtomSpace.h"
#endif

#ifndef _MORKSTREAM_
#include "morkStream.h"
#endif

#ifndef _MORKATOMSPACE_
#include "morkAtomSpace.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKPORTTABLECURSOR_
#include "morkPortTableCursor.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKROWMAP_
#include "morkRowMap.h"
#endif

#ifndef _MORKPARSER_
#include "morkParser.h"
#endif

#include "nsCOMPtr.h"













 void
morkStore::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseStore(ev);
    this->MarkShut();
  }
}

 void
morkStore::ClosePort(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      morkFactory::SlotWeakFactory((morkFactory*) 0, ev, &mPort_Factory);
      nsIMdbHeap_SlotStrongHeap((nsIMdbHeap*) 0, ev, &mPort_Heap);
      this->CloseObject(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}


morkStore::~morkStore() 
{
  MOZ_COUNT_DTOR(morkStore);
  if (IsOpenNode())
    CloseMorkNode(mMorkEnv);
  MORK_ASSERT(this->IsShutNode());
  MORK_ASSERT(mStore_File==0);
  MORK_ASSERT(mStore_InStream==0);
  MORK_ASSERT(mStore_OutStream==0);
  MORK_ASSERT(mStore_Builder==0);
  MORK_ASSERT(mStore_OidAtomSpace==0);
  MORK_ASSERT(mStore_GroundAtomSpace==0);
  MORK_ASSERT(mStore_GroundColumnSpace==0);
  MORK_ASSERT(mStore_RowSpaces.IsShutNode());
  MORK_ASSERT(mStore_AtomSpaces.IsShutNode());
  MORK_ASSERT(mStore_Pool.IsShutNode());
}


morkStore::morkStore(morkEnv* ev, const morkUsage& inUsage,
     nsIMdbHeap* ioNodeHeap, 
     morkFactory* inFactory, 
     nsIMdbHeap* ioPortHeap  
     )
: morkObject(ev, inUsage, ioNodeHeap, morkColor_kNone, (morkHandle*) 0)
, mPort_Env( ev )
, mPort_Factory( 0 )
, mPort_Heap( 0 )
, mStore_OidAtomSpace( 0 )
, mStore_GroundAtomSpace( 0 )
, mStore_GroundColumnSpace( 0 )

, mStore_File( 0 )
, mStore_InStream( 0 )
, mStore_Builder( 0 )
, mStore_OutStream( 0 )

, mStore_RowSpaces(ev, morkUsage::kMember, (nsIMdbHeap*) 0, ioPortHeap)
, mStore_AtomSpaces(ev, morkUsage::kMember, (nsIMdbHeap*) 0, ioPortHeap)
, mStore_Zone(ev, morkUsage::kMember, (nsIMdbHeap*) 0, ioPortHeap)
, mStore_Pool(ev, morkUsage::kMember, (nsIMdbHeap*) 0, ioPortHeap)

, mStore_CommitGroupIdentity( 0 )

, mStore_FirstCommitGroupPos( 0 )
, mStore_SecondCommitGroupPos( 0 )


, mStore_CanAutoAssignAtomIdentity( morkBool_kFalse )
, mStore_CanDirty( morkBool_kFalse ) 
, mStore_CanWriteIncremental( morkBool_kTrue ) 
{
  MOZ_COUNT_CTOR(morkStore);
  if ( ev->Good() )
  {
    if ( inFactory && ioPortHeap )
    {
      morkFactory::SlotWeakFactory(inFactory, ev, &mPort_Factory);
      nsIMdbHeap_SlotStrongHeap(ioPortHeap, ev, &mPort_Heap);
      if ( ev->Good() )
        mNode_Derived = morkDerived_kPort;
    }
    else
      ev->NilPointerError();
  }
  if ( ev->Good() )
  {
    mNode_Derived = morkDerived_kStore;
    
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(morkStore, morkObject, nsIMdbStore)

 void
morkStore::CloseStore(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {

      nsIMdbFile* file = mStore_File;
      file->AddRef();

      morkFactory::SlotWeakFactory((morkFactory*) 0, ev, &mPort_Factory);
      nsIMdbHeap_SlotStrongHeap((nsIMdbHeap*) 0, ev, &mPort_Heap);
      morkAtomSpace::SlotStrongAtomSpace((morkAtomSpace*) 0, ev,
        &mStore_OidAtomSpace);
      morkAtomSpace::SlotStrongAtomSpace((morkAtomSpace*) 0, ev,
        &mStore_GroundAtomSpace);
      morkAtomSpace::SlotStrongAtomSpace((morkAtomSpace*) 0, ev,
        &mStore_GroundColumnSpace);
      mStore_RowSpaces.CloseMorkNode(ev);
      mStore_AtomSpaces.CloseMorkNode(ev);
      morkBuilder::SlotStrongBuilder((morkBuilder*) 0, ev, &mStore_Builder);
      
      nsIMdbFile_SlotStrongFile((nsIMdbFile*) 0, ev,
        &mStore_File);
      
      file->Release();

      morkStream::SlotStrongStream((morkStream*) 0, ev, &mStore_InStream);
      morkStream::SlotStrongStream((morkStream*) 0, ev, &mStore_OutStream);

      mStore_Pool.CloseMorkNode(ev);
      mStore_Zone.CloseMorkNode(ev);
      this->ClosePort(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}





mork_bool morkStore::DoPreferLargeOverCompressCommit(morkEnv* ev)
  
{
  nsIMdbFile* file = mStore_File;
  if ( file && mStore_CanWriteIncremental )
  {
    mdb_pos fileEof = 0;
    file->Eof(ev->AsMdbEnv(), &fileEof);
    if ( ev->Good() && fileEof > 128 )
      return morkBool_kTrue;
  }
  return morkBool_kFalse;
}

mork_percent morkStore::PercentOfStoreWasted(morkEnv* ev)
{
  mork_percent outPercent = 0;
  nsIMdbFile* file = mStore_File;
  
  if ( file )
  {
    mork_pos firstPos = mStore_FirstCommitGroupPos;
    mork_pos secondPos = mStore_SecondCommitGroupPos;
    if ( firstPos || secondPos )
    {
      if ( firstPos < 512 && secondPos > firstPos )
        firstPos = secondPos; 
        
      mork_pos fileLength = 0;
      file->Eof(ev->AsMdbEnv(), &fileLength); 
      if ( ev->Good() && fileLength > firstPos )
      {
        mork_size groupContent = fileLength - firstPos;
        outPercent = ( groupContent * 100 ) / fileLength;
      }
    }
  }
  else
    this->NilStoreFileError(ev);
    
  return outPercent;
}

void
morkStore::SetStoreAndAllSpacesCanDirty(morkEnv* ev, mork_bool inCanDirty)
{
  mStore_CanDirty = inCanDirty;
  
  mork_change* c = 0;
  mork_scope* key = 0; 

  if ( ev->Good() )
  {
    morkAtomSpaceMapIter asi(ev, &mStore_AtomSpaces);

    morkAtomSpace* atomSpace = 0; 
    
    for ( c = asi.FirstAtomSpace(ev, key, &atomSpace); c && ev->Good();
          c = asi.NextAtomSpace(ev, key, &atomSpace) )
    {
      if ( atomSpace )
      {
        if ( atomSpace->IsAtomSpace() )
          atomSpace->mSpace_CanDirty = inCanDirty;
        else
          atomSpace->NonAtomSpaceTypeError(ev);
      }
      else
        ev->NilPointerError();
    }
  }

  if ( ev->Good() )
  {
    morkRowSpaceMapIter rsi(ev, &mStore_RowSpaces);
    morkRowSpace* rowSpace = 0; 
    
    for ( c = rsi.FirstRowSpace(ev, key, &rowSpace); c && ev->Good();
          c = rsi.NextRowSpace(ev, key, &rowSpace) )
    {
      if ( rowSpace )
      {
        if ( rowSpace->IsRowSpace() )
          rowSpace->mSpace_CanDirty = inCanDirty;
        else
          rowSpace->NonRowSpaceTypeError(ev);
      }
    }
  }
}

void
morkStore::RenumberAllCollectableContent(morkEnv* ev)
{
  MORK_USED_1(ev);
  
}

nsIMdbStore*
morkStore::AcquireStoreHandle(morkEnv* ev)
{
  return this;
}


morkFarBookAtom*
morkStore::StageAliasAsFarBookAtom(morkEnv* ev, const morkMid* inMid,
   morkAtomSpace* ioSpace, mork_cscode inForm)
{
  if ( inMid && inMid->mMid_Buf )
  {
    const morkBuf* buf = inMid->mMid_Buf;
    mork_size length = buf->mBuf_Fill;
    if ( length <= morkBookAtom_kMaxBodySize )
    {
      mork_aid dummyAid = 1;
      
      
       
      mStore_FarBookAtom.InitFarBookAtom(ev, *buf, 
        inForm, ioSpace, dummyAid);
      return &mStore_FarBookAtom;
    }
  }
  else
    ev->NilPointerError();
    
  return (morkFarBookAtom*) 0;
}

morkFarBookAtom*
morkStore::StageYarnAsFarBookAtom(morkEnv* ev, const mdbYarn* inYarn,
   morkAtomSpace* ioSpace)
{
  if ( inYarn && inYarn->mYarn_Buf )
  {
    mork_size length = inYarn->mYarn_Fill;
    if ( length <= morkBookAtom_kMaxBodySize )
    {
      morkBuf buf(inYarn->mYarn_Buf, length);
      mork_aid dummyAid = 1;
      
      
      mStore_FarBookAtom.InitFarBookAtom(ev, buf, 
        inYarn->mYarn_Form, ioSpace, dummyAid);
      return &mStore_FarBookAtom;
    }
  }
  else
    ev->NilPointerError();
    
  return (morkFarBookAtom*) 0;
}

morkFarBookAtom*
morkStore::StageStringAsFarBookAtom(morkEnv* ev, const char* inString,
   mork_cscode inForm, morkAtomSpace* ioSpace)
{
  if ( inString )
  {
    mork_size length = MORK_STRLEN(inString);
    if ( length <= morkBookAtom_kMaxBodySize )
    {
      morkBuf buf(inString, length);
      mork_aid dummyAid = 1;
      
      mStore_FarBookAtom.InitFarBookAtom(ev, buf, inForm, ioSpace, dummyAid);
      return &mStore_FarBookAtom;
    }
  }
  else
    ev->NilPointerError();
    
  return (morkFarBookAtom*) 0;
}

morkAtomSpace* morkStore::LazyGetOidAtomSpace(morkEnv* ev)
{
  MORK_USED_1(ev);
  if ( !mStore_OidAtomSpace )
  {
  }
  return mStore_OidAtomSpace;
}

morkAtomSpace* morkStore::LazyGetGroundAtomSpace(morkEnv* ev)
{
  if ( !mStore_GroundAtomSpace )
  {
    mork_scope atomScope = morkStore_kValueSpaceScope;
    nsIMdbHeap* heap = mPort_Heap;
    morkAtomSpace* space = new(*heap, ev) 
      morkAtomSpace(ev, morkUsage::kHeap, atomScope, this, heap, heap);
      
    if ( space ) 
    {
      this->MaybeDirtyStore();
    
      mStore_GroundAtomSpace = space; 
      mStore_AtomSpaces.AddAtomSpace(ev, space);
    }
  }
  return mStore_GroundAtomSpace;
}

morkAtomSpace* morkStore::LazyGetGroundColumnSpace(morkEnv* ev)
{
  if ( !mStore_GroundColumnSpace )
  {
    mork_scope atomScope = morkStore_kGroundColumnSpace;
    nsIMdbHeap* heap = mPort_Heap;
    morkAtomSpace* space = new(*heap, ev) 
      morkAtomSpace(ev, morkUsage::kHeap, atomScope, this, heap, heap);
      
    if ( space ) 
    {
      this->MaybeDirtyStore();
    
      mStore_GroundColumnSpace = space; 
      mStore_AtomSpaces.AddAtomSpace(ev, space);
    }
  }
  return mStore_GroundColumnSpace;
}

morkStream* morkStore::LazyGetInStream(morkEnv* ev)
{
  if ( !mStore_InStream )
  {
    nsIMdbFile* file = mStore_File;
    if ( file )
    {
      morkStream* stream = new(*mPort_Heap, ev) 
        morkStream(ev, morkUsage::kHeap, mPort_Heap, file,
          morkStore_kStreamBufSize,  morkBool_kTrue);
      if ( stream )
      {
        this->MaybeDirtyStore();
        mStore_InStream = stream; 
      }
    }
    else
      this->NilStoreFileError(ev);
  }
  return mStore_InStream;
}

morkStream* morkStore::LazyGetOutStream(morkEnv* ev)
{
  if ( !mStore_OutStream )
  {
    nsIMdbFile* file = mStore_File;
    if ( file )
    {
      morkStream* stream = new(*mPort_Heap, ev) 
        morkStream(ev, morkUsage::kHeap, mPort_Heap, file,
          morkStore_kStreamBufSize,  morkBool_kFalse);
      if ( stream )
      {
        this->MaybeDirtyStore();
        mStore_InStream = stream; 
      }
    }
    else
      this->NilStoreFileError(ev);
  }
  return mStore_OutStream;
}

void
morkStore::ForgetBuilder(morkEnv* ev)
{
  if ( mStore_Builder )
    morkBuilder::SlotStrongBuilder((morkBuilder*) 0, ev, &mStore_Builder);
  if ( mStore_InStream )
    morkStream::SlotStrongStream((morkStream*) 0, ev, &mStore_InStream);
}

morkBuilder* morkStore::LazyGetBuilder(morkEnv* ev)
{
  if ( !mStore_Builder )
  {
    morkStream* stream = this->LazyGetInStream(ev);
    if ( stream )
    {
      nsIMdbHeap* heap = mPort_Heap;
      morkBuilder* builder = new(*heap, ev) 
        morkBuilder(ev, morkUsage::kHeap, heap, stream,
          morkBuilder_kDefaultBytesPerParseSegment, heap, this);
      if ( builder )
      {
        mStore_Builder = builder; 
      }
    }
  }
  return mStore_Builder;
}

morkRowSpace*
morkStore::LazyGetRowSpace(morkEnv* ev, mdb_scope inRowScope)
{
  morkRowSpace* outSpace = mStore_RowSpaces.GetRowSpace(ev, inRowScope);
  if ( !outSpace && ev->Good() ) 
  {
    nsIMdbHeap* heap = mPort_Heap;
    outSpace = new(*heap, ev) 
      morkRowSpace(ev, morkUsage::kHeap, inRowScope, this, heap, heap);
      
    if ( outSpace ) 
    {
      this->MaybeDirtyStore();
    
      
      if ( mStore_RowSpaces.AddRowSpace(ev, outSpace) )
        outSpace->CutStrongRef(ev); 
    }
  }
  return outSpace;
}

morkAtomSpace*
morkStore::LazyGetAtomSpace(morkEnv* ev, mdb_scope inAtomScope)
{
  morkAtomSpace* outSpace = mStore_AtomSpaces.GetAtomSpace(ev, inAtomScope);
  if ( !outSpace && ev->Good() ) 
  {
    if ( inAtomScope == morkStore_kValueSpaceScope )
      outSpace = this->LazyGetGroundAtomSpace(ev);
      
    else if ( inAtomScope == morkStore_kGroundColumnSpace )
      outSpace = this->LazyGetGroundColumnSpace(ev);
    else
    {
      nsIMdbHeap* heap = mPort_Heap;
      outSpace = new(*heap, ev) 
        morkAtomSpace(ev, morkUsage::kHeap, inAtomScope, this, heap, heap);
        
      if ( outSpace ) 
      {
        this->MaybeDirtyStore();
    
        
        if ( mStore_AtomSpaces.AddAtomSpace(ev, outSpace) )
          outSpace->CutStrongRef(ev); 
      }
    }
  }
  return outSpace;
}

 void
morkStore::NonStoreTypeError(morkEnv* ev)
{
  ev->NewError("non morkStore");
}

 void
morkStore::NilStoreFileError(morkEnv* ev)
{
  ev->NewError("nil mStore_File");
}

 void
morkStore::CannotAutoAssignAtomIdentityError(morkEnv* ev)
{
  ev->NewError("false mStore_CanAutoAssignAtomIdentity");
}


mork_bool
morkStore::OpenStoreFile(morkEnv* ev, mork_bool inFrozen,
    
    nsIMdbFile* ioFile, 
    const mdbOpenPolicy* inOpenPolicy)
{
  MORK_USED_2(inOpenPolicy,inFrozen);
  nsIMdbFile_SlotStrongFile(ioFile, ev, &mStore_File);
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  return ev->Good();
}

mork_bool
morkStore::CreateStoreFile(morkEnv* ev,
    
    nsIMdbFile* ioFile, 
    const mdbOpenPolicy* inOpenPolicy)
{
  MORK_USED_1(inOpenPolicy);
  nsIMdbFile_SlotStrongFile(ioFile, ev, &mStore_File);
  
  return ev->Good();
}

morkAtom*
morkStore::CopyAtom(morkEnv* ev, const morkAtom* inAtom)

{
  morkAtom* outAtom = 0;
  if ( inAtom )
  {
    mdbYarn yarn;
    if ( inAtom->AliasYarn(&yarn) )
      outAtom = this->YarnToAtom(ev, &yarn, PR_TRUE );
  }
  return outAtom;
}
 
morkAtom*
morkStore::YarnToAtom(morkEnv* ev, const mdbYarn* inYarn, PRBool createIfMissing )
{
  morkAtom* outAtom = 0;
  if ( ev->Good() )
  {
    morkAtomSpace* groundSpace = this->LazyGetGroundAtomSpace(ev);
    if ( groundSpace )
    {
      morkFarBookAtom* keyAtom =
        this->StageYarnAsFarBookAtom(ev, inYarn, groundSpace);
        
      if ( keyAtom )
      {
        morkAtomBodyMap* map = &groundSpace->mAtomSpace_AtomBodies;
        outAtom = map->GetAtom(ev, keyAtom);
        if ( !outAtom && createIfMissing)
        {
          this->MaybeDirtyStore();
          outAtom = groundSpace->MakeBookAtomCopy(ev, *keyAtom);
        }
      }
      else if ( ev->Good() )
      {
        morkBuf b(inYarn->mYarn_Buf, inYarn->mYarn_Fill);
        morkZone* z = &mStore_Zone;
        outAtom = mStore_Pool.NewAnonAtom(ev, b, inYarn->mYarn_Form, z);
      }
    }
  }
  return outAtom;
}

mork_bool
morkStore::MidToOid(morkEnv* ev, const morkMid& inMid, mdbOid* outOid)
{
  *outOid = inMid.mMid_Oid;
  const morkBuf* buf = inMid.mMid_Buf;
  if ( buf && !outOid->mOid_Scope )
  {
    if ( buf->mBuf_Fill <= morkBookAtom_kMaxBodySize )
    {
      if ( buf->mBuf_Fill == 1 )
      {
        mork_u1* name = (mork_u1*) buf->mBuf_Body;
        if ( name )
        {
          outOid->mOid_Scope = (mork_scope) *name;
          return ev->Good();
        }
      }
      morkAtomSpace* groundSpace = this->LazyGetGroundColumnSpace(ev);
      if ( groundSpace )
      {
        mork_cscode form = 0; 
        mork_aid aid = 1; 
        mStore_FarBookAtom.InitFarBookAtom(ev, *buf, form, groundSpace, aid);
        morkFarBookAtom* keyAtom = &mStore_FarBookAtom;
        morkAtomBodyMap* map = &groundSpace->mAtomSpace_AtomBodies;
        morkBookAtom* bookAtom = map->GetAtom(ev, keyAtom);
        if ( bookAtom )
          outOid->mOid_Scope = bookAtom->mBookAtom_Id;
        else
        {
          this->MaybeDirtyStore();
          bookAtom = groundSpace->MakeBookAtomCopy(ev, *keyAtom);
          if ( bookAtom )
          {
            outOid->mOid_Scope = bookAtom->mBookAtom_Id;
            bookAtom->MakeCellUseForever(ev);
          }
        }
      }
    }
  }
  return ev->Good();
}

morkRow*
morkStore::MidToRow(morkEnv* ev, const morkMid& inMid)
{
  mdbOid tempOid;
  this->MidToOid(ev, inMid, &tempOid);
  return this->OidToRow(ev, &tempOid);
}

morkTable*
morkStore::MidToTable(morkEnv* ev, const morkMid& inMid)
{
  mdbOid tempOid;
  this->MidToOid(ev, inMid, &tempOid);
  return this->OidToTable(ev, &tempOid,  (mdbOid*) 0);
}

mork_bool
morkStore::MidToYarn(morkEnv* ev, const morkMid& inMid, mdbYarn* outYarn)
{
  mdbOid tempOid;
  this->MidToOid(ev, inMid, &tempOid);
  return this->OidToYarn(ev, tempOid, outYarn);
}

mork_bool
morkStore::OidToYarn(morkEnv* ev, const mdbOid& inOid, mdbYarn* outYarn)
{
  morkBookAtom* atom = 0;
      
  morkAtomSpace* atomSpace = mStore_AtomSpaces.GetAtomSpace(ev, inOid.mOid_Scope);
  if ( atomSpace )
  {
    morkAtomAidMap* map = &atomSpace->mAtomSpace_AtomAids;
    atom = map->GetAid(ev, (mork_aid) inOid.mOid_Id);
  }
  atom->GetYarn(outYarn); 

  return ev->Good();
}

morkBookAtom*
morkStore::MidToAtom(morkEnv* ev, const morkMid& inMid)
{
  morkBookAtom* outAtom = 0;
  mdbOid oid;
  if ( this->MidToOid(ev, inMid, &oid) )
  {
    morkAtomSpace* atomSpace = mStore_AtomSpaces.GetAtomSpace(ev, oid.mOid_Scope);
    if ( atomSpace )
    {
      morkAtomAidMap* map = &atomSpace->mAtomSpace_AtomAids;
      outAtom = map->GetAid(ev, (mork_aid) oid.mOid_Id);
    }
  }
  return outAtom;
}

 void
morkStore::SmallTokenToOneByteYarn(morkEnv* ev, mdb_token inToken,
  mdbYarn* outYarn)
{
  MORK_USED_1(ev);
  if ( outYarn->mYarn_Buf && outYarn->mYarn_Size ) 
  {
    mork_u1* buf = (mork_u1*) outYarn->mYarn_Buf; 
    buf[ 0 ] = (mork_u1) inToken; 
    outYarn->mYarn_Fill = 1;
    outYarn->mYarn_More = 0;
  }
  else 
  {
    outYarn->mYarn_More = 1;
    outYarn->mYarn_Fill = 0;
  }
}

void
morkStore::TokenToString(morkEnv* ev, mdb_token inToken, mdbYarn* outTokenName)
{
  if ( inToken > morkAtomSpace_kMaxSevenBitAid )
  {
    morkBookAtom* atom = 0;
    morkAtomSpace* space = mStore_GroundColumnSpace;
    if ( space )
      atom = space->mAtomSpace_AtomAids.GetAid(ev, (mork_aid) inToken);
      
    atom->GetYarn(outTokenName); 
  }
  else 
    this->SmallTokenToOneByteYarn(ev, inToken, outTokenName);
}
  






























































morkAtom*
morkStore::AddAlias(morkEnv* ev, const morkMid& inMid, mork_cscode inForm)
{
  morkBookAtom* outAtom = 0;
  if ( ev->Good() )
  {
    const mdbOid* oid = &inMid.mMid_Oid;
    morkAtomSpace* atomSpace = this->LazyGetAtomSpace(ev, oid->mOid_Scope);
    if ( atomSpace )
    {
      morkFarBookAtom* keyAtom =
        this->StageAliasAsFarBookAtom(ev, &inMid, atomSpace, inForm);
      if ( keyAtom )
      {
         morkAtomAidMap* map = &atomSpace->mAtomSpace_AtomAids;
        outAtom = map->GetAid(ev, (mork_aid) oid->mOid_Id);
        if ( outAtom )
        {
          if ( !outAtom->EqualFormAndBody(ev, keyAtom) )
              ev->NewError("duplicate alias ID");
        }
        else
        {
          this->MaybeDirtyStore();
          keyAtom->mBookAtom_Id = oid->mOid_Id;
          outAtom = atomSpace->MakeBookAtomCopyWithAid(ev,
            *keyAtom, (mork_aid) oid->mOid_Id);
            
          
          
          
          
          
          
          
          
          
        }
      }
    }
  }
  return outAtom;
}

#define morkStore_kMaxCopyTokenSize 512 /* if larger, cannot be copied */
  
mork_token
morkStore::CopyToken(morkEnv* ev, mdb_token inToken, morkStore* inStore)

{
  mork_token outToken = 0;
  if ( inStore == this ) 
    outToken = inToken; 
  else
  {
    char yarnBuf[ morkStore_kMaxCopyTokenSize ];
    mdbYarn yarn;
    yarn.mYarn_Buf = yarnBuf;
    yarn.mYarn_Fill = 0;
    yarn.mYarn_Size = morkStore_kMaxCopyTokenSize;
    yarn.mYarn_More = 0;
    yarn.mYarn_Form = 0;
    yarn.mYarn_Grow = 0;
    
    inStore->TokenToString(ev, inToken, &yarn);
    if ( ev->Good() )
    {
      morkBuf buf(yarn.mYarn_Buf, yarn.mYarn_Fill);
      outToken = this->BufToToken(ev, &buf);
    }
  }
  return outToken;
}

mork_token
morkStore::BufToToken(morkEnv* ev, const morkBuf* inBuf)
{
  mork_token outToken = 0;
  if ( ev->Good() )
  {
    const mork_u1* s = (const mork_u1*) inBuf->mBuf_Body;
    mork_bool nonAscii = ( *s > 0x7F );
    mork_size length = inBuf->mBuf_Fill;
    if ( nonAscii || length > 1 ) 
    {
      mork_cscode form = 0; 
      morkAtomSpace* space = this->LazyGetGroundColumnSpace(ev);
      if ( space )
      {
        morkFarBookAtom* keyAtom = 0;
        if ( length <= morkBookAtom_kMaxBodySize )
        {
          mork_aid aid = 1; 
          
          mStore_FarBookAtom.InitFarBookAtom(ev, *inBuf, form, space, aid);
          keyAtom = &mStore_FarBookAtom;
        }
        if ( keyAtom )
        {
          morkAtomBodyMap* map = &space->mAtomSpace_AtomBodies;
          morkBookAtom* bookAtom = map->GetAtom(ev, keyAtom);
          if ( bookAtom )
            outToken = bookAtom->mBookAtom_Id;
          else
          {
            this->MaybeDirtyStore();
            bookAtom = space->MakeBookAtomCopy(ev, *keyAtom);
            if ( bookAtom )
            {
              outToken = bookAtom->mBookAtom_Id;
              bookAtom->MakeCellUseForever(ev);
            }
          }
        }
      }
    }
    else 
      outToken = *s;
  }
  
  return outToken;
}

mork_token
morkStore::StringToToken(morkEnv* ev, const char* inTokenName)
{
  mork_token outToken = 0;
  if ( ev->Good() )
  {
    const mork_u1* s = (const mork_u1*) inTokenName;
    mork_bool nonAscii = ( *s > 0x7F );
    if ( nonAscii || ( *s && s[ 1 ] ) ) 
    {
      mork_cscode form = 0; 
      morkAtomSpace* groundSpace = this->LazyGetGroundColumnSpace(ev);
      if ( groundSpace )
      {
        morkFarBookAtom* keyAtom =
          this->StageStringAsFarBookAtom(ev, inTokenName, form, groundSpace);
        if ( keyAtom )
        {
          morkAtomBodyMap* map = &groundSpace->mAtomSpace_AtomBodies;
          morkBookAtom* bookAtom = map->GetAtom(ev, keyAtom);
          if ( bookAtom )
            outToken = bookAtom->mBookAtom_Id;
          else
          {
            this->MaybeDirtyStore();
            bookAtom = groundSpace->MakeBookAtomCopy(ev, *keyAtom);
            if ( bookAtom )
            {
              outToken = bookAtom->mBookAtom_Id;
              bookAtom->MakeCellUseForever(ev);
            }
          }
        }
      }
    }
    else 
      outToken = *s;
  }
  
  return outToken;
}

mork_token
morkStore::QueryToken(morkEnv* ev, const char* inTokenName)
{
  mork_token outToken = 0;
  if ( ev->Good() )
  {
    const mork_u1* s = (const mork_u1*) inTokenName;
    mork_bool nonAscii = ( *s > 0x7F );
    if ( nonAscii || ( *s && s[ 1 ] ) ) 
    {
      mork_cscode form = 0; 
      morkAtomSpace* groundSpace = this->LazyGetGroundColumnSpace(ev);
      if ( groundSpace )
      {
        morkFarBookAtom* keyAtom =
          this->StageStringAsFarBookAtom(ev, inTokenName, form, groundSpace);
        if ( keyAtom )
        {
          morkAtomBodyMap* map = &groundSpace->mAtomSpace_AtomBodies;
          morkBookAtom* bookAtom = map->GetAtom(ev, keyAtom);
          if ( bookAtom )
          {
            outToken = bookAtom->mBookAtom_Id;
            bookAtom->MakeCellUseForever(ev);
          }
        }
      }
    }
    else 
      outToken = *s;
  }
  
  return outToken;
}

mork_bool
morkStore::HasTableKind(morkEnv* ev, mdb_scope inRowScope, 
  mdb_kind inTableKind, mdb_count* outTableCount)
{
  MORK_USED_2(inRowScope,inTableKind);
  mork_bool outBool = morkBool_kFalse;
  mdb_count tableCount = 0;

  ev->StubMethodOnlyError();
  
  if ( outTableCount )
    *outTableCount = tableCount;
  return outBool;
}

morkTable*
morkStore::GetTableKind(morkEnv* ev, mdb_scope inRowScope, 
  mdb_kind inTableKind, mdb_count* outTableCount,
  mdb_bool* outMustBeUnique)
{
  morkTable* outTable = 0;
  if ( ev->Good() )
  {
    morkRowSpace* rowSpace = this->LazyGetRowSpace(ev, inRowScope);
    if ( rowSpace )
    {
      outTable = rowSpace->FindTableByKind(ev, inTableKind);
      if ( outTable )
      {
        if ( outTableCount )
          *outTableCount = outTable->GetRowCount();
        if ( outMustBeUnique )
          *outMustBeUnique = outTable->IsTableUnique();
      }
    }
  }
  return outTable;
}

morkRow*
morkStore::FindRow(morkEnv* ev, mdb_scope inScope, mdb_column inColumn,
  const mdbYarn* inYarn)
{
  morkRow* outRow = 0;
  if ( ev->Good() )
  {
    morkRowSpace* rowSpace = this->LazyGetRowSpace(ev, inScope);
    if ( rowSpace )
    {
      outRow = rowSpace->FindRow(ev, inColumn, inYarn);
    }
  }
  return outRow;
}

morkRow*
morkStore::GetRow(morkEnv* ev, const mdbOid* inOid)
{
  morkRow* outRow = 0;
  if ( ev->Good() )
  {
    morkRowSpace* rowSpace = this->LazyGetRowSpace(ev, inOid->mOid_Scope);
    if ( rowSpace )
    {
      outRow = rowSpace->mRowSpace_Rows.GetOid(ev, inOid);
    }
  }
  return outRow;
}

morkTable*
morkStore::GetTable(morkEnv* ev, const mdbOid* inOid)
{
  morkTable* outTable = 0;
  if ( ev->Good() )
  {
    morkRowSpace* rowSpace = this->LazyGetRowSpace(ev, inOid->mOid_Scope);
    if ( rowSpace )
    {
      outTable = rowSpace->FindTableByTid(ev, inOid->mOid_Id);
    }
  }
  return outTable;
}
  
morkTable*
morkStore::NewTable(morkEnv* ev, mdb_scope inRowScope,
  mdb_kind inTableKind, mdb_bool inMustBeUnique,
  const mdbOid* inOptionalMetaRowOid) 
{
  morkTable* outTable = 0;
  if ( ev->Good() )
  {
    morkRowSpace* rowSpace = this->LazyGetRowSpace(ev, inRowScope);
    if ( rowSpace )
      outTable = rowSpace->NewTable(ev, inTableKind, inMustBeUnique,
        inOptionalMetaRowOid);
  }
  return outTable;
}

morkPortTableCursor*
morkStore::GetPortTableCursor(morkEnv* ev, mdb_scope inRowScope,
  mdb_kind inTableKind)
{
  morkPortTableCursor* outCursor = 0;
  if ( ev->Good() )
  {
    nsIMdbHeap* heap = mPort_Heap;
    outCursor = new(*heap, ev) 
      morkPortTableCursor(ev, morkUsage::kHeap, heap, this,
        inRowScope, inTableKind, heap);
  }
  NS_IF_ADDREF(outCursor);
  return outCursor;
}

morkRow*
morkStore::NewRow(morkEnv* ev, mdb_scope inRowScope)
{
  morkRow* outRow = 0;
  if ( ev->Good() )
  {
    morkRowSpace* rowSpace = this->LazyGetRowSpace(ev, inRowScope);
    if ( rowSpace )
      outRow = rowSpace->NewRow(ev);
  }
  return outRow;
}

morkRow*
morkStore::NewRowWithOid(morkEnv* ev, const mdbOid* inOid)
{
  morkRow* outRow = 0;
  if ( ev->Good() )
  {
    morkRowSpace* rowSpace = this->LazyGetRowSpace(ev, inOid->mOid_Scope);
    if ( rowSpace )
      outRow = rowSpace->NewRowWithOid(ev, inOid);
  }
  return outRow;
}

morkRow*
morkStore::OidToRow(morkEnv* ev, const mdbOid* inOid)
  
{
  morkRow* outRow = 0;
  if ( ev->Good() )
  {
    morkRowSpace* rowSpace = this->LazyGetRowSpace(ev, inOid->mOid_Scope);
    if ( rowSpace )
    {
      outRow = rowSpace->mRowSpace_Rows.GetOid(ev, inOid);
      if ( !outRow && ev->Good() )
        outRow = rowSpace->NewRowWithOid(ev, inOid);
    }
  }
  return outRow;
}

morkTable*
morkStore::OidToTable(morkEnv* ev, const mdbOid* inOid,
  const mdbOid* inOptionalMetaRowOid) 
  
{
  morkTable* outTable = 0;
  if ( ev->Good() )
  {
    morkRowSpace* rowSpace = this->LazyGetRowSpace(ev, inOid->mOid_Scope);
    if ( rowSpace )
    {
      outTable = rowSpace->mRowSpace_Tables.GetTable(ev, inOid->mOid_Id);
      if ( !outTable && ev->Good() )
      {
        mork_kind tableKind = morkStore_kNoneToken;
        outTable = rowSpace->NewTableWithTid(ev, inOid->mOid_Id, tableKind,
          inOptionalMetaRowOid);
      }
    }
  }
  return outTable;
}




NS_IMETHODIMP
morkStore::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  *outCount = WeakRefsOnly();
  return NS_OK;
}  
NS_IMETHODIMP
morkStore::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  *outCount = StrongRefsOnly();
  return NS_OK;
}

NS_IMETHODIMP
morkStore::AddWeakRef(nsIMdbEnv* mev)
{
  morkEnv *ev  = morkEnv::FromMdbEnv(mev);
  return morkNode::AddWeakRef(ev);
}
NS_IMETHODIMP
morkStore::AddStrongRef(nsIMdbEnv* mev)
{
  return AddRef();
}

NS_IMETHODIMP
morkStore::CutWeakRef(nsIMdbEnv* mev)
{
  morkEnv *ev  = morkEnv::FromMdbEnv(mev);
  return morkNode::CutWeakRef(ev);
}
NS_IMETHODIMP
morkStore::CutStrongRef(nsIMdbEnv* mev)
{
  return Release();
}

NS_IMETHODIMP
morkStore::CloseMdbObject(nsIMdbEnv* mev)
{
  morkEnv *ev = morkEnv::FromMdbEnv(mev);
  CloseMorkNode(ev);
  Release();
  return NS_OK;
}

NS_IMETHODIMP
morkStore::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  *outOpen = IsOpenNode();
  return NS_OK;
}







NS_IMETHODIMP
morkStore::GetIsPortReadonly(nsIMdbEnv* mev, mdb_bool* outBool)
{
  mdb_err outErr = 0;
  mdb_bool isReadOnly = morkBool_kFalse;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outBool )
    *outBool = isReadOnly;
  return outErr;
}

morkEnv*
morkStore::CanUseStore(nsIMdbEnv* mev,
  mork_bool inMutable, mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if (IsStore())
      outEnv = ev;
    else
      NonStoreTypeError(ev);
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  return outEnv;
}

NS_IMETHODIMP
morkStore::GetIsStore(nsIMdbEnv* mev, mdb_bool* outBool)
{
  MORK_USED_1(mev);
 if ( outBool )
    *outBool = morkBool_kTrue;
  return 0;
}

NS_IMETHODIMP
morkStore::GetIsStoreAndDirty(nsIMdbEnv* mev, mdb_bool* outBool)
{
  mdb_err outErr = 0;
  mdb_bool isStoreAndDirty = morkBool_kFalse;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outBool )
    *outBool = isStoreAndDirty;
  return outErr;
}

NS_IMETHODIMP
morkStore::GetUsagePolicy(nsIMdbEnv* mev, 
  mdbUsagePolicy* ioUsagePolicy)
{
  MORK_USED_1(ioUsagePolicy);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkStore::SetUsagePolicy(nsIMdbEnv* mev, 
  const mdbUsagePolicy* inUsagePolicy)
{
  MORK_USED_1(inUsagePolicy);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}



NS_IMETHODIMP
morkStore::IdleMemoryPurge( 
  nsIMdbEnv* mev, 
  mdb_size* outEstimatedBytesFreed) 
{
  mdb_err outErr = 0;
  mdb_size estimatedBytesFreed = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  if ( outEstimatedBytesFreed )
    *outEstimatedBytesFreed = estimatedBytesFreed;
  return outErr;
}

NS_IMETHODIMP
morkStore::SessionMemoryPurge( 
  nsIMdbEnv* mev, 
  mdb_size inDesiredBytesFreed, 
  mdb_size* outEstimatedBytesFreed) 
{
  MORK_USED_1(inDesiredBytesFreed);
  mdb_err outErr = 0;
  mdb_size estimate = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  if ( outEstimatedBytesFreed )
    *outEstimatedBytesFreed = estimate;
  return outErr;
}

NS_IMETHODIMP
morkStore::PanicMemoryPurge( 
  nsIMdbEnv* mev, 
  mdb_size* outEstimatedBytesFreed) 
{
  mdb_err outErr = 0;
  mdb_size estimate = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  if ( outEstimatedBytesFreed )
    *outEstimatedBytesFreed = estimate;
  return outErr;
}



NS_IMETHODIMP
morkStore::GetPortFilePath(
  nsIMdbEnv* mev, 
  mdbYarn* outFilePath, 
  mdbYarn* outFormatVersion) 
{
  mdb_err outErr = 0;
  if ( outFormatVersion )
    outFormatVersion->mYarn_Fill = 0;
  if ( outFilePath )
    outFilePath->mYarn_Fill = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( mStore_File )
      mStore_File->Path(mev, outFilePath);
    else
      NilStoreFileError(ev);
    
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkStore::GetPortFile(
  nsIMdbEnv* mev, 
  nsIMdbFile** acqFile) 
{
  mdb_err outErr = 0;
  if ( acqFile )
    *acqFile = 0;

  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    if ( mStore_File )
    {
      if ( acqFile )
      {
        mStore_File->AddRef();
        if ( ev->Good() )
          *acqFile = mStore_File;
      }
    }
    else
      NilStoreFileError(ev);
      
    outErr = ev->AsErr();
  }
  return outErr;
}



NS_IMETHODIMP
morkStore::BestExportFormat( 
  nsIMdbEnv* mev, 
  mdbYarn* outFormatVersion) 
{
  mdb_err outErr = 0;
  if ( outFormatVersion )
    outFormatVersion->mYarn_Fill = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkStore::CanExportToFormat( 
  nsIMdbEnv* mev, 
  const char* inFormatVersion, 
  mdb_bool* outCanExport) 
{
  MORK_USED_1(inFormatVersion);
  mdb_bool canExport = morkBool_kFalse;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outCanExport )
    *outCanExport = canExport;
  return outErr;
}

NS_IMETHODIMP
morkStore::ExportToFormat( 
  nsIMdbEnv* mev, 
  
  nsIMdbFile* ioFile, 
  const char* inFormatVersion, 
  nsIMdbThumb** acqThumb) 


{
  mdb_err outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioFile && inFormatVersion && acqThumb )
    {
      ev->StubMethodOnlyError();
    }
    else
      ev->NilPointerError();
    
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
  return outErr;
}




NS_IMETHODIMP
morkStore::TokenToString( 
  nsIMdbEnv* mev, 
  mdb_token inToken, 
  mdbYarn* outTokenName) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    TokenToString(ev, inToken, outTokenName);
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkStore::StringToToken( 
  nsIMdbEnv* mev, 
  const char* inTokenName, 
  mdb_token* outToken) 
  
  
  
  
{
  mdb_err outErr = 0;
  mdb_token token = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    token = StringToToken(ev, inTokenName);
    outErr = ev->AsErr();
  }
  if ( outToken )
    *outToken = token;
  return outErr;
}
  

NS_IMETHODIMP
morkStore::QueryToken( 
  nsIMdbEnv* mev, 
  const char* inTokenName, 
  mdb_token* outToken) 
  
  
  
{
  mdb_err outErr = 0;
  mdb_token token = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    token = QueryToken(ev, inTokenName);
    outErr = ev->AsErr();
  }
  if ( outToken )
    *outToken = token;
  return outErr;
}





NS_IMETHODIMP
morkStore::HasRow( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  mdb_bool* outHasRow) 
{
  mdb_err outErr = 0;
  mdb_bool hasRow = morkBool_kFalse;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRow* row = GetRow(ev, inOid);
    if ( row )
      hasRow = morkBool_kTrue;
      
    outErr = ev->AsErr();
  }
  if ( outHasRow )
    *outHasRow = hasRow;
  return outErr;
}
  
NS_IMETHODIMP
morkStore::GetRow( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  nsIMdbRow** acqRow) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRow* row = GetRow(ev, inOid);
    if ( row && ev->Good() )
      outRow = row->AcquireRowHandle(ev, this);
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

NS_IMETHODIMP
morkStore::GetRowRefCount( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  mdb_count* outRefCount) 
{
  mdb_err outErr = 0;
  mdb_count count = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRow* row = GetRow(ev, inOid);
    if ( row && ev->Good() )
      count = row->mRow_GcUses;
      
    outErr = ev->AsErr();
  }
  if ( outRefCount )
    *outRefCount = count;
  return outErr;
}

NS_IMETHODIMP
morkStore::FindRow(nsIMdbEnv* mev, 
    mdb_scope inRowScope,   
    mdb_column inColumn,   
    const mdbYarn* inTargetCellValue, 
    mdbOid* outRowOid, 
    nsIMdbRow** acqRow) 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  mdbOid rowOid;
  rowOid.mOid_Scope = 0;
  rowOid.mOid_Id = (mdb_id) -1;
  
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRow* row = FindRow(ev, inRowScope, inColumn, inTargetCellValue);
    if ( row && ev->Good() )
    {
      rowOid = row->mRow_Oid;
      if ( acqRow )
          outRow = row->AcquireRowHandle(ev, this);
    }
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  if ( outRowOid )
    *outRowOid = rowOid;

  return outErr;
}




NS_IMETHODIMP
morkStore::HasTable( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  mdb_bool* outHasTable) 
{
  mdb_err outErr = 0;
  mork_bool hasTable = morkBool_kFalse;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = GetTable(ev, inOid);
    if ( table )
      hasTable = morkBool_kTrue;
    
    outErr = ev->AsErr();
  }
  if ( outHasTable )
    *outHasTable = hasTable;
  return outErr;
}
  
NS_IMETHODIMP
morkStore::GetTable( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  nsIMdbTable** acqTable) 
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = GetTable(ev, inOid);
    if ( table && ev->Good() )
      outTable = table->AcquireTableHandle(ev);
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}

NS_IMETHODIMP
morkStore::HasTableKind( 
  nsIMdbEnv* mev, 
  mdb_scope inRowScope, 
  mdb_kind inTableKind, 
  mdb_count* outTableCount, 
  mdb_bool* outSupportsTable) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    *outSupportsTable = HasTableKind(ev, inRowScope,
        inTableKind, outTableCount);
    outErr = ev->AsErr();
  }
  return outErr;
}
      
NS_IMETHODIMP
morkStore::GetTableKind( 
  nsIMdbEnv* mev, 
  mdb_scope inRowScope,      
  mdb_kind inTableKind,      
  mdb_count* outTableCount, 
  mdb_bool* outMustBeUnique, 
  nsIMdbTable** acqTable)      
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = GetTableKind(ev, inRowScope,
        inTableKind, outTableCount, outMustBeUnique);
    if ( table && ev->Good() )
      outTable = table->AcquireTableHandle(ev);
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}
  
NS_IMETHODIMP
morkStore::GetPortTableCursor( 
  nsIMdbEnv* mev, 
  mdb_scope inRowScope, 
  mdb_kind inTableKind, 
  nsIMdbPortTableCursor** acqCursor) 
{
  mdb_err outErr = 0;
  nsIMdbPortTableCursor* outCursor = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkPortTableCursor* cursor =
      GetPortTableCursor(ev, inRowScope,
        inTableKind);
    if ( cursor && ev->Good() )
      outCursor = cursor;

    outErr = ev->AsErr();
  }
  if ( acqCursor )
    *acqCursor = outCursor;
  return outErr;
}



  
NS_IMETHODIMP
morkStore::ShouldCompress( 
  nsIMdbEnv* mev, 
  mdb_percent inPercentWaste, 
  mdb_percent* outActualWaste, 
  mdb_bool* outShould) 
{
  mdb_percent actualWaste = 0;
  mdb_bool shouldCompress = morkBool_kFalse;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    actualWaste = PercentOfStoreWasted(ev);
    if ( inPercentWaste > 100 )
      inPercentWaste = 100;
    shouldCompress = ( actualWaste >= inPercentWaste );
    outErr = ev->AsErr();
  }
  if ( outActualWaste )
    *outActualWaste = actualWaste;
  if ( outShould )
    *outShould = shouldCompress;
  return outErr;
}




NS_IMETHODIMP
morkStore::NewTable( 
  nsIMdbEnv* mev, 
  mdb_scope inRowScope,    
  mdb_kind inTableKind,    
  mdb_bool inMustBeUnique, 
  const mdbOid* inOptionalMetaRowOid, 
  nsIMdbTable** acqTable)     
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = NewTable(ev, inRowScope,
        inTableKind, inMustBeUnique, inOptionalMetaRowOid);
    if ( table && ev->Good() )
      outTable = table->AcquireTableHandle(ev);
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}

NS_IMETHODIMP
morkStore::NewTableWithOid( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,   
  mdb_kind inTableKind,    
  mdb_bool inMustBeUnique, 
  const mdbOid* inOptionalMetaRowOid, 
  nsIMdbTable** acqTable)     
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev = CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = OidToTable(ev, inOid,
      inOptionalMetaRowOid);
    if ( table && ev->Good() )
    {
      table->mTable_Kind = inTableKind;
      if ( inMustBeUnique )
        table->SetTableUnique();
      outTable = table->AcquireTableHandle(ev);
    }
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}



NS_IMETHODIMP
morkStore::RowScopeHasAssignedIds(nsIMdbEnv* mev,
  mdb_scope inRowScope,   
  mdb_bool* outCallerAssigned, 
  mdb_bool* outStoreAssigned) 
{
  NS_ASSERTION(PR_FALSE, " not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkStore::SetCallerAssignedIds(nsIMdbEnv* mev,
  mdb_scope inRowScope,   
  mdb_bool* outCallerAssigned, 
  mdb_bool* outStoreAssigned) 
{
  NS_ASSERTION(PR_FALSE, " not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkStore::SetStoreAssignedIds(nsIMdbEnv* mev,
  mdb_scope inRowScope,   
  mdb_bool* outCallerAssigned, 
  mdb_bool* outStoreAssigned) 
{
  NS_ASSERTION(PR_FALSE, " not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
morkStore::NewRowWithOid(nsIMdbEnv* mev, 
  const mdbOid* inOid,   
  nsIMdbRow** acqRow) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRow* row = NewRowWithOid(ev, inOid);
    if ( row && ev->Good() )
      outRow = row->AcquireRowHandle(ev, this);
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

NS_IMETHODIMP
morkStore::NewRow(nsIMdbEnv* mev, 
  mdb_scope inRowScope,   
  nsIMdbRow** acqRow) 


{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRow* row = NewRow(ev, inRowScope);
    if ( row && ev->Good() )
      outRow = row->AcquireRowHandle(ev, this);
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}



NS_IMETHODIMP
morkStore::ImportContent( 
  nsIMdbEnv* mev, 
  mdb_scope inRowScope, 
  nsIMdbPort* ioPort, 
  nsIMdbThumb** acqThumb) 


{
  NS_ASSERTION(PR_FALSE, " not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkStore::ImportFile( 
  nsIMdbEnv* mev, 
  nsIMdbFile* ioFile, 
  nsIMdbThumb** acqThumb) 


{
  NS_ASSERTION(PR_FALSE, " not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
morkStore::ShareAtomColumnsHint( 
  nsIMdbEnv* mev, 
  mdb_scope inScopeHint, 
  const mdbColumnSet* inColumnSet) 
{
  MORK_USED_2(inColumnSet,inScopeHint);
  mdb_err outErr = 0;
  morkEnv* ev = CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkStore::AvoidAtomColumnsHint( 
  nsIMdbEnv* mev, 
  const mdbColumnSet* inColumnSet) 
{
  MORK_USED_1(inColumnSet);
  mdb_err outErr = 0;
  morkEnv* ev = CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}



NS_IMETHODIMP
morkStore::SmallCommit( 
  nsIMdbEnv* mev) 
{
  mdb_err outErr = 0;
  morkEnv* ev = CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkStore::LargeCommit( 
  nsIMdbEnv* mev, 
  nsIMdbThumb** acqThumb) 




{
  nsresult outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    morkThumb* thumb = 0;
    
    if ( DoPreferLargeOverCompressCommit(ev) )
    {
      thumb = morkThumb::Make_LargeCommit(ev, mPort_Heap, this);
    }
    else
    {
      mork_bool doCollect = morkBool_kFalse;
      thumb = morkThumb::Make_CompressCommit(ev, mPort_Heap, this, doCollect);
    }
    
    if ( thumb )
    {
      outThumb = thumb;
      thumb->AddRef();
    }
      
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
  return outErr;
}

NS_IMETHODIMP
morkStore::SessionCommit( 
  nsIMdbEnv* mev, 
  nsIMdbThumb** acqThumb) 




{
  nsresult outErr = NS_OK;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    morkThumb* thumb = 0;
    if ( DoPreferLargeOverCompressCommit(ev) )
    {
      thumb = morkThumb::Make_LargeCommit(ev, mPort_Heap, this);
    }
    else
    {
      mork_bool doCollect = morkBool_kFalse;
      thumb = morkThumb::Make_CompressCommit(ev, mPort_Heap, this, doCollect);
    }
    
    if ( thumb )
    {
      outThumb = thumb;
      thumb->AddRef();
    }
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
  return outErr;
}

NS_IMETHODIMP
morkStore::CompressCommit( 
  nsIMdbEnv* mev, 
  nsIMdbThumb** acqThumb) 




{
  nsresult outErr = NS_OK;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    mork_bool doCollect = morkBool_kFalse;
    morkThumb* thumb = morkThumb::Make_CompressCommit(ev, mPort_Heap, this, doCollect);
    if ( thumb )
    {
      outThumb = thumb;
      thumb->AddRef();
      mStore_CanWriteIncremental = morkBool_kTrue;
    }
      
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
  return outErr;
}








