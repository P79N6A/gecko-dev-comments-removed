




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKPARSER_
#include "morkParser.h"
#endif

#ifndef _MORKBUILDER_
#include "morkBuilder.h"
#endif

#ifndef _MORKCELL_
#include "morkCell.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _MORKCELL_
#include "morkCell.h"
#endif

#ifndef _MORKATOM_
#include "morkAtom.h"
#endif

#ifndef _MORKATOMSPACE_
#include "morkAtomSpace.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif






 void
morkBuilder::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseBuilder(ev);
    this->MarkShut();
  }
}


morkBuilder::~morkBuilder() 
{
  MORK_ASSERT(mBuilder_Store==0);
  MORK_ASSERT(mBuilder_Row==0);
  MORK_ASSERT(mBuilder_Table==0);
  MORK_ASSERT(mBuilder_Cell==0);
  MORK_ASSERT(mBuilder_RowSpace==0);
  MORK_ASSERT(mBuilder_AtomSpace==0);
}


morkBuilder::morkBuilder(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap, 
  morkStream* ioStream, mdb_count inBytesPerParseSegment,
  nsIMdbHeap* ioSlotHeap, morkStore* ioStore)

: morkParser(ev, inUsage, ioHeap, ioStream,
  inBytesPerParseSegment, ioSlotHeap)
  
, mBuilder_Store( 0 )
  
, mBuilder_Table( 0 )
, mBuilder_Row( 0 )
, mBuilder_Cell( 0 )
  
, mBuilder_RowSpace( 0 )
, mBuilder_AtomSpace( 0 )
  
, mBuilder_OidAtomSpace( 0 )
, mBuilder_ScopeAtomSpace( 0 )
  
, mBuilder_PortForm( 0 )
, mBuilder_PortRowScope( (mork_scope) 'r' )
, mBuilder_PortAtomScope( (mork_scope) 'v' )

, mBuilder_TableForm( 0 )
, mBuilder_TableRowScope( (mork_scope) 'r' )
, mBuilder_TableAtomScope( (mork_scope) 'v' )
, mBuilder_TableKind( 0 )

, mBuilder_TablePriority( morkPriority_kLo )
, mBuilder_TableIsUnique( morkBool_kFalse )
, mBuilder_TableIsVerbose( morkBool_kFalse )
, mBuilder_TablePadByte( 0 )
  
, mBuilder_RowForm( 0 )
, mBuilder_RowRowScope( (mork_scope) 'r' )
, mBuilder_RowAtomScope( (mork_scope) 'v' )

, mBuilder_CellForm( 0 )
, mBuilder_CellAtomScope( (mork_scope) 'v' )

, mBuilder_DictForm( 0 )
, mBuilder_DictAtomScope( (mork_scope) 'v' )

, mBuilder_MetaTokenSlot( 0 )
  
, mBuilder_DoCutRow( morkBool_kFalse )
, mBuilder_DoCutCell( morkBool_kFalse )
, mBuilder_CellsVecFill( 0 )
{
  if ( ev->Good() )
  {
    if ( ioStore )
    {
      morkStore::SlotWeakStore(ioStore, ev, &mBuilder_Store);
      if ( ev->Good() )
        mNode_Derived = morkDerived_kBuilder;
    }
    else
      ev->NilPointerError();
  }
   
}

 void
