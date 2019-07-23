




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKBLOB_
#include "morkBlob.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKARRAY_
#include "morkWriter.h"
#endif





#ifndef _MORKSTREAM_
#include "morkStream.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKATOMSPACE_
#include "morkAtomSpace.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKROWMAP_
#include "morkRowMap.h"
#endif

#ifndef _MORKATOMMAP_
#include "morkAtomMap.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKCELL_
#include "morkCell.h"
#endif

#ifndef _MORKATOM_
#include "morkAtom.h"
#endif

#ifndef _MORKCH_
#include "morkCh.h"
#endif






 void
morkWriter::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseWriter(ev);
    this->MarkShut();
  }
}


morkWriter::~morkWriter() 
{
  MORK_ASSERT(this->IsShutNode());
  MORK_ASSERT(mWriter_Store==0);
}


morkWriter::morkWriter(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, morkStore* ioStore, nsIMdbFile* ioFile,
    nsIMdbHeap* ioSlotHeap)
: morkNode(ev, inUsage, ioHeap)
, mWriter_Store( 0 )
, mWriter_File( 0 )
, mWriter_Bud( 0 )
, mWriter_Stream( 0 )
, mWriter_SlotHeap( 0 )

, mWriter_CommitGroupIdentity( 0 ) 
, mWriter_GroupBufFill( 0 )

, mWriter_TotalCount( morkWriter_kCountNumberOfPhases )
, mWriter_DoneCount( 0 )

, mWriter_LineSize( 0 )
, mWriter_MaxIndent( morkWriter_kMaxIndent )
, mWriter_MaxLine( morkWriter_kMaxLine )
  
, mWriter_TableForm( 0 )
, mWriter_TableAtomScope( 'v' )
, mWriter_TableRowScope( 0 )
, mWriter_TableKind( 0 )
  
, mWriter_RowForm( 0 )
, mWriter_RowAtomScope( 0 )
, mWriter_RowScope( 0 )
  
, mWriter_DictForm( 0 )
, mWriter_DictAtomScope( 'v' )

, mWriter_NeedDirtyAll( morkBool_kFalse )
, mWriter_Incremental( morkBool_kTrue ) 
, mWriter_DidStartDict( morkBool_kFalse )
, mWriter_DidEndDict( morkBool_kTrue )

, mWriter_SuppressDirtyRowNewline( morkBool_kFalse )
, mWriter_DidStartGroup( morkBool_kFalse )
, mWriter_DidEndGroup( morkBool_kTrue )
, mWriter_Phase( morkWriter_kPhaseNothingDone )

, mWriter_BeVerbose( ev->mEnv_BeVerbose )

, mWriter_TableRowArrayPos( 0 )


, mWriter_StoreAtomSpacesIter( )
, mWriter_AtomSpaceAtomAidsIter( )
  
, mWriter_StoreRowSpacesIter( )
, mWriter_RowSpaceTablesIter( )
, mWriter_RowSpaceRowsIter( )
{
  mWriter_GroupBuf[ 0 ] = 0;

  mWriter_SafeNameBuf[ 0 ] = 0;
  mWriter_SafeNameBuf[ morkWriter_kMaxColumnNameSize * 2 ] = 0;
  mWriter_ColNameBuf[ 0 ] = 0;
  mWriter_ColNameBuf[ morkWriter_kMaxColumnNameSize ] = 0;
  
  mdbYarn* y = &mWriter_ColYarn;
  y->mYarn_Buf = mWriter_ColNameBuf; 
  y->mYarn_Fill = 0; 
  y->mYarn_Size = morkWriter_kMaxColumnNameSize; 
  y->mYarn_More = 0; 
  y->mYarn_Form = 0; 
  y->mYarn_Grow = 0; 
  
  y = &mWriter_SafeYarn;
  y->mYarn_Buf = mWriter_SafeNameBuf; 
  y->mYarn_Fill = 0; 
  y->mYarn_Size = morkWriter_kMaxColumnNameSize * 2; 
  y->mYarn_More = 0; 
  y->mYarn_Form = 0; 
  y->mYarn_Grow = 0; 
  
  if ( ev->Good() )
  {
    if ( ioSlotHeap && ioFile && ioStore )
    {
      morkStore::SlotWeakStore(ioStore, ev, &mWriter_Store);
      nsIMdbFile_SlotStrongFile(ioFile, ev, &mWriter_File);
      nsIMdbHeap_SlotStrongHeap(ioSlotHeap, ev, &mWriter_SlotHeap);
      if ( ev->Good() )
      {
        mNode_Derived = morkDerived_kWriter;
      }
    }
    else
      ev->NilPointerError();
  }
}


void
morkWriter::MakeWriterStream(morkEnv* ev) 
{
  mWriter_Incremental = !mWriter_NeedDirtyAll; 
  
  if ( !mWriter_Stream && ev->Good() )
  {
    if ( mWriter_File )
    {
      morkStream* stream = 0;
      mork_bool frozen = morkBool_kFalse; 
      nsIMdbHeap* heap = mWriter_SlotHeap;
    
      if ( mWriter_Incremental )
      {
        stream = new(*heap, ev)
          morkStream(ev, morkUsage::kHeap, heap, mWriter_File,
            morkWriter_kStreamBufSize, frozen);
      }
      else 
      {
        nsIMdbFile* bud = 0;
        mWriter_File->AcquireBud(ev->AsMdbEnv(), heap, &bud);
        if ( bud )
        {
          if ( ev->Good() )
          {
            mWriter_Bud = bud;
            stream = new(*heap, ev)
              morkStream(ev, morkUsage::kHeap, heap, bud,
                morkWriter_kStreamBufSize, frozen);
          }
          else
            bud->Release();
        }
      }
        
      if ( stream )
      {
        if ( ev->Good() )
          mWriter_Stream = stream;
        else
          stream->CutStrongRef(ev->AsMdbEnv());
      }
    }
    else
      this->NilWriterFileError(ev);
  }
}

 void
