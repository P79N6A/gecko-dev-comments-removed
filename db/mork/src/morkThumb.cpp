




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKTHUMB_
#include "morkThumb.h"
#endif

#ifndef _ORKINTHUMB_
#include "orkinThumb.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif





#ifndef _MORKWRITER_
#include "morkWriter.h"
#endif

#ifndef _MORKPARSER_
#include "morkParser.h"
#endif

#ifndef _MORKBUILDER_
#include "morkBuilder.h"
#endif






 void
morkThumb::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseThumb(ev);
    this->MarkShut();
  }
}


morkThumb::~morkThumb() 
{
  CloseMorkNode(mMorkEnv);
  MORK_ASSERT(mThumb_Magic==0);
  MORK_ASSERT(mThumb_Store==0);
  MORK_ASSERT(mThumb_File==0);
}


morkThumb::morkThumb(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap,
  nsIMdbHeap* ioSlotHeap, mork_magic inMagic)
: morkObject(ev, inUsage, ioHeap, morkColor_kNone, (morkHandle*) 0)
, mThumb_Magic( 0 )
, mThumb_Total( 0 )
, mThumb_Current( 0 )

, mThumb_Done( morkBool_kFalse )
, mThumb_Broken( morkBool_kFalse )
, mThumb_Seed( 0 )

, mThumb_Store( 0 )
, mThumb_File( 0 )
, mThumb_Writer( 0 )
, mThumb_Builder( 0 )
, mThumb_SourcePort( 0 )