morkBuilder::CloseBuilder(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      mBuilder_Row = 0;
      mBuilder_Cell = 0;
      mBuilder_MetaTokenSlot = 0;
      
      morkTable::SlotStrongTable((morkTable*) 0, ev, &mBuilder_Table);
      morkStore::SlotWeakStore((morkStore*) 0, ev, &mBuilder_Store);

      morkRowSpace::SlotStrongRowSpace((morkRowSpace*) 0, ev,
        &mBuilder_RowSpace);

      morkAtomSpace::SlotStrongAtomSpace((morkAtomSpace*) 0, ev,
        &mBuilder_AtomSpace);

      morkAtomSpace::SlotStrongAtomSpace((morkAtomSpace*) 0, ev,
        &mBuilder_OidAtomSpace);

      morkAtomSpace::SlotStrongAtomSpace((morkAtomSpace*) 0, ev,
        &mBuilder_ScopeAtomSpace);
      this->CloseParser(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkBuilder::NonBuilderTypeError(morkEnv* ev)
{
  ev->NewError("non morkBuilder");
}

 void
morkBuilder::NilBuilderCellError(morkEnv* ev)
{
  ev->NewError("nil mBuilder_Cell");
}

 void
morkBuilder::NilBuilderRowError(morkEnv* ev)
{
  ev->NewError("nil mBuilder_Row");
}

 void
morkBuilder::NilBuilderTableError(morkEnv* ev)
{
  ev->NewError("nil mBuilder_Table");
}

 void
morkBuilder::NonColumnSpaceScopeError(morkEnv* ev)
{
  ev->NewError("column space != 'c'");
}

void
morkBuilder::LogGlitch(morkEnv* ev, const morkGlitch& inGlitch, 
  const char* inKind)
{
  MORK_USED_2(inGlitch,inKind);
  ev->NewWarning("parsing glitch");
}

 void
morkBuilder::MidToYarn(morkEnv* ev,
  const morkMid& inMid,  
  mdbYarn* outYarn)




{
  mBuilder_Store->MidToYarn(ev, inMid, outYarn);
}

 void
morkBuilder::OnNewPort(morkEnv* ev, const morkPlace& inPlace)



{
  MORK_USED_2(ev,inPlace);
  
  mBuilder_PortForm = 0;
  mBuilder_PortRowScope = (mork_scope) 'r';
  mBuilder_PortAtomScope = (mork_scope) 'v';
}

 void
morkBuilder::OnPortGlitch(morkEnv* ev, const morkGlitch& inGlitch)  
{
  this->LogGlitch(ev, inGlitch, "port");
}

 void
morkBuilder::OnPortEnd(morkEnv* ev, const morkSpan& inSpan)

{
  MORK_USED_2(ev,inSpan);
  
  
  
}

 void
morkBuilder::OnNewGroup(morkEnv* ev, const morkPlace& inPlace, mork_gid inGid)
{
  MORK_USED_1(inPlace);
  
  mork_pos startPos = inPlace.mPlace_Pos;

  morkStore* store = mBuilder_Store;
  if ( store )
  {
    if ( inGid >= store->mStore_CommitGroupIdentity )
      store->mStore_CommitGroupIdentity = inGid + 1;
  
    if ( !store->mStore_FirstCommitGroupPos )
      store->mStore_FirstCommitGroupPos = startPos;
    else if ( !store->mStore_SecondCommitGroupPos )
      store->mStore_SecondCommitGroupPos = startPos;
  }
}

 void
morkBuilder::OnGroupGlitch(morkEnv* ev, const morkGlitch& inGlitch) 
{
  this->LogGlitch(ev, inGlitch, "group");
}

 void
morkBuilder::OnGroupCommitEnd(morkEnv* ev, const morkSpan& inSpan)  
{
  MORK_USED_2(ev,inSpan);
  
  
}

 void
morkBuilder::OnGroupAbortEnd(morkEnv* ev, const morkSpan& inSpan) 
{
  MORK_USED_1(inSpan);
  
  ev->StubMethodOnlyError();
}

 void
morkBuilder::OnNewPortRow(morkEnv* ev, const morkPlace& inPlace, 
  const morkMid& inMid, mork_change inChange)
{
  MORK_USED_3(inMid,inPlace,inChange);
  
  ev->StubMethodOnlyError();
}

 void
morkBuilder::OnPortRowGlitch(morkEnv* ev, const morkGlitch& inGlitch)
{
  this->LogGlitch(ev, inGlitch, "port row");
}

 void
morkBuilder::OnPortRowEnd(morkEnv* ev, const morkSpan& inSpan)
{
  MORK_USED_1(inSpan);
  
  ev->StubMethodOnlyError();
}

 void
morkBuilder::OnNewTable(morkEnv* ev, const morkPlace& inPlace,
  const morkMid& inMid, mork_bool inCutAllRows)





{
  MORK_USED_1(inPlace);
  
  mBuilder_TableForm = mBuilder_PortForm;
  mBuilder_TableRowScope = mBuilder_PortRowScope;
  mBuilder_TableAtomScope = mBuilder_PortAtomScope;
  mBuilder_TableKind = morkStore_kNoneToken;
  
  mBuilder_TablePriority = morkPriority_kLo;
  mBuilder_TableIsUnique = morkBool_kFalse;
  mBuilder_TableIsVerbose = morkBool_kFalse;

  morkTable* table = mBuilder_Store->MidToTable(ev, inMid);
  morkTable::SlotStrongTable(table, ev, &mBuilder_Table);
  if ( table )
  {
    if ( table->mTable_RowSpace )
      mBuilder_TableRowScope = table->mTable_RowSpace->SpaceScope();
      
    if ( inCutAllRows )
      table->CutAllRows(ev);
  }
}

 void
morkBuilder::OnTableGlitch(morkEnv* ev, const morkGlitch& inGlitch)
{
  this->LogGlitch(ev, inGlitch, "table");
}

 void
morkBuilder::OnTableEnd(morkEnv* ev, const morkSpan& inSpan)

{
  MORK_USED_1(inSpan);
  
  if ( mBuilder_Table )
  {
    mBuilder_Table->mTable_Priority = mBuilder_TablePriority;
    
    if ( mBuilder_TableIsUnique )
      mBuilder_Table->SetTableUnique();

    if ( mBuilder_TableIsVerbose )
      mBuilder_Table->SetTableVerbose();
  
    morkTable::SlotStrongTable((morkTable*) 0, ev, &mBuilder_Table);
  }
  else
    this->NilBuilderTableError(ev);
    
  mBuilder_Row = 0;
  mBuilder_Cell = 0;
  
  
  mBuilder_TablePriority = morkPriority_kLo;
  mBuilder_TableIsUnique = morkBool_kFalse;
  mBuilder_TableIsVerbose = morkBool_kFalse;

  if ( mBuilder_TableKind == morkStore_kNoneToken )
    ev->NewError("missing table kind");

  mBuilder_CellAtomScope = mBuilder_RowAtomScope =
    mBuilder_TableAtomScope = mBuilder_PortAtomScope;

  mBuilder_DoCutCell = morkBool_kFalse;
  mBuilder_DoCutRow = morkBool_kFalse;
}

 void
morkBuilder::OnNewMeta(morkEnv* ev, const morkPlace& inPlace)





{
  MORK_USED_2(ev,inPlace);
  
  
}

 void
morkBuilder::OnMetaGlitch(morkEnv* ev, const morkGlitch& inGlitch)
{
  this->LogGlitch(ev, inGlitch, "meta");
}

 void
morkBuilder::OnMetaEnd(morkEnv* ev, const morkSpan& inSpan)

{
  MORK_USED_2(ev,inSpan);
  
}

 void
morkBuilder::OnMinusRow(morkEnv* ev)
{
  MORK_USED_1(ev);
  mBuilder_DoCutRow = morkBool_kTrue;
}

 void
morkBuilder::OnNewRow(morkEnv* ev, const morkPlace& inPlace, 
  const morkMid& inMid, mork_bool inCutAllCols)








{
  MORK_USED_1(inPlace);
  
  
  mBuilder_CellForm = mBuilder_RowForm = mBuilder_TableForm;
  mBuilder_CellAtomScope = mBuilder_RowAtomScope = mBuilder_TableAtomScope;
  mBuilder_RowRowScope = mBuilder_TableRowScope;
  morkStore* store = mBuilder_Store;
  
  if ( !inMid.mMid_Buf && !inMid.mMid_Oid.mOid_Scope )
  {
    morkMid mid(inMid);
    mid.mMid_Oid.mOid_Scope = mBuilder_RowRowScope;
    mBuilder_Row = store->MidToRow(ev, mid);
  }
  else
  {
    mBuilder_Row = store->MidToRow(ev, inMid);
  }
  morkRow* row = mBuilder_Row;
  if ( row && inCutAllCols )
  {
    row->CutAllColumns(ev);
  }

  morkTable* table = mBuilder_Table;
  if ( table )
  {
    if ( row )
    {
      if ( mParser_InMeta )
      {
        morkRow* metaRow = table->mTable_MetaRow;
        if ( !metaRow )
        {
          table->mTable_MetaRow = row;
          table->mTable_MetaRowOid = row->mRow_Oid;
          row->AddRowGcUse(ev);
        }
        else if ( metaRow != row ) 
          ev->NewError("duplicate table meta row");
      }
      else
      {
        if ( mBuilder_DoCutRow )
          table->CutRow(ev, row);
        else
          table->AddRow(ev, row);
      }
    }
  }
  
  
    
  mBuilder_DoCutRow = morkBool_kFalse;
}

 void
morkBuilder::OnRowPos(morkEnv* ev, mork_pos inRowPos) 
{
  if ( mBuilder_Row && mBuilder_Table && !mParser_InMeta )
  {
    mork_pos hintFromPos = 0; 
    mBuilder_Table->MoveRow(ev, mBuilder_Row, hintFromPos, inRowPos);
  }
}

 void
morkBuilder::OnRowGlitch(morkEnv* ev, const morkGlitch& inGlitch) 
{
  this->LogGlitch(ev, inGlitch, "row");
}

void
morkBuilder::FlushBuilderCells(morkEnv* ev)
{
  if ( mBuilder_Row )
  {
    morkPool* pool = mBuilder_Store->StorePool();
    morkCell* cells = mBuilder_CellsVec;
    mork_fill fill = mBuilder_CellsVecFill;
    mBuilder_Row->TakeCells(ev, cells, fill, mBuilder_Store);

    morkCell* end = cells + fill;
    --cells; 
    while ( ++cells < end )
    {
      if ( cells->mCell_Atom )
        cells->SetAtom(ev, (morkAtom*) 0, pool);
    }
    mBuilder_CellsVecFill = 0;
  }
  else
    this->NilBuilderRowError(ev);
}

 void
morkBuilder::OnRowEnd(morkEnv* ev, const morkSpan& inSpan) 

{
  MORK_USED_1(inSpan);
  
  if ( mBuilder_Row )
  {
    this->FlushBuilderCells(ev);
  }
  else
    this->NilBuilderRowError(ev);
    
  mBuilder_Row = 0;
  mBuilder_Cell = 0;

  mBuilder_DoCutCell = morkBool_kFalse;
  mBuilder_DoCutRow = morkBool_kFalse;
}

 void
morkBuilder::OnNewDict(morkEnv* ev, const morkPlace& inPlace)


{
  MORK_USED_2(ev,inPlace);
  
  
  mBuilder_CellForm = mBuilder_DictForm = mBuilder_PortForm;
  mBuilder_CellAtomScope = mBuilder_DictAtomScope = mBuilder_PortAtomScope;
}

 void
morkBuilder::OnDictGlitch(morkEnv* ev, const morkGlitch& inGlitch) 
{
  this->LogGlitch(ev, inGlitch, "dict");
}

 void
morkBuilder::OnDictEnd(morkEnv* ev, const morkSpan& inSpan)  

{
  MORK_USED_2(ev,inSpan);
  

  mBuilder_DictForm = 0;
  mBuilder_DictAtomScope = 0;
}

 void
morkBuilder::OnAlias(morkEnv* ev, const morkSpan& inSpan,
  const morkMid& inMid)
{
  MORK_USED_1(inSpan);
  if ( mParser_InDict )
  {
    morkMid mid = inMid; 
    mid.mMid_Oid.mOid_Scope = mBuilder_DictAtomScope;
    mBuilder_Store->AddAlias(ev, mid, mBuilder_DictForm);
  }
  else
    ev->NewError("alias not in dict");
}

 void
morkBuilder::OnAliasGlitch(morkEnv* ev, const morkGlitch& inGlitch)
{
  this->LogGlitch(ev, inGlitch, "alias");
}


morkCell* 
morkBuilder::AddBuilderCell(morkEnv* ev,
  const morkMid& inMid, mork_change inChange)
{
  morkCell* outCell = 0;
  mork_column column = inMid.mMid_Oid.mOid_Id;
  
  if ( ev->Good() )
  {
    if ( mBuilder_CellsVecFill >= morkBuilder_kCellsVecSize )
      this->FlushBuilderCells(ev);
    if ( ev->Good() )
    {
      if ( mBuilder_CellsVecFill < morkBuilder_kCellsVecSize )
      {
        mork_fill indx = mBuilder_CellsVecFill++;
        outCell = mBuilder_CellsVec + indx;
        outCell->SetColumnAndChange(column, inChange);
        outCell->mCell_Atom = 0;
      }
      else
        ev->NewError("out of builder cells");
    }
  }
  return outCell;
}

 void
morkBuilder::OnMinusCell(morkEnv* ev)
{
  MORK_USED_1(ev);
  mBuilder_DoCutCell = morkBool_kTrue;
}

 void
morkBuilder::OnNewCell(morkEnv* ev, const morkPlace& inPlace,
    const morkMid* inMid, const morkBuf* inBuf)



  
  
  
  
{
  MORK_USED_1(inPlace);
  
  
  mork_change cellChange = ( mBuilder_DoCutCell )?
    morkChange_kCut : morkChange_kAdd;
    
  mBuilder_DoCutCell = morkBool_kFalse;
  
  mBuilder_CellAtomScope = mBuilder_RowAtomScope;
  
  mBuilder_Cell = 0; 
  morkStore* store = mBuilder_Store;
  mork_scope scope = morkStore_kColumnSpaceScope;
  morkMid tempMid; 
  morkMid* cellMid = &tempMid; 
  
  if ( inMid ) 
  {
    *cellMid = *inMid; 

    if ( !cellMid->mMid_Oid.mOid_Scope ) 
    {
      if ( cellMid->mMid_Buf )
      {
        scope = store->BufToToken(ev, cellMid->mMid_Buf);
        cellMid->mMid_Buf = 0; 
        ev->NewWarning("column mids need column scope");
      }
      cellMid->mMid_Oid.mOid_Scope = scope;
    }
  }
  else if ( inBuf ) 
  {
    cellMid->ClearMid();
    cellMid->mMid_Oid.mOid_Id = store->BufToToken(ev, inBuf);
    cellMid->mMid_Oid.mOid_Scope = scope; 
  }
  else
    ev->NilPointerError(); 

  mork_column column = cellMid->mMid_Oid.mOid_Id;
  
  if ( mBuilder_Row && ev->Good() ) 
  {
      

      if ( mBuilder_CellsVecFill >= morkBuilder_kCellsVecSize )
        this->FlushBuilderCells(ev);
      if ( ev->Good() )
      {
        if ( mBuilder_CellsVecFill < morkBuilder_kCellsVecSize )
        {
          mork_fill ix = mBuilder_CellsVecFill++;
          morkCell* cell =  mBuilder_CellsVec + ix;
          cell->SetColumnAndChange(column, cellChange);
          
          cell->mCell_Atom = 0;
          mBuilder_Cell = cell;
        }
        else
          ev->NewError("out of builder cells");
      }
  }

  else if ( mParser_InMeta &&  ev->Good() ) 
  {
    if ( scope == morkStore_kColumnSpaceScope )
    {
      if ( mParser_InTable ) 
      {
        if ( column == morkStore_kKindColumn )
          mBuilder_MetaTokenSlot = &mBuilder_TableKind;
        else if ( column == morkStore_kStatusColumn )
          mBuilder_MetaTokenSlot = &mBuilder_TableStatus;
        else if ( column == morkStore_kRowScopeColumn )
          mBuilder_MetaTokenSlot = &mBuilder_TableRowScope;
        else if ( column == morkStore_kAtomScopeColumn )
          mBuilder_MetaTokenSlot = &mBuilder_TableAtomScope;
        else if ( column == morkStore_kFormColumn )
          mBuilder_MetaTokenSlot = &mBuilder_TableForm;
      }
      else if ( mParser_InDict ) 
      {
        if ( column == morkStore_kAtomScopeColumn )
          mBuilder_MetaTokenSlot = &mBuilder_DictAtomScope;
        else if ( column == morkStore_kFormColumn )
          mBuilder_MetaTokenSlot = &mBuilder_DictForm;
      }
      else if ( mParser_InRow ) 
      {
        if ( column == morkStore_kAtomScopeColumn )
          mBuilder_MetaTokenSlot = &mBuilder_RowAtomScope;
        else if ( column == morkStore_kRowScopeColumn )
          mBuilder_MetaTokenSlot = &mBuilder_RowRowScope;
        else if ( column == morkStore_kFormColumn )
          mBuilder_MetaTokenSlot = &mBuilder_RowForm;
      }
    }
    else
      ev->NewWarning("expected column scope");
  }
}

 void
morkBuilder::OnCellGlitch(morkEnv* ev, const morkGlitch& inGlitch)
{
  this->LogGlitch(ev, inGlitch, "cell");
}

 void
morkBuilder::OnCellForm(morkEnv* ev, mork_cscode inCharsetFormat)
{
  morkCell* cell = mBuilder_Cell;
  if ( cell )
  {
    mBuilder_CellForm = inCharsetFormat;
  }
  else
    this->NilBuilderCellError(ev);
}

 void
morkBuilder::OnCellEnd(morkEnv* ev, const morkSpan& inSpan)

{
  MORK_USED_2(ev,inSpan);
  
  
  mBuilder_MetaTokenSlot = 0;
  mBuilder_CellAtomScope = mBuilder_RowAtomScope;
}

 void
morkBuilder::OnValue(morkEnv* ev, const morkSpan& inSpan,
  const morkBuf& inBuf)


{
  MORK_USED_1(inSpan);
  morkStore* store = mBuilder_Store;
  morkCell* cell = mBuilder_Cell;
  if ( cell )
  {
    mdbYarn yarn;
    yarn.mYarn_Buf = inBuf.mBuf_Body;
    yarn.mYarn_Fill = yarn.mYarn_Size = inBuf.mBuf_Fill;
    yarn.mYarn_More = 0;
    yarn.mYarn_Form = mBuilder_CellForm;
    yarn.mYarn_Grow = 0;
    morkAtom* atom = store->YarnToAtom(ev, &yarn, PR_TRUE );
    cell->SetAtom(ev, atom, store->StorePool());
  }
  else if ( mParser_InMeta )
  {
    mork_token* metaSlot = mBuilder_MetaTokenSlot;
    if ( metaSlot )
    {
      if ( metaSlot == &mBuilder_TableStatus ) 
      {
        if ( mParser_InTable && mBuilder_Table )
        {
          const char* body = (const char*) inBuf.mBuf_Body;
          mork_fill bufFill = inBuf.mBuf_Fill;
          if ( body && bufFill )
          {
            const char* bodyEnd = body + bufFill;
            while ( body < bodyEnd )
            {
              int c = *body++;
              switch ( c )
              {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                  mBuilder_TablePriority = (mork_priority) ( c - '0' );
                  break;
                
                case 'u':
                case 'U':
                  mBuilder_TableIsUnique = morkBool_kTrue;
                  break;
                  
                case 'v':
                case 'V':
                  mBuilder_TableIsVerbose = morkBool_kTrue;
                  break;
              }
            }
          }
        }
      }
      else
      {
        mork_token token = store->BufToToken(ev, &inBuf);
        if ( token )
        {
          *metaSlot = token;
          if ( metaSlot == &mBuilder_TableKind ) 
          {
            if ( mParser_InTable && mBuilder_Table )
              mBuilder_Table->mTable_Kind = token;
          }
        }
      }
    }
  }
  else
    this->NilBuilderCellError(ev);
}

 void
morkBuilder::OnValueMid(morkEnv* ev, const morkSpan& inSpan,
  const morkMid& inMid)


{
  MORK_USED_1(inSpan);
  morkStore* store = mBuilder_Store;
  morkCell* cell = mBuilder_Cell;

  morkMid valMid; 
  mdbOid* valOid = &valMid.mMid_Oid; 
  *valOid = inMid.mMid_Oid; 
  
  if ( inMid.mMid_Buf )
  {
    if ( !valOid->mOid_Scope )
      store->MidToOid(ev, inMid, valOid);
  }
  else if ( !valOid->mOid_Scope )
    valOid->mOid_Scope = mBuilder_CellAtomScope;
  
  if ( cell )
  {
    morkBookAtom* atom = store->MidToAtom(ev, valMid);
    if ( atom )
      cell->SetAtom(ev, atom, store->StorePool());
    else
      ev->NewError("undefined cell value alias");
  }
  else if ( mParser_InMeta )
  {
    mork_token* metaSlot = mBuilder_MetaTokenSlot;
    if ( metaSlot )
    {
      mork_scope valScope = valOid->mOid_Scope;
      if ( !valScope || valScope == morkStore_kColumnSpaceScope )
      {
        if ( ev->Good() && valMid.HasSomeId() )
        {
          *metaSlot = valOid->mOid_Id;
          if ( metaSlot == &mBuilder_TableKind ) 
          {
            if ( mParser_InTable && mBuilder_Table )
            {
              mBuilder_Table->mTable_Kind = valOid->mOid_Id;
            }
            else
              ev->NewWarning("mBuilder_TableKind not in table");
          }
          else if ( metaSlot == &mBuilder_TableStatus ) 
          {
            if ( mParser_InTable && mBuilder_Table )
            {
              
            }
            else
              ev->NewWarning("mBuilder_TableStatus not in table");
          }
        }
      }
      else
        this->NonColumnSpaceScopeError(ev);
    }
  }
  else
    this->NilBuilderCellError(ev);
}

 void
morkBuilder::OnRowMid(morkEnv* ev, const morkSpan& inSpan,
  const morkMid& inMid)


{
  MORK_USED_1(inSpan);
  morkStore* store = mBuilder_Store;
  morkCell* cell = mBuilder_Cell;
  if ( cell )
  {
    mdbOid rowOid = inMid.mMid_Oid;
    if ( inMid.mMid_Buf )
    {
      if ( !rowOid.mOid_Scope )
        store->MidToOid(ev, inMid, &rowOid);
    }
    else if ( !rowOid.mOid_Scope )
      rowOid.mOid_Scope = mBuilder_RowRowScope;
    
    if ( ev->Good() )
     {
       morkPool* pool = store->StorePool();
       morkAtom* atom = pool->NewRowOidAtom(ev, rowOid, &store->mStore_Zone);
       if ( atom )
       {
         cell->SetAtom(ev, atom, pool);
         morkRow* row = store->OidToRow(ev, &rowOid);
         if ( row ) 
           row->AddRowGcUse(ev);
       }
     }
  }
  else
    this->NilBuilderCellError(ev);
}

 void
morkBuilder::OnTableMid(morkEnv* ev, const morkSpan& inSpan,
  const morkMid& inMid)


{
  MORK_USED_1(inSpan);
  morkStore* store = mBuilder_Store;
  morkCell* cell = mBuilder_Cell;
  if ( cell )
  {
    mdbOid tableOid = inMid.mMid_Oid;
    if ( inMid.mMid_Buf )
    {
      if ( !tableOid.mOid_Scope )
        store->MidToOid(ev, inMid, &tableOid);
    }
    else if ( !tableOid.mOid_Scope )
      tableOid.mOid_Scope = mBuilder_RowRowScope;
    
    if ( ev->Good() )
     {
       morkPool* pool = store->StorePool();
       morkAtom* atom = pool->NewTableOidAtom(ev, tableOid, &store->mStore_Zone);
       if ( atom )
       {
         cell->SetAtom(ev, atom, pool);
         morkTable* table = store->OidToTable(ev, &tableOid,
            (mdbOid*) 0);
         if ( table ) 
           table->AddTableGcUse(ev);
       }
     }
  }
  else
    this->NilBuilderCellError(ev);
}