morkWriter::CloseWriter(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      morkStore::SlotWeakStore((morkStore*) 0, ev, &mWriter_Store);
      nsIMdbFile_SlotStrongFile((nsIMdbFile*) 0, ev, &mWriter_File);
      nsIMdbFile_SlotStrongFile((nsIMdbFile*) 0, ev, &mWriter_Bud);
      morkStream::SlotStrongStream((morkStream*) 0, ev, &mWriter_Stream);
      nsIMdbHeap_SlotStrongHeap((nsIMdbHeap*) 0, ev, &mWriter_SlotHeap);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkWriter::NonWriterTypeError(morkEnv* ev)
{
  ev->NewError("non morkWriter");
}

 void
morkWriter::NilWriterStoreError(morkEnv* ev)
{
  ev->NewError("nil mWriter_Store");
}

 void
morkWriter::NilWriterBudError(morkEnv* ev)
{
  ev->NewError("nil mWriter_Bud");
}

 void
morkWriter::NilWriterFileError(morkEnv* ev)
{
  ev->NewError("nil mWriter_File");
}

 void
morkWriter::NilWriterStreamError(morkEnv* ev)
{
  ev->NewError("nil mWriter_Stream");
}

 void
morkWriter::UnsupportedPhaseError(morkEnv* ev)
{
  ev->NewError("unsupported mWriter_Phase");
}

mork_bool
morkWriter::WriteMore(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    if ( this->IsWriter() )
    {
      if ( !mWriter_Stream )
        this->MakeWriterStream(ev);
        
      if ( mWriter_Stream )
      {
        if ( ev->Bad() )
        {
          ev->NewWarning("writing stops on error");
          mWriter_Phase = morkWriter_kPhaseWritingDone;
        }
        switch( mWriter_Phase )
        {
          case morkWriter_kPhaseNothingDone:
            OnNothingDone(ev); break;
          
          case morkWriter_kPhaseDirtyAllDone:
            OnDirtyAllDone(ev); break;
          
          case morkWriter_kPhasePutHeaderDone:
            OnPutHeaderDone(ev); break;
          
          case morkWriter_kPhaseRenumberAllDone:
            OnRenumberAllDone(ev); break;
          
          case morkWriter_kPhaseStoreAtomSpaces:
            OnStoreAtomSpaces(ev); break;
          
          case morkWriter_kPhaseAtomSpaceAtomAids:
            OnAtomSpaceAtomAids(ev); break;
          
          case morkWriter_kPhaseStoreRowSpacesTables:
            OnStoreRowSpacesTables(ev); break;
          
          case morkWriter_kPhaseRowSpaceTables:
            OnRowSpaceTables(ev); break;
          
          case morkWriter_kPhaseTableRowArray:
            OnTableRowArray(ev); break;
          
          case morkWriter_kPhaseStoreRowSpacesRows:
            OnStoreRowSpacesRows(ev); break;
          
          case morkWriter_kPhaseRowSpaceRows:
            OnRowSpaceRows(ev); break;
          
          case morkWriter_kPhaseContentDone:
            OnContentDone(ev); break;
          
          case morkWriter_kPhaseWritingDone:
            OnWritingDone(ev); break;
          
          default:
            this->UnsupportedPhaseError(ev);
        }
      }
      else
        this->NilWriterStreamError(ev);
    }
    else
      this->NonWriterTypeError(ev);
  }
  else
    this->NonOpenNodeError(ev);
    
  return ev->Good();
}

static const char morkWriter_kHexDigits[] = "0123456789ABCDEF";

mork_size
morkWriter::WriteYarn(morkEnv* ev, const mdbYarn* inYarn)
  
  
  
{


  mork_size outSize = 0;
  mork_size lineSize = mWriter_LineSize;
  morkStream* stream = mWriter_Stream;

  const mork_u1* b = (const mork_u1*) inYarn->mYarn_Buf;
  if ( b )
  {
    register int c;
    mork_fill fill = inYarn->mYarn_Fill;

    const mork_u1* end = b + fill;
    while ( b < end && ev->Good() )
    {
      if ( lineSize + outSize >= mWriter_MaxLine ) 
      {
        stream->PutByteThenNewline(ev, '\\');
        mWriter_LineSize = lineSize = outSize = 0;
      }
      
      c = *b++; 
      if ( morkCh_IsValue(c) )
      {
        stream->Putc(ev, c);
        ++outSize; 
      }
      else if ( c == ')' || c == '$' || c == '\\' )
      {
        stream->Putc(ev, '\\');
        stream->Putc(ev, c);
        outSize += 2; 
      }
      else
      {
        outSize += 3; 
        stream->Putc(ev, '$');
        stream->Putc(ev, morkWriter_kHexDigits[ (c >> 4) & 0x0F ]);
        stream->Putc(ev, morkWriter_kHexDigits[ c & 0x0F ]);
      }
    }
  }
  mWriter_LineSize += outSize;
    
  return outSize;
}

mork_size
morkWriter::WriteAtom(morkEnv* ev, const morkAtom* inAtom)
  
  
  
{
  mork_size outSize = 0;
  mdbYarn yarn; 

  if ( inAtom->AliasYarn(&yarn) )
  {
    if ( mWriter_DidStartDict && yarn.mYarn_Form != mWriter_DictForm )
      this->ChangeDictForm(ev, yarn.mYarn_Form);  
      
    outSize = this->WriteYarn(ev, &yarn);
    
  }
  else
    inAtom->BadAtomKindError(ev);
    
  return outSize;
}

void
morkWriter::WriteAtomSpaceAsDict(morkEnv* ev, morkAtomSpace* ioSpace)
{
  morkStream* stream = mWriter_Stream;
  nsIMdbEnv *mdbev = ev->AsMdbEnv();
  mork_scope scope = ioSpace->SpaceScope();
  if ( scope < 0x80 )
  {
    if ( mWriter_LineSize )
      stream->PutLineBreak(ev);
    stream->PutString(ev, "< <(a=");
    stream->Putc(ev, (int) scope);
    ++mWriter_LineSize;
    stream->PutString(ev, ")> // (f=iso-8859-1)");
    mWriter_LineSize = stream->PutIndent(ev, morkWriter_kDictAliasDepth);
  }
  else
    ioSpace->NonAsciiSpaceScopeName(ev);

  if ( ev->Good() )
  {
    mdbYarn yarn; 
    char buf[ 64 ]; 
    char* idBuf = buf + 1; 
    buf[ 0 ] = '('; 
    morkBookAtom* atom = 0;
    morkAtomAidMapIter* ai = &mWriter_AtomSpaceAtomAidsIter;
    ai->InitAtomAidMapIter(ev, &ioSpace->mAtomSpace_AtomAids);
    mork_change* c = 0;
    
    for ( c = ai->FirstAtom(ev, &atom); c && ev->Good();
          c = ai->NextAtom(ev, &atom) )
    {
      if ( atom )
      {
        if ( atom->IsAtomDirty() )
        {
          atom->SetAtomClean(); 
          
          atom->AliasYarn(&yarn);
          mork_size size = ev->TokenAsHex(idBuf, atom->mBookAtom_Id);
          
          if ( yarn.mYarn_Form != mWriter_DictForm )
            this->ChangeDictForm(ev, yarn.mYarn_Form);

          mork_size pending = yarn.mYarn_Fill + size + 
            morkWriter_kYarnEscapeSlop + 4;
          this->IndentOverMaxLine(ev, pending, morkWriter_kDictAliasDepth);
          mork_size bytesWritten;
          stream->Write(mdbev, buf, size+1, &bytesWritten); 
          mWriter_LineSize += bytesWritten;
          
          pending -= ( size + 1 );
          this->IndentOverMaxLine(ev, pending, morkWriter_kDictAliasValueDepth);
          stream->Putc(ev, '='); 
          ++mWriter_LineSize;
          
          this->WriteYarn(ev, &yarn);
          stream->Putc(ev, ')'); 
          ++mWriter_LineSize;
          
          ++mWriter_DoneCount;
        }
      }
      else
        ev->NilPointerError();
    }
    ai->CloseMapIter(ev);
  }
  
  if ( ev->Good() )
  {
    ioSpace->SetAtomSpaceClean();
    
    
    
    stream->Putc(ev, '>'); 
    ++mWriter_LineSize;
  }
}


















































mork_bool
morkWriter::DirtyAll(morkEnv* ev)
  
  
  
  
  
  
  
  
  