, mThumb_DoCollect( morkBool_kFalse )
{
  if ( ev->Good() )
  {
    if ( ioSlotHeap )
    {
      mThumb_Magic = inMagic;
      mNode_Derived = morkDerived_kThumb;
    }
    else
      ev->NilPointerError();
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(morkThumb, morkObject, nsIMdbThumb)

 void
morkThumb::CloseThumb(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      mThumb_Magic = 0;
      if ( mThumb_Builder && mThumb_Store )
        mThumb_Store->ForgetBuilder(ev);
      morkBuilder::SlotStrongBuilder((morkBuilder*) 0, ev, &mThumb_Builder);
      
      morkWriter::SlotStrongWriter((morkWriter*) 0, ev, &mThumb_Writer);
      nsIMdbFile_SlotStrongFile((nsIMdbFile*) 0, ev, &mThumb_File);
      morkStore::SlotStrongStore((morkStore*) 0, ev, &mThumb_Store);
      morkStore::SlotStrongPort((morkPort*) 0, ev, &mThumb_SourcePort);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}





NS_IMETHODIMP
morkThumb::GetProgress(nsIMdbEnv* mev, mdb_count* outTotal,
  mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    GetProgress(ev, outTotal, outCurrent, outDone, outBroken);
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkThumb::DoMore(nsIMdbEnv* mev, mdb_count* outTotal,
  mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    DoMore(ev, outTotal, outCurrent, outDone, outBroken);
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkThumb::CancelAndBreakThumb(nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    mThumb_Done = morkBool_kTrue;
    mThumb_Broken = morkBool_kTrue;
    CloseMorkNode(ev); 
    outErr = ev->AsErr();
  }
  return outErr;
}


 void morkThumb::NonThumbTypeError(morkEnv* ev)
{
  ev->NewError("non morkThumb");
}

 void morkThumb::UnsupportedThumbMagicError(morkEnv* ev)
{
  ev->NewError("unsupported mThumb_Magic");
}

 void morkThumb::NilThumbStoreError(morkEnv* ev)
{
  ev->NewError("nil mThumb_Store");
}

 void morkThumb::NilThumbFileError(morkEnv* ev)
{
  ev->NewError("nil mThumb_File");
}

 void morkThumb::NilThumbWriterError(morkEnv* ev)
{
  ev->NewError("nil mThumb_Writer");
}

 void morkThumb::NilThumbBuilderError(morkEnv* ev)
{
  ev->NewError("nil mThumb_Builder");
}

 void morkThumb::NilThumbSourcePortError(morkEnv* ev)
{
  ev->NewError("nil mThumb_SourcePort");
}

 morkThumb*
morkThumb::Make_OpenFileStore(morkEnv* ev, nsIMdbHeap* ioHeap, 
  morkStore* ioStore)
{
  morkThumb* outThumb = 0;
  if ( ioHeap && ioStore )
  {
    nsIMdbFile* file = ioStore->mStore_File;
    if ( file )
    {
      mork_pos fileEof = 0;
      file->Eof(ev->AsMdbEnv(), &fileEof);
      if ( ev->Good() )
      {
        outThumb = new(*ioHeap, ev)
          morkThumb(ev, morkUsage::kHeap, ioHeap, ioHeap,
            morkThumb_kMagic_OpenFileStore);
            
        if ( outThumb )
        {
          morkBuilder* builder = ioStore->LazyGetBuilder(ev);
          if ( builder )
          {
            outThumb->mThumb_Total = (mork_count) fileEof;
            morkStore::SlotStrongStore(ioStore, ev, &outThumb->mThumb_Store);
            morkBuilder::SlotStrongBuilder(builder, ev,
              &outThumb->mThumb_Builder);
          }
        }
      }
    }
    else
      ioStore->NilStoreFileError(ev);
  }
  else
    ev->NilPointerError();
    
  return outThumb;
}


 morkThumb*
morkThumb::Make_LargeCommit(morkEnv* ev, 
  nsIMdbHeap* ioHeap, morkStore* ioStore)
{
  morkThumb* outThumb = 0;
  if ( ioHeap && ioStore )
  {
    nsIMdbFile* file = ioStore->mStore_File;
    if ( file )
    {
      outThumb = new(*ioHeap, ev)
        morkThumb(ev, morkUsage::kHeap, ioHeap, ioHeap,
          morkThumb_kMagic_LargeCommit);
          
      if ( outThumb )
      {
        morkWriter* writer = new(*ioHeap, ev)
          morkWriter(ev, morkUsage::kHeap, ioHeap, ioStore, file, ioHeap);
        if ( writer )
        {
          writer->mWriter_CommitGroupIdentity =
            ++ioStore->mStore_CommitGroupIdentity;
          writer->mWriter_NeedDirtyAll = morkBool_kFalse;
          outThumb->mThumb_DoCollect = morkBool_kFalse;
          morkStore::SlotStrongStore(ioStore, ev, &outThumb->mThumb_Store);
          
          nsIMdbFile_SlotStrongFile(file, ev, &outThumb->mThumb_File);
          
          outThumb->mThumb_Writer = writer; 
        }
      }
    }
    else
      ioStore->NilStoreFileError(ev);
  }
  else
    ev->NilPointerError();
    
  return outThumb;
}

 morkThumb*
morkThumb::Make_CompressCommit(morkEnv* ev, 
  nsIMdbHeap* ioHeap, morkStore* ioStore, mork_bool inDoCollect)
{
  morkThumb* outThumb = 0;
  if ( ioHeap && ioStore )
  {
    nsIMdbFile* file = ioStore->mStore_File;
    if ( file )
    {
      outThumb = new(*ioHeap, ev)
        morkThumb(ev, morkUsage::kHeap, ioHeap, ioHeap,
          morkThumb_kMagic_CompressCommit);
          
      if ( outThumb )
      {
        morkWriter* writer = new(*ioHeap, ev)
          morkWriter(ev, morkUsage::kHeap, ioHeap, ioStore, file, ioHeap);
        if ( writer )
        {
          writer->mWriter_NeedDirtyAll = morkBool_kTrue;
          outThumb->mThumb_DoCollect = inDoCollect;
          morkStore::SlotStrongStore(ioStore, ev, &outThumb->mThumb_Store);
          nsIMdbFile_SlotStrongFile(file, ev, &outThumb->mThumb_File);
          outThumb->mThumb_Writer = writer; 
          
          
          ioStore->mStore_FirstCommitGroupPos = 0;
          ioStore->mStore_SecondCommitGroupPos = 0;
        }
      }
    }
    else
      ioStore->NilStoreFileError(ev);
  }
  else
    ev->NilPointerError();
    
  return outThumb;
}


void morkThumb::GetProgress(morkEnv* ev, mdb_count* outTotal,
  mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken)
{
  MORK_USED_1(ev);
  if ( outTotal )
    *outTotal = mThumb_Total;
  if ( outCurrent )
    *outCurrent = mThumb_Current;
  if ( outDone )
    *outDone = mThumb_Done;
  if ( outBroken )
    *outBroken = mThumb_Broken;
}

void morkThumb::DoMore(morkEnv* ev, mdb_count* outTotal,
  mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken)
{
  if ( !mThumb_Done && !mThumb_Broken )
  {
    switch ( mThumb_Magic )
    {
      case morkThumb_kMagic_OpenFilePort: 
        this->DoMore_OpenFilePort(ev); break;

      case morkThumb_kMagic_OpenFileStore: 
        this->DoMore_OpenFileStore(ev); break;

      case morkThumb_kMagic_ExportToFormat: 
        this->DoMore_ExportToFormat(ev); break;

      case morkThumb_kMagic_ImportContent: 
        this->DoMore_ImportContent(ev); break;

      case morkThumb_kMagic_LargeCommit: 
        this->DoMore_LargeCommit(ev); break;

      case morkThumb_kMagic_SessionCommit: 
        this->DoMore_SessionCommit(ev); break;

      case morkThumb_kMagic_CompressCommit: 
        this->DoMore_CompressCommit(ev); break;

      case morkThumb_kMagic_SearchManyColumns: 
        this->DoMore_SearchManyColumns(ev); break;

      case morkThumb_kMagic_NewSortColumn: 
        this->DoMore_NewSortColumn(ev); break;

      case morkThumb_kMagic_NewSortColumnWithCompare: 
        this->DoMore_NewSortColumnWithCompare(ev); break;

      case morkThumb_kMagic_CloneSortColumn: 
        this->DoMore_CloneSortColumn(ev); break;

      case morkThumb_kMagic_AddIndex: 
        this->DoMore_AddIndex(ev); break;

      case morkThumb_kMagic_CutIndex: 
        this->DoMore_CutIndex(ev); break;

      default:
        this->UnsupportedThumbMagicError(ev);
        break;
    }
  }
  if ( outTotal )
    *outTotal = mThumb_Total;
  if ( outCurrent )
    *outCurrent = mThumb_Current;
  if ( outDone )
    *outDone = mThumb_Done;
  if ( outBroken )
    *outBroken = mThumb_Broken;
}

void morkThumb::CancelAndBreakThumb(morkEnv* ev)
{
  MORK_USED_1(ev);
  mThumb_Broken = morkBool_kTrue;
}



morkStore*
morkThumb::ThumbToOpenStore(morkEnv* ev)

{
  MORK_USED_1(ev);
  return mThumb_Store;
}

void morkThumb::DoMore_OpenFilePort(morkEnv* ev)
{
  this->UnsupportedThumbMagicError(ev);
}

void morkThumb::DoMore_OpenFileStore(morkEnv* ev)
{
  morkBuilder* builder = mThumb_Builder;
  if ( builder )
  {
    mork_pos pos = 0;
    builder->ParseMore(ev, &pos, &mThumb_Done, &mThumb_Broken);
    
    
    mThumb_Current = (mork_count) pos;
  }
  else
  {
    this->NilThumbBuilderError(ev);
    mThumb_Broken = morkBool_kTrue;
    mThumb_Done = morkBool_kTrue;
  }
}

void morkThumb::DoMore_ExportToFormat(morkEnv* ev)
{
  this->UnsupportedThumbMagicError(ev);
}

void morkThumb::DoMore_ImportContent(morkEnv* ev)
{
  this->UnsupportedThumbMagicError(ev);
}

void morkThumb::DoMore_LargeCommit(morkEnv* ev)
{
  this->DoMore_Commit(ev);
}

void morkThumb::DoMore_SessionCommit(morkEnv* ev)
{
  this->DoMore_Commit(ev);
}

void morkThumb::DoMore_Commit(morkEnv* ev)
{
  morkWriter* writer = mThumb_Writer;
  if ( writer )
  {
    writer->WriteMore(ev);
    mThumb_Total = writer->mWriter_TotalCount;
    mThumb_Current = writer->mWriter_DoneCount;
    mThumb_Done = ( ev->Bad() || writer->IsWritingDone() );
    mThumb_Broken = ev->Bad();
  }
  else
  {
    this->NilThumbWriterError(ev);
    mThumb_Broken = morkBool_kTrue;
    mThumb_Done = morkBool_kTrue;
  }
}

void morkThumb::DoMore_CompressCommit(morkEnv* ev)
{
  this->DoMore_Commit(ev);
}

void morkThumb::DoMore_SearchManyColumns(morkEnv* ev)
{
  this->UnsupportedThumbMagicError(ev);
}

void morkThumb::DoMore_NewSortColumn(morkEnv* ev)
{
  this->UnsupportedThumbMagicError(ev);
}

void morkThumb::DoMore_NewSortColumnWithCompare(morkEnv* ev)
{
  this->UnsupportedThumbMagicError(ev);
}

void morkThumb::DoMore_CloneSortColumn(morkEnv* ev)
{
  this->UnsupportedThumbMagicError(ev);
}

void morkThumb::DoMore_AddIndex(morkEnv* ev)
{
  this->UnsupportedThumbMagicError(ev);
}

void morkThumb::DoMore_CutIndex(morkEnv* ev)
{
  this->UnsupportedThumbMagicError(ev);
}