{
  morkStore* store = mWriter_Store;
  if ( store )
  {
    store->SetStoreDirty();
    mork_change* c = 0;

    if ( ev->Good() )
    {
      morkAtomSpaceMapIter* asi = &mWriter_StoreAtomSpacesIter;
      asi->InitAtomSpaceMapIter(ev, &store->mStore_AtomSpaces);

      mork_scope* key = 0; 
      morkAtomSpace* space = 0; 
      
      for ( c = asi->FirstAtomSpace(ev, key, &space); c && ev->Good();
            c = asi->NextAtomSpace(ev, key, &space) )
      {
        if ( space )
        {
          if ( space->IsAtomSpace() )
          {
            space->SetAtomSpaceDirty();
            morkBookAtom* atom = 0;
            morkAtomAidMapIter* ai = &mWriter_AtomSpaceAtomAidsIter;
            ai->InitAtomAidMapIter(ev, &space->mAtomSpace_AtomAids);
            
            for ( c = ai->FirstAtom(ev, &atom); c && ev->Good();
                  c = ai->NextAtom(ev, &atom) )
            {
              if ( atom )
              {
                atom->SetAtomDirty();
                ++mWriter_TotalCount;
              }
              else
                ev->NilPointerError();
            }
            
            ai->CloseMapIter(ev);
          }
          else
            space->NonAtomSpaceTypeError(ev);
        }
        else
          ev->NilPointerError();
      }
    }
    
    if ( ev->Good() )
    {
      morkRowSpaceMapIter* rsi = &mWriter_StoreRowSpacesIter;
      rsi->InitRowSpaceMapIter(ev, &store->mStore_RowSpaces);

      mork_scope* key = 0; 
      morkRowSpace* space = 0; 
      
      for ( c = rsi->FirstRowSpace(ev, key, &space); c && ev->Good();
            c = rsi->NextRowSpace(ev, key, &space) )
      {
        if ( space )
        {
          if ( space->IsRowSpace() )
          {
            space->SetRowSpaceDirty();
            if ( ev->Good() )
            {
#ifdef MORK_ENABLE_PROBE_MAPS
              morkRowProbeMapIter* ri = &mWriter_RowSpaceRowsIter;
#else 
              morkRowMapIter* ri = &mWriter_RowSpaceRowsIter;
#endif 
              ri->InitRowMapIter(ev, &space->mRowSpace_Rows);

              morkRow* row = 0; 
                
              for ( c = ri->FirstRow(ev, &row); c && ev->Good();
                    c = ri->NextRow(ev, &row) )
              {
                if ( row && row->IsRow() ) 
                {
                	if ( row->IsRowUsed() || row->IsRowDirty() )
                	{
	                  row->DirtyAllRowContent(ev);
	                  ++mWriter_TotalCount;
                	}
                }
                else
                  row->NonRowTypeWarning(ev);
              }
              ri->CloseMapIter(ev);
            }
            
            if ( ev->Good() )
            {
              morkTableMapIter* ti = &mWriter_RowSpaceTablesIter;
              ti->InitTableMapIter(ev, &space->mRowSpace_Tables);

#ifdef MORK_BEAD_OVER_NODE_MAPS
              morkTable* table = ti->FirstTable(ev);
                
              for ( ; table && ev->Good(); table = ti->NextTable(ev) )
#else 
              mork_tid* tableKey = 0; 
              morkTable* table = 0; 
                
              for ( c = ti->FirstTable(ev, tableKey, &table); c && ev->Good();
                    c = ti->NextTable(ev, tableKey, &table) )
#endif 
              {
                if ( table && table->IsTable() ) 
                {
                	if ( table->IsTableUsed() || table->IsTableDirty() )
                	{
	                  
	                  
	                  table->SetTableDirty();
	                  table->SetTableRewrite();
	                  ++mWriter_TotalCount;
                	}
                }
                else
                  table->NonTableTypeWarning(ev);
              }
              ti->CloseMapIter(ev);
            }
          }
          else
            space->NonRowSpaceTypeError(ev);
        }
        else
          ev->NilPointerError();
      }
    }
  }
  else
    this->NilWriterStoreError(ev);
  
  return ev->Good();
}


mork_bool
morkWriter::OnNothingDone(morkEnv* ev)
{
  mWriter_Incremental = !mWriter_NeedDirtyAll; 
  
  if (!mWriter_Store->IsStoreDirty() && !mWriter_NeedDirtyAll)
  {
    mWriter_Phase = morkWriter_kPhaseWritingDone;
    return morkBool_kTrue;
  }

  
  if ( mWriter_NeedDirtyAll )
    this->DirtyAll(ev);
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhaseDirtyAllDone;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 
    
  return ev->Good();
}

mork_bool
morkWriter::StartGroup(morkEnv* ev)
{
  nsIMdbEnv *mdbev = ev->AsMdbEnv();
  morkStream* stream = mWriter_Stream;
  mWriter_DidStartGroup = morkBool_kTrue;
  mWriter_DidEndGroup = morkBool_kFalse;

  char buf[ 64 ];
  char* p = buf;
  *p++ = '@';
  *p++ = '$';
  *p++ = '$';
  *p++ = '{';
  
  mork_token groupID = mWriter_CommitGroupIdentity;
  mork_fill idFill = ev->TokenAsHex(p, groupID);
  mWriter_GroupBufFill = 0;
  
  if ( idFill < morkWriter_kGroupBufSize )
  {
    MORK_MEMCPY(mWriter_GroupBuf, p, idFill + 1);
    mWriter_GroupBufFill = idFill;
  }
  else
    *mWriter_GroupBuf = 0;
    
  p += idFill;
  *p++ = '{';
  *p++ = '@';
  *p = 0;

  stream->PutLineBreak(ev);
  
  morkStore* store = mWriter_Store;
  if ( store ) 
  {
    mork_pos groupPos;
    stream->Tell(mdbev, &groupPos);
    if ( !store->mStore_FirstCommitGroupPos )
      store->mStore_FirstCommitGroupPos = groupPos;
    else if ( !store->mStore_SecondCommitGroupPos )
      store->mStore_SecondCommitGroupPos = groupPos;
  }
  
  mork_size bytesWritten;
  stream->Write(mdbev, buf, idFill + 6, &bytesWritten); 
  stream->PutLineBreak(ev);
  mWriter_LineSize = 0;
  
  return ev->Good();
}

mork_bool
morkWriter::CommitGroup(morkEnv* ev)
{
  if ( mWriter_DidStartGroup )
  {
    nsIMdbEnv *mdbev = ev->AsMdbEnv();
    mork_size bytesWritten;
    morkStream* stream = mWriter_Stream;
  
    if ( mWriter_LineSize )
      stream->PutLineBreak(ev);
      
    stream->Putc(ev, '@');
    stream->Putc(ev, '$');
    stream->Putc(ev, '$');
    stream->Putc(ev, '}');
    
    mork_fill bufFill = mWriter_GroupBufFill;
    if ( bufFill )
      stream->Write(mdbev, mWriter_GroupBuf, bufFill, &bytesWritten);

    stream->Putc(ev, '}');
    stream->Putc(ev, '@');
    stream->PutLineBreak(ev);

    mWriter_LineSize = 0;
  }

  mWriter_DidStartGroup = morkBool_kFalse;
  mWriter_DidEndGroup = morkBool_kTrue;
  
  return ev->Good();
}

mork_bool
morkWriter::AbortGroup(morkEnv* ev)
{
  if ( mWriter_DidStartGroup )
  {
    morkStream* stream = mWriter_Stream;
    stream->PutLineBreak(ev);
    stream->PutStringThenNewline(ev, "@$$}~~}@");
    mWriter_LineSize = 0;
  }
  
  mWriter_DidStartGroup = morkBool_kFalse;
  mWriter_DidEndGroup = morkBool_kTrue;

  return ev->Good();
}


mork_bool
morkWriter::OnDirtyAllDone(morkEnv* ev)
{
  if ( ev->Good() )
  {
    nsIMdbEnv *mdbev = ev->AsMdbEnv();
    morkStream* stream = mWriter_Stream;
    mork_pos resultPos;
    if ( mWriter_NeedDirtyAll ) 
    {

      stream->Seek(mdbev, 0, &resultPos); 
      stream->PutStringThenNewline(ev, morkWriter_kFileHeader);
      mWriter_LineSize = 0;
    }
    else 
    {
      mork_pos eos = stream->Length(ev); 
      if ( ev->Good() )
      {
        stream->Seek(mdbev, eos, &resultPos); 
        if ( eos < 128 ) 
        {
          stream->PutStringThenNewline(ev, morkWriter_kFileHeader);
          mWriter_LineSize = 0;
        }
        this->StartGroup(ev); 
      }
    }
  }
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhasePutHeaderDone;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 
    
  return ev->Good();
}

mork_bool
morkWriter::OnPutHeaderDone(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);
  
  
  
  mWriter_LineSize = 0;
  
  if ( mWriter_NeedDirtyAll ) 
  {
    morkStore* store = mWriter_Store;
    if ( store )
      store->RenumberAllCollectableContent(ev);
    else
      this->NilWriterStoreError(ev);
  }
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhaseRenumberAllDone;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 
    
  return ev->Good();
}

mork_bool
morkWriter::OnRenumberAllDone(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);
    
  
  
  mWriter_LineSize = 0;
  
  if ( mWriter_NeedDirtyAll ) 
  {
  }
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhaseStoreAtomSpaces;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 

  return ev->Good();
}

mork_bool
morkWriter::OnStoreAtomSpaces(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);

  
  
  mWriter_LineSize = 0;
  
  if ( mWriter_NeedDirtyAll ) 
  {
  }
  
  if ( ev->Good() )
  {
    morkStore* store = mWriter_Store;
    if ( store )
    {
      morkAtomSpace* space = store->LazyGetGroundColumnSpace(ev);
      if ( space && space->IsAtomSpaceDirty() )
      {
        
        
        if ( mWriter_LineSize )
        {
          stream->PutLineBreak(ev);
          mWriter_LineSize = 0;
        }
        this->WriteAtomSpaceAsDict(ev, space);
        space->SetAtomSpaceClean();
      }
    }
    else
      this->NilWriterStoreError(ev);
  }
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhaseStoreRowSpacesTables;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 

  return ev->Good();
}

mork_bool
morkWriter::OnAtomSpaceAtomAids(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);

  
  
  mWriter_LineSize = 0;
  
  if ( mWriter_NeedDirtyAll ) 
  {
  }
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhaseStoreRowSpacesTables;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 

  return ev->Good();
}

void
morkWriter::WriteAllStoreTables(morkEnv* ev)
{
  morkStore* store = mWriter_Store;
  if ( store && ev->Good() )
  {
    morkRowSpaceMapIter* rsi = &mWriter_StoreRowSpacesIter;
    rsi->InitRowSpaceMapIter(ev, &store->mStore_RowSpaces);

    mork_scope* key = 0; 
    morkRowSpace* space = 0; 
    mork_change* c = 0;
    
    for ( c = rsi->FirstRowSpace(ev, key, &space); c && ev->Good();
          c = rsi->NextRowSpace(ev, key, &space) )
    {
      if ( space )
      {
        if ( space->IsRowSpace() )
        {
          space->SetRowSpaceClean();
          if ( ev->Good() )
          {
            morkTableMapIter* ti = &mWriter_RowSpaceTablesIter;
            ti->InitTableMapIter(ev, &space->mRowSpace_Tables);

#ifdef MORK_BEAD_OVER_NODE_MAPS
            morkTable* table = ti->FirstTable(ev);
              
            for ( ; table && ev->Good(); table = ti->NextTable(ev) )
#else 
            mork_tid* key2 = 0; 
            morkTable* table = 0; 
              
            for ( c = ti->FirstTable(ev, key2, &table); c && ev->Good();
                  c = ti->NextTable(ev, key2, &table) )
#endif 
            {
              if ( table && table->IsTable() )
              {
                if ( table->IsTableDirty() )
                {
                  mWriter_BeVerbose =
                    ( ev->mEnv_BeVerbose || table->IsTableVerbose() );
                    
                  if ( this->PutTableDict(ev, table) )
                    this->PutTable(ev, table);

                  table->SetTableClean(ev);
                  mWriter_BeVerbose = ev->mEnv_BeVerbose;
                }
              }
              else
                table->NonTableTypeWarning(ev);
            }
            ti->CloseMapIter(ev);
          }
          if ( ev->Good() )
          {
            mWriter_TableRowScope = 0; 
            
#ifdef MORK_ENABLE_PROBE_MAPS
            morkRowProbeMapIter* ri = &mWriter_RowSpaceRowsIter;
#else 
            morkRowMapIter* ri = &mWriter_RowSpaceRowsIter;
#endif 
            ri->InitRowMapIter(ev, &space->mRowSpace_Rows);

            morkRow* row = 0; 
              
            for ( c = ri->FirstRow(ev, &row); c && ev->Good();
                  c = ri->NextRow(ev, &row) )
            {
              if ( row && row->IsRow() )
              {
                
                if ( row->IsRowDirty() ) 
                {
                  mWriter_BeVerbose = ev->mEnv_BeVerbose;
                  if ( this->PutRowDict(ev, row) )
                  {
                    if ( ev->Good() && mWriter_DidStartDict )
                    {
                      this->EndDict(ev);
                      if ( mWriter_LineSize < 32 && ev->Good() )
                        mWriter_SuppressDirtyRowNewline = morkBool_kTrue;
                    }
                      
                    if ( ev->Good() )
                      this->PutRow(ev, row);
                  }
                  mWriter_BeVerbose = ev->mEnv_BeVerbose;
                }
              }
              else
                row->NonRowTypeWarning(ev);
            }
            ri->CloseMapIter(ev);
          }
        }
        else
          space->NonRowSpaceTypeError(ev);
      }
      else
        ev->NilPointerError();
    }
  }
}

mork_bool
morkWriter::OnStoreRowSpacesTables(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);

  
  
  mWriter_LineSize = 0;
  
  if ( mWriter_NeedDirtyAll ) 
  {
  }
  
  
  this->WriteAllStoreTables(ev);
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhaseStoreRowSpacesRows;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 

  return ev->Good();
}

mork_bool
morkWriter::OnRowSpaceTables(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);

  
  
  mWriter_LineSize = 0;
  
  if ( mWriter_NeedDirtyAll ) 
  {
  }
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhaseStoreRowSpacesRows;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 

  return ev->Good();
}

mork_bool
morkWriter::OnTableRowArray(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);

  
  
  mWriter_LineSize = 0;
  
  if ( mWriter_NeedDirtyAll ) 
  {
  }
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhaseStoreRowSpacesRows;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 

  return ev->Good();
}

mork_bool
morkWriter::OnStoreRowSpacesRows(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);

  
  
  mWriter_LineSize = 0;
  
  if ( mWriter_NeedDirtyAll ) 
  {
  }
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhaseContentDone;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 

  return ev->Good();
}

mork_bool
morkWriter::OnRowSpaceRows(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);

  
  
  mWriter_LineSize = 0;
  
  if ( mWriter_NeedDirtyAll ) 
  {
  }
    
  if ( ev->Good() )
    mWriter_Phase = morkWriter_kPhaseContentDone;
  else
    mWriter_Phase = morkWriter_kPhaseWritingDone; 

  return ev->Good();
}

mork_bool
morkWriter::OnContentDone(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);

  
  
  mWriter_LineSize = 0;
  
  if ( mWriter_Incremental )
  {
    if ( ev->Good() )
      this->CommitGroup(ev);
    else
      this->AbortGroup(ev);
  }
  else if ( mWriter_Store && ev->Good() )
  {
    
    mWriter_Store->mStore_FirstCommitGroupPos = 0;
    mWriter_Store->mStore_SecondCommitGroupPos = 0;
  }
  
  stream->Flush(ev->AsMdbEnv());
  nsIMdbFile* bud = mWriter_Bud;
  if ( bud )
  {
    bud->Flush(ev->AsMdbEnv());
    bud->BecomeTrunk(ev->AsMdbEnv());
    nsIMdbFile_SlotStrongFile((nsIMdbFile*) 0, ev, &mWriter_Bud);
  }
  else if ( !mWriter_Incremental ) 
    this->NilWriterBudError(ev);
    
  mWriter_Phase = morkWriter_kPhaseWritingDone; 
  mWriter_DoneCount = mWriter_TotalCount;
  
  return ev->Good();
}

mork_bool
morkWriter::OnWritingDone(morkEnv* ev)
{
  mWriter_DoneCount = mWriter_TotalCount;
  ev->NewWarning("writing is done");
  return ev->Good();
}

mork_bool
morkWriter::PutTableChange(morkEnv* ev, const morkTableChange* inChange)
{
  nsIMdbEnv *mdbev = ev->AsMdbEnv();
  if ( inChange->IsAddRowTableChange() )
  {
    this->PutRow(ev, inChange->mTableChange_Row ); 
  }
  else if ( inChange->IsCutRowTableChange() )
  {
    mWriter_Stream->Putc(ev, '-'); 
    ++mWriter_LineSize;
    this->PutRow(ev, inChange->mTableChange_Row );
  }
  else if ( inChange->IsMoveRowTableChange() )
  {
    this->PutRow(ev, inChange->mTableChange_Row );
    char buf[ 64 ];
    char* p = buf;
    *p++ = '!'; 
    mork_size posSize = ev->TokenAsHex(p, inChange->mTableChange_Pos);
    p += posSize;
    *p++ = ' ';
    mork_size bytesWritten;
    mWriter_Stream->Write(mdbev, buf, posSize + 2, &bytesWritten);
    mWriter_LineSize += bytesWritten;
  }
  else
    inChange->UnknownChangeError(ev);
  
  return ev->Good();
}

mork_bool
morkWriter::PutTable(morkEnv* ev, morkTable* ioTable)
{
  if ( ev->Good() )
    this->StartTable(ev, ioTable);
    
  if ( ev->Good() )
  {
    if ( ioTable->IsTableRewrite() || mWriter_NeedDirtyAll )
    {
      morkArray* array = &ioTable->mTable_RowArray; 
      mork_fill fill = array->mArray_Fill; 
      morkRow** rows = (morkRow**) array->mArray_Slots;
      if ( rows && fill )
      {
        morkRow** end = rows + fill;
        while ( rows < end && ev->Good() )
        {
          morkRow* r = *rows++; 
          this->PutRow(ev, r);
        }
      }
    }
    else 
    {
      morkList* list = &ioTable->mTable_ChangeList;
      morkNext* next = list->GetListHead();
      while ( next && ev->Good() )
      {
        this->PutTableChange(ev, (morkTableChange*) next);
        next = next->GetNextLink();
      }
    }
  }
    
  if ( ev->Good() )
    this->EndTable(ev);
  
  ioTable->SetTableClean(ev); 
  mWriter_TableRowScope = 0;

  ++mWriter_DoneCount;
  return ev->Good();
}

mork_bool
morkWriter::PutTableDict(morkEnv* ev, morkTable* ioTable)
{
  morkRowSpace* space = ioTable->mTable_RowSpace;
  mWriter_TableRowScope = space->SpaceScope();
  mWriter_TableForm = 0;     
  mWriter_TableAtomScope = 'v'; 
  mWriter_TableKind = ioTable->mTable_Kind;
  
  mWriter_RowForm = mWriter_TableForm;
  mWriter_RowAtomScope = mWriter_TableAtomScope;
  mWriter_RowScope = mWriter_TableRowScope;
  
  mWriter_DictForm = mWriter_TableForm;
  mWriter_DictAtomScope = mWriter_TableAtomScope;
  
  
  

  if ( ev->Good() )
  {
    morkRow* r = ioTable->mTable_MetaRow;
    if ( r )
    {
      if ( r->IsRow() )
        this->PutRowDict(ev, r);
      else
        r->NonRowTypeError(ev);
    }
    morkArray* array = &ioTable->mTable_RowArray; 
    mork_fill fill = array->mArray_Fill; 
    morkRow** rows = (morkRow**) array->mArray_Slots;
    if ( rows && fill )
    {
      morkRow** end = rows + fill;
      while ( rows < end && ev->Good() )
      {
        r = *rows++; 
        if ( r && r->IsRow() )
          this->PutRowDict(ev, r);
        else
          r->NonRowTypeError(ev);
      }
    }
    
    
    
    
    morkList* list = &ioTable->mTable_ChangeList;
    morkNext* next = list->GetListHead();
    while ( next && ev->Good() )
    {
      r = ((morkTableChange*) next)->mTableChange_Row;
      if  ( r && r->IsRow() )
        this->PutRowDict(ev, r);
      next = next->GetNextLink();
    }
  }
  if ( ev->Good() )
    this->EndDict(ev);
  
  return ev->Good();
}
  
void
morkWriter::WriteTokenToTokenMetaCell(morkEnv* ev,
  mork_token inCol, mork_token inValue)
{
  morkStream* stream = mWriter_Stream;
  mork_bool isKindCol = ( morkStore_kKindColumn == inCol );
  mork_u1 valSep = (mork_u1) (( isKindCol )? '^' : '=');
  
  char buf[ 128 ]; 
  char* p = buf;

  mork_size bytesWritten;
  if ( inCol < 0x80 )
  {
    stream->Putc(ev, '(');
    stream->Putc(ev, (char) inCol);
    stream->Putc(ev, valSep);
  }
  else
  {
    *p++ = '('; 
    
    *p++ = '^'; 
    mork_size colSize = ev->TokenAsHex(p, inCol);
    p += colSize;
    *p++ = (char) valSep;
    stream->Write(ev->AsMdbEnv(), buf, colSize + 3, &bytesWritten);
    
    mWriter_LineSize += bytesWritten;
  }

  if ( isKindCol )
  {
    p = buf;
    mork_size valSize = ev->TokenAsHex(p, inValue);
    p += valSize;
    *p++ = ':';
    *p++ = 'c';
    *p++ = ')';
    stream->Write(ev->AsMdbEnv(), buf, valSize + 3, &bytesWritten);
    mWriter_LineSize += bytesWritten;
  }
  else
  {
    this->IndentAsNeeded(ev, morkWriter_kTableMetaCellValueDepth);
    mdbYarn* yarn = &mWriter_ColYarn;
    
    mWriter_Store->TokenToString(ev, inValue, yarn);
    this->WriteYarn(ev, yarn);
    stream->Putc(ev, ')');
    ++mWriter_LineSize;
  }
  
  
  
  
}
  
void
morkWriter::WriteStringToTokenDictCell(morkEnv* ev,
  const char* inCol, mork_token inValue)
  
{
  morkStream* stream = mWriter_Stream;
  mWriter_LineSize += stream->PutString(ev, inCol);

  this->IndentAsNeeded(ev, morkWriter_kDictMetaCellValueDepth);
  mdbYarn* yarn = &mWriter_ColYarn;
  
  mWriter_Store->TokenToString(ev, inValue, yarn);
  this->WriteYarn(ev, yarn);
  stream->Putc(ev, ')');
  ++mWriter_LineSize;
  
  
  
  
}

void
morkWriter::ChangeDictAtomScope(morkEnv* ev, mork_scope inScope)
{
  if ( inScope != mWriter_DictAtomScope )
  {
    ev->NewWarning("unexpected atom scope change");
    
    morkStream* stream = mWriter_Stream;
    if ( mWriter_LineSize )
      stream->PutLineBreak(ev);
    mWriter_LineSize = 0;

    char buf[ 128 ]; 
    char* p = buf;
    *p++ = '<'; 
    *p++ = '('; 
    *p++ = (char) morkStore_kAtomScopeColumn;

    mork_size scopeSize = 1; 
    if ( inScope >= 0x80 )
    {
      *p++ = '^'; 
      scopeSize = ev->TokenAsHex(p, inScope);
      p += scopeSize;
    }
    else
    {
      *p++ = '='; 
      *p++ = (char) (mork_u1) inScope;
    }

    *p++ = ')';
    *p++ = '>';
    *p = 0;

    mork_size pending = scopeSize + 6;
    this->IndentOverMaxLine(ev, pending, morkWriter_kDictAliasDepth);
    mork_size bytesWritten;

    stream->Write(ev->AsMdbEnv(), buf, pending, &bytesWritten);
    mWriter_LineSize += bytesWritten;
      
    mWriter_DictAtomScope = inScope;
  }
}

void
morkWriter::ChangeRowForm(morkEnv* ev, mork_cscode inNewForm)
{
  if ( inNewForm != mWriter_RowForm )
  {
    morkStream* stream = mWriter_Stream;
    if ( mWriter_LineSize )
      stream->PutLineBreak(ev);
    mWriter_LineSize = 0;

    char buf[ 128 ]; 
    char* p = buf;
    *p++ = '['; 
    *p++ = '('; 
    *p++ = (char) morkStore_kFormColumn;

    mork_size formSize = 1; 
    if (! morkCh_IsValue(inNewForm))
    {
      *p++ = '^'; 
      formSize = ev->TokenAsHex(p, inNewForm);
      p += formSize;
    }
    else
    {
      *p++ = '='; 
      *p++ = (char) (mork_u1) inNewForm;
    }
    
    *p++ = ')';
    *p++ = ']';
    *p = 0;

    mork_size pending = formSize + 6;
    this->IndentOverMaxLine(ev, pending, morkWriter_kRowCellDepth);
    mork_size bytesWritten;
    stream->Write(ev->AsMdbEnv(), buf, pending, &bytesWritten);
    mWriter_LineSize += bytesWritten;
      
    mWriter_RowForm = inNewForm;
  }
}

void
morkWriter::ChangeDictForm(morkEnv* ev, mork_cscode inNewForm)
{
  if ( inNewForm != mWriter_DictForm )
  {
    morkStream* stream = mWriter_Stream;
    if ( mWriter_LineSize )
      stream->PutLineBreak(ev);
    mWriter_LineSize = 0;

    char buf[ 128 ]; 
    char* p = buf;
    *p++ = '<'; 
    *p++ = '('; 
    *p++ = (char) morkStore_kFormColumn;

    mork_size formSize = 1; 
    if (! morkCh_IsValue(inNewForm))
    {
      *p++ = '^'; 
      formSize = ev->TokenAsHex(p, inNewForm);
      p += formSize;
    }
    else
    {
      *p++ = '='; 
      *p++ = (char) (mork_u1) inNewForm;
    }
    
    *p++ = ')';
    *p++ = '>';
    *p = 0;

    mork_size pending = formSize + 6;
    this->IndentOverMaxLine(ev, pending, morkWriter_kDictAliasDepth);
    
    mork_size bytesWritten;
    stream->Write(ev->AsMdbEnv(), buf, pending, &bytesWritten);
    mWriter_LineSize += bytesWritten;
      
    mWriter_DictForm = inNewForm;
  }
}

void
morkWriter::StartDict(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_DidStartDict )
  {
    stream->Putc(ev, '>'); 
    ++mWriter_LineSize;
  }
  mWriter_DidStartDict = morkBool_kTrue;
  mWriter_DidEndDict = morkBool_kFalse;
  
  if ( mWriter_LineSize )
    stream->PutLineBreak(ev);
  mWriter_LineSize = 0;
  
  if ( mWriter_TableRowScope ) 
    stream->PutLineBreak(ev);
    
  if ( mWriter_DictForm || mWriter_DictAtomScope != 'v' )
  {
    stream->Putc(ev, '<');
    stream->Putc(ev, ' ');
    stream->Putc(ev, '<');
    mWriter_LineSize = 3;
    if ( mWriter_DictForm )
      this->WriteStringToTokenDictCell(ev, "(f=", mWriter_DictForm);
    if ( mWriter_DictAtomScope != 'v' )
      this->WriteStringToTokenDictCell(ev, "(a=", mWriter_DictAtomScope);
  
    stream->Putc(ev, '>');
    ++mWriter_LineSize;

    mWriter_LineSize = stream->PutIndent(ev, morkWriter_kDictAliasDepth);
  }
  else
  {
    stream->Putc(ev, '<');
    
    ++mWriter_LineSize;
  }
}

void
morkWriter::EndDict(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  if ( mWriter_DidStartDict )
  {
    stream->Putc(ev, '>'); 
    ++mWriter_LineSize;
  }
  mWriter_DidStartDict = morkBool_kFalse;
  mWriter_DidEndDict = morkBool_kTrue;
}

void
morkWriter::StartTable(morkEnv* ev, morkTable* ioTable)
{
  mdbOid toid; 
  ioTable->GetTableOid(ev, &toid);
  
  if ( ev->Good() )
  {
    morkStream* stream = mWriter_Stream;
    if ( mWriter_LineSize )
      stream->PutLineBreak(ev);
    mWriter_LineSize = 0;
    

    char buf[ 64 + 16 ]; 
    char* p = buf;
    *p++ = '{'; 
    mork_size punctSize = (mWriter_BeVerbose) ? 10 : 3; 

    if ( ioTable->IsTableRewrite() && mWriter_Incremental )
    {
      *p++ = '-';
      ++punctSize; 
      ++mWriter_LineSize;
    }
    mork_size oidSize = ev->OidAsHex(p, toid);
    p += oidSize;
    *p++ = ' '; 
    *p++ = '{'; 
    if (mWriter_BeVerbose)
    {
    
      *p++ = '/'; 
      *p++ = '*'; 
      *p++ = 'r'; 
      *p++ = '='; 

      mork_token tableUses = (mork_token) ioTable->mTable_GcUses;
      mork_size usesSize = ev->TokenAsHex(p, tableUses);
      punctSize += usesSize;
      p += usesSize;
    
      *p++ = '*'; 
      *p++ = '/'; 
      *p++ = ' '; 
    }
    mork_size bytesWritten;

    stream->Write(ev->AsMdbEnv(), buf, oidSize + punctSize, &bytesWritten);
    mWriter_LineSize += bytesWritten;

    mork_kind tk = mWriter_TableKind;
    if ( tk )
    {
      this->IndentAsNeeded(ev, morkWriter_kTableMetaCellDepth);
      this->WriteTokenToTokenMetaCell(ev, morkStore_kKindColumn, tk);
    }
      
    stream->Putc(ev, '('); 
    stream->Putc(ev, 's'); 
    stream->Putc(ev, '='); 
    mWriter_LineSize += 3;

    int prio = (int) ioTable->mTable_Priority;
    if ( prio > 9 ) 
      prio = 9;
    prio += '0'; 
    stream->Putc(ev, prio); 
    ++mWriter_LineSize;
    
    if ( ioTable->IsTableUnique() )
    {
      stream->Putc(ev, 'u'); 
      ++mWriter_LineSize;
    }
    if ( ioTable->IsTableVerbose() )
    {
      stream->Putc(ev, 'v'); 
      ++mWriter_LineSize;
    }
    
    
    
    stream->Putc(ev, ')'); 
    mWriter_LineSize += 1; 

    morkRow* r = ioTable->mTable_MetaRow;
    if ( r )
    {
      if ( r->IsRow() )
      {
        mWriter_SuppressDirtyRowNewline = morkBool_kTrue;
        this->PutRow(ev, r);
      }
      else
        r->NonRowTypeError(ev);
    }
    
    stream->Putc(ev, '}'); 
    ++mWriter_LineSize;
    
    if ( mWriter_LineSize < mWriter_MaxIndent )
    {
      stream->Putc(ev, ' '); 
      ++mWriter_LineSize;
    }
  }
}

void
morkWriter::EndTable(morkEnv* ev)
{
  morkStream* stream = mWriter_Stream;
  stream->Putc(ev, '}'); 
  ++mWriter_LineSize;

  mWriter_TableAtomScope = 'v'; 
}

mork_bool
morkWriter::PutRowDict(morkEnv* ev, morkRow* ioRow)
{
  mWriter_RowForm = mWriter_TableForm;

  morkCell* cells = ioRow->mRow_Cells;
  if ( cells )
  {
    morkStream* stream = mWriter_Stream;
    mdbYarn yarn; 
    char buf[ 64 ]; 
    char* idBuf = buf + 1; 
    buf[ 0 ] = '('; 

    morkCell* end = cells + ioRow->mRow_Length;
    --cells; 
    while ( ++cells < end && ev->Good() )
    {
      morkAtom* atom = cells->GetAtom();
      if ( atom && atom->IsAtomDirty() )
      {
        if ( atom->IsBook() ) 
        {
          if ( !this->DidStartDict() )
          {
            this->StartDict(ev);
            if ( ev->Bad() )
              break;
          }
          atom->SetAtomClean(); 
          
          this->IndentAsNeeded(ev, morkWriter_kDictAliasDepth);
          morkBookAtom* ba = (morkBookAtom*) atom;
          mork_size size = ev->TokenAsHex(idBuf, ba->mBookAtom_Id);
          mork_size bytesWritten;
          stream->Write(ev->AsMdbEnv(), buf, size+1, &bytesWritten); 
          mWriter_LineSize += bytesWritten;

          if ( atom->AliasYarn(&yarn) )
          {
            mork_scope atomScope = atom->GetBookAtomSpaceScope(ev);
            if ( atomScope && atomScope != mWriter_DictAtomScope )
              this->ChangeDictAtomScope(ev, atomScope);
            
            if ( mWriter_DidStartDict && yarn.mYarn_Form != mWriter_DictForm )
              this->ChangeDictForm(ev, yarn.mYarn_Form);  
      
            mork_size pending = yarn.mYarn_Fill + morkWriter_kYarnEscapeSlop + 1;
            this->IndentOverMaxLine(ev, pending, morkWriter_kDictAliasValueDepth);
              
            stream->Putc(ev, '='); 
            ++mWriter_LineSize;
      
            this->WriteYarn(ev, &yarn);

            stream->Putc(ev, ')'); 
            ++mWriter_LineSize;
          }
          else
            atom->BadAtomKindError(ev);
                      
          ++mWriter_DoneCount;
        }
      }
    }
  }
  return ev->Good();
}

mork_bool
morkWriter::IsYarnAllValue(const mdbYarn* inYarn)
{
  mork_fill fill = inYarn->mYarn_Fill;
  const mork_u1* buf = (const mork_u1*) inYarn->mYarn_Buf;
  const mork_u1* end = buf + fill;
  --buf; 
  while ( ++buf < end )
  {
    mork_ch c = *buf;
    if ( !morkCh_IsValue(c) )
      return morkBool_kFalse;
  }
  return morkBool_kTrue;
}

mork_bool
morkWriter::PutVerboseCell(morkEnv* ev, morkCell* ioCell, mork_bool inWithVal)
{
  morkStream* stream = mWriter_Stream;
  morkStore* store = mWriter_Store;

  mdbYarn* colYarn = &mWriter_ColYarn;
  
  morkAtom* atom = (inWithVal)? ioCell->GetAtom() : (morkAtom*) 0;
  
  mork_column col = ioCell->GetColumn();
  store->TokenToString(ev, col, colYarn);
  
  mdbYarn yarn; 
  atom->AliasYarn(&yarn); 
  
  if ( yarn.mYarn_Form != mWriter_RowForm )
    this->ChangeRowForm(ev, yarn.mYarn_Form);

  mork_size pending = yarn.mYarn_Fill + colYarn->mYarn_Fill +
     morkWriter_kYarnEscapeSlop + 3;
  this->IndentOverMaxLine(ev, pending, morkWriter_kRowCellDepth);

  stream->Putc(ev, '('); 
  ++mWriter_LineSize;

  this->WriteYarn(ev, colYarn); 
  
  pending = yarn.mYarn_Fill + morkWriter_kYarnEscapeSlop;
  this->IndentOverMaxLine(ev, pending, morkWriter_kRowCellValueDepth);
  stream->Putc(ev, '=');
  ++mWriter_LineSize;
  
  this->WriteYarn(ev, &yarn); 
  
  stream->Putc(ev, ')'); 
  ++mWriter_LineSize;

  return ev->Good();
}

mork_bool
morkWriter::PutVerboseRowCells(morkEnv* ev, morkRow* ioRow)
{
  morkCell* cells = ioRow->mRow_Cells;
  if ( cells )
  {

    morkCell* end = cells + ioRow->mRow_Length;
    --cells; 
    while ( ++cells < end && ev->Good() )
    {
      
      if ( cells->GetAtom() ) 
        this->PutVerboseCell(ev, cells,  morkBool_kTrue);
    }
  }
  return ev->Good();
}


mork_bool
morkWriter::PutCell(morkEnv* ev, morkCell* ioCell, mork_bool inWithVal)
{
  morkStream* stream = mWriter_Stream;
  char buf[ 128 ]; 
  char* idBuf = buf + 2; 
  buf[ 0 ] = '('; 
  buf[ 1 ] = '^'; 
  
  mork_size colSize = 0; 
  mork_size bytesWritten;
  
  morkAtom* atom = (inWithVal)? ioCell->GetAtom() : (morkAtom*) 0;
  
  mork_column col = ioCell->GetColumn();
  char* p = idBuf;
  colSize = ev->TokenAsHex(p, col);
  p += colSize;

  mdbYarn yarn; 
  atom->AliasYarn(&yarn); 
  
  if ( yarn.mYarn_Form != mWriter_RowForm )
    this->ChangeRowForm(ev, yarn.mYarn_Form);
  
  if ( atom && atom->IsBook() ) 
  {
    this->IndentAsNeeded(ev, morkWriter_kRowCellDepth);
    *p++ = '^';
    morkBookAtom* ba = (morkBookAtom*) atom;

    mork_size valSize = ev->TokenAsHex(p, ba->mBookAtom_Id);
    mork_fill yarnFill = yarn.mYarn_Fill;
    mork_bool putImmYarn = ( yarnFill <= valSize );
    if ( putImmYarn )
      putImmYarn = this->IsYarnAllValue(&yarn);
    
    if ( putImmYarn ) 
    {
      p[ -1 ] = '='; 
      if ( yarnFill )
      {
        MORK_MEMCPY(p, yarn.mYarn_Buf, yarnFill);
        p += yarnFill;
      }
      *p++ = ')';
      mork_size distance = (mork_size) (p - buf);
      stream->Write(ev->AsMdbEnv(), buf, distance, &bytesWritten);
      mWriter_LineSize += bytesWritten;
    }
    else
    {
      p += valSize;
      *p = ')';
      stream->Write(ev->AsMdbEnv(), buf, colSize + valSize + 4, &bytesWritten);
      mWriter_LineSize += bytesWritten;
    }

    if ( atom->IsAtomDirty() )
    {
      atom->SetAtomClean();
      ++mWriter_DoneCount;
    }
  }
  else 
  {
    mork_size pending = yarn.mYarn_Fill + colSize +
      morkWriter_kYarnEscapeSlop + 2;
    this->IndentOverMaxLine(ev, pending, morkWriter_kRowCellDepth);

    mork_size bytesWritten;
    stream->Write(ev->AsMdbEnv(), buf, colSize + 2, &bytesWritten);
    mWriter_LineSize += bytesWritten;

    pending -= ( colSize + 2 );
    this->IndentOverMaxLine(ev, pending, morkWriter_kRowCellDepth);
    stream->Putc(ev, '=');
    ++mWriter_LineSize;
    
    this->WriteYarn(ev, &yarn);
    stream->Putc(ev, ')'); 
    ++mWriter_LineSize;
  }
  return ev->Good();
}

mork_bool
morkWriter::PutRowCells(morkEnv* ev, morkRow* ioRow)
{
  morkCell* cells = ioRow->mRow_Cells;
  if ( cells )
  {
    morkCell* end = cells + ioRow->mRow_Length;
    --cells; 
    while ( ++cells < end && ev->Good() )
    {
      
      if ( cells->GetAtom() ) 
        this->PutCell(ev, cells,  morkBool_kTrue);
    }
  }
  return ev->Good();
}

mork_bool
morkWriter::PutRow(morkEnv* ev, morkRow* ioRow)
{
  if ( ioRow && ioRow->IsRow() )
  {
    mWriter_RowForm = mWriter_TableForm;

    mork_size bytesWritten;
    morkStream* stream = mWriter_Stream;
    char buf[ 128 + 16 ]; 
    char* p = buf;
    mdbOid* roid = &ioRow->mRow_Oid;
    mork_size ridSize = 0;
    
    mork_scope tableScope = mWriter_TableRowScope;

    if ( ioRow->IsRowDirty() )
    {
      if ( mWriter_SuppressDirtyRowNewline || !mWriter_LineSize )
        mWriter_SuppressDirtyRowNewline = morkBool_kFalse;
      else
      {
        if ( tableScope ) 
          mWriter_LineSize = stream->PutIndent(ev, morkWriter_kRowDepth);
        else
          mWriter_LineSize = stream->PutIndent(ev, 0); 
      }
      

      *p++ = '['; 
      mork_size punctSize = (mWriter_BeVerbose) ? 9 : 1; 
      
      mork_bool rowRewrite = ioRow->IsRowRewrite();
            
      if ( rowRewrite && mWriter_Incremental )
      {
        *p++ = '-';
        ++punctSize; 
        ++mWriter_LineSize;
      }

      if ( tableScope && roid->mOid_Scope == tableScope )
        ridSize = ev->TokenAsHex(p, roid->mOid_Id);
      else
        ridSize = ev->OidAsHex(p, *roid);
      
      p += ridSize;
      
      if (mWriter_BeVerbose)
      {
        *p++ = ' '; 
        *p++ = '/'; 
        *p++ = '*'; 
        *p++ = 'r'; 
        *p++ = '='; 

        mork_size usesSize = ev->TokenAsHex(p, (mork_token) ioRow->mRow_GcUses);
        punctSize += usesSize;
        p += usesSize;
      
        *p++ = '*'; 
        *p++ = '/'; 
        *p++ = ' '; 
      }
      stream->Write(ev->AsMdbEnv(), buf, ridSize + punctSize, &bytesWritten);
      mWriter_LineSize += bytesWritten;
      
      
      if ( !rowRewrite && mWriter_Incremental && ioRow->HasRowDelta() )
      {
        mork_column col = ioRow->GetDeltaColumn();
        morkCell dummy(col, morkChange_kNil, (morkAtom*) 0);
        morkCell* cell = 0;
        
        mork_bool withVal = ( ioRow->GetDeltaChange() != morkChange_kCut );
        
        if ( withVal )
        {
          mork_pos cellPos = 0; 
          cell = ioRow->GetCell(ev, col, &cellPos);
        }
        if ( !cell )
          cell = &dummy;
          
        if ( mWriter_BeVerbose )
          this->PutVerboseCell(ev, cell, withVal);
        else
          this->PutCell(ev, cell, withVal);
      }
      else 
      {
        if ( mWriter_BeVerbose )
          this->PutVerboseRowCells(ev, ioRow); 
        else
          this->PutRowCells(ev, ioRow); 
      }
        
      stream->Putc(ev, ']'); 
      ++mWriter_LineSize;
    }
    else
    {
      this->IndentAsNeeded(ev, morkWriter_kRowDepth);

      if ( tableScope && roid->mOid_Scope == tableScope )
        ridSize = ev->TokenAsHex(p, roid->mOid_Id);
      else
        ridSize = ev->OidAsHex(p, *roid);

      stream->Write(ev->AsMdbEnv(), buf, ridSize, &bytesWritten);
      mWriter_LineSize += bytesWritten;
      stream->Putc(ev, ' ');
      ++mWriter_LineSize;
    }

    ++mWriter_DoneCount;

    ioRow->SetRowClean(); 
  }
  else
    ioRow->NonRowTypeWarning(ev);
  
  return ev->Good();
}



