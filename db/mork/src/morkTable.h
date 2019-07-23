




































#ifndef _MORKTABLE_
#define _MORKTABLE_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKDEQUE_
#include "morkDeque.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif

#ifndef _MORKARRAY_
#include "morkArray.h"
#endif

#ifndef _MORKROWMAP_
#include "morkRowMap.h"
#endif

#ifndef _MORKNODEMAP_
#include "morkNodeMap.h"
#endif

#ifndef _MORKPROBEMAP_
#include "morkProbeMap.h"
#endif

#ifndef _MORKBEAD_
#include "morkBead.h"
#endif



class nsIMdbTable;
#define morkDerived_kTable 0x5462 /* ascii 'Tb' */






#define morkTable_kStartRowArraySize 3 /* modest starting size for array */










#define morkTable_kMakeRowMapThreshold 17 /* when to build mTable_RowMap */

#define morkTable_kStartRowMapSlotCount 13
#define morkTable_kMaxTableGcUses 0x0FF /* max for 8-bit unsigned int */

#define morkTable_kUniqueBit   ((mork_u1) (1 << 0))
#define morkTable_kVerboseBit  ((mork_u1) (1 << 1))
#define morkTable_kNotedBit    ((mork_u1) (1 << 2)) /* space has change notes */
#define morkTable_kRewriteBit  ((mork_u1) (1 << 3)) /* must rewrite all rows */
#define morkTable_kNewMetaBit  ((mork_u1) (1 << 4)) /* new table meta row */

class morkTable : public morkObject, public morkLink, public nsIMdbTable { 

  


  

  
  
  
  
  
  
  
  
  
  

  
  

public: 

  NS_DECL_ISUPPORTS_INHERITED
  mork_tid     TableId() const { return mBead_Color; }
  void         SetTableId(mork_tid inTid) { mBead_Color = inTid; }

  
  virtual mork_refs    AddStrongRef(morkEnv* ev);
  virtual mork_refs    CutStrongRef(morkEnv* ev);
public: 



  
  NS_IMETHOD GetSeed(nsIMdbEnv* ev,
    mdb_seed* outSeed);    
  NS_IMETHOD GetCount(nsIMdbEnv* ev,
    mdb_count* outCount); 

  NS_IMETHOD GetPort(nsIMdbEnv* ev,
    nsIMdbPort** acqPort); 
  

  
  NS_IMETHOD GetCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inMemberPos, 
    nsIMdbCursor** acqCursor); 
  

  
  NS_IMETHOD GetOid(nsIMdbEnv* ev,
    mdbOid* outOid); 
  NS_IMETHOD BecomeContent(nsIMdbEnv* ev,
    const mdbOid* inOid); 
  

  
  NS_IMETHOD DropActivity( 
    nsIMdbEnv* ev);
  


  NS_IMETHOD SetTablePriority(nsIMdbEnv* ev, mdb_priority inPrio);
  NS_IMETHOD GetTablePriority(nsIMdbEnv* ev, mdb_priority* outPrio);
  
  NS_IMETHOD GetTableBeVerbose(nsIMdbEnv* ev, mdb_bool* outBeVerbose);
  NS_IMETHOD SetTableBeVerbose(nsIMdbEnv* ev, mdb_bool inBeVerbose);
  
  NS_IMETHOD GetTableIsUnique(nsIMdbEnv* ev, mdb_bool* outIsUnique);
  
  NS_IMETHOD GetTableKind(nsIMdbEnv* ev, mdb_kind* outTableKind);
  NS_IMETHOD GetRowScope(nsIMdbEnv* ev, mdb_scope* outRowScope);
  
  NS_IMETHOD GetMetaRow(
    nsIMdbEnv* ev, 
    const mdbOid* inOptionalMetaRowOid, 
    mdbOid* outOid, 
    nsIMdbRow** acqRow); 
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  


  
  NS_IMETHOD GetTableRowCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    nsIMdbTableRowCursor** acqCursor); 
  

  
  NS_IMETHOD PosToOid( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    mdbOid* outOid); 

  NS_IMETHOD OidToPos( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid, 
    mdb_pos* outPos); 
    
  NS_IMETHOD PosToRow( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    nsIMdbRow** acqRow); 
    
  NS_IMETHOD RowToPos( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow, 
    mdb_pos* outPos); 
  

  
  NS_IMETHOD AddOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid); 

  NS_IMETHOD HasOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid, 
    mdb_bool* outHasOid); 

  NS_IMETHOD CutOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid); 
  

  
  NS_IMETHOD NewRow( 
    nsIMdbEnv* ev, 
    mdbOid* ioOid, 
    nsIMdbRow** acqRow); 

  NS_IMETHOD AddRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow); 

  NS_IMETHOD HasRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow, 
    mdb_bool* outHasRow); 

  NS_IMETHOD CutRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow); 

  NS_IMETHOD CutAllRows( 
    nsIMdbEnv* ev); 
  

  
  NS_IMETHOD SearchColumnsHint( 
    nsIMdbEnv* ev, 
    const mdbColumnSet* inColumnSet); 
    
  NS_IMETHOD SortColumnsHint( 
    nsIMdbEnv* ev, 
    const mdbColumnSet* inColumnSet); 
    
  NS_IMETHOD StartBatchChangeHint( 
    nsIMdbEnv* ev, 
    const void* inLabel); 
    
    
    
    
  NS_IMETHOD EndBatchChangeHint( 
    nsIMdbEnv* ev, 
    const void* inLabel); 
    
    
    
    
    
    
    
    
    
  

  
  NS_IMETHOD FindRowMatches( 
    nsIMdbEnv* ev, 
    const mdbYarn* inPrefix, 
    nsIMdbTableRowCursor** acqCursor); 
    
  NS_IMETHOD GetSearchColumns( 
    nsIMdbEnv* ev, 
    mdb_count* outCount, 
    mdbColumnSet* outColSet); 
    
    
    
    
    
    
    
    
    
    
    
    
  

  
  
  

  NS_IMETHOD
  CanSortColumn( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdb_bool* outCanSort); 
    
  NS_IMETHOD GetSorting( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbSorting** acqSorting); 
    
  NS_IMETHOD SetSearchSorting( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbSorting* ioSorting); 
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  

  
  
  
  NS_IMETHOD MoveOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_pos inHintFromPos, 
    mdb_pos inToPos,       
    mdb_pos* outActualPos); 

  NS_IMETHOD MoveRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow,  
    mdb_pos inHintFromPos, 
    mdb_pos inToPos,       
    mdb_pos* outActualPos); 
  
  
  
  NS_IMETHOD AddIndex( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbThumb** acqThumb); 
  
  
  
  NS_IMETHOD CutIndex( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbThumb** acqThumb); 
  
  
  
  NS_IMETHOD HasIndex( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdb_bool* outHasIndex); 

  
  NS_IMETHOD EnableIndexOnSort( 
    nsIMdbEnv* ev, 
    mdb_column inColumn); 
  
  NS_IMETHOD QueryIndexOnSort( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdb_bool* outIndexOnSort); 
  
  NS_IMETHOD DisableIndexOnSort( 
    nsIMdbEnv* ev, 
    mdb_column inColumn); 
  

  morkStore*      mTable_Store;   

  
  morkRowSpace*   mTable_RowSpace; 

  morkRow*        mTable_MetaRow; 
  mdbOid          mTable_MetaRowOid; 
  
  morkRowMap*     mTable_RowMap;     
  morkArray       mTable_RowArray;   
  
  morkList        mTable_ChangeList;      
  mork_u2         mTable_ChangesCount; 
  mork_u2         mTable_ChangesMax;   
  
  
  mork_kind       mTable_Kind;
  
  mork_u1         mTable_Flags;         
  mork_priority   mTable_Priority;      
  mork_u1         mTable_GcUses;        
  mork_u1         mTable_Pad;      

public: 
  
  void SetTableUnique() { mTable_Flags |= morkTable_kUniqueBit; }
  void SetTableVerbose() { mTable_Flags |= morkTable_kVerboseBit; }
  void SetTableNoted() { mTable_Flags |= morkTable_kNotedBit; }
  void SetTableRewrite() { mTable_Flags |= morkTable_kRewriteBit; }
  void SetTableNewMeta() { mTable_Flags |= morkTable_kNewMetaBit; }

  void ClearTableUnique() { mTable_Flags &= (mork_u1) ~morkTable_kUniqueBit; }
  void ClearTableVerbose() { mTable_Flags &= (mork_u1) ~morkTable_kVerboseBit; }
  void ClearTableNoted() { mTable_Flags &= (mork_u1) ~morkTable_kNotedBit; }
  void ClearTableRewrite() { mTable_Flags &= (mork_u1) ~morkTable_kRewriteBit; }
  void ClearTableNewMeta() { mTable_Flags &= (mork_u1) ~morkTable_kNewMetaBit; }

  mork_bool IsTableUnique() const
  { return ( mTable_Flags & morkTable_kUniqueBit ) != 0; }
  
  mork_bool IsTableVerbose() const
  { return ( mTable_Flags & morkTable_kVerboseBit ) != 0; }
  
  mork_bool IsTableNoted() const
  { return ( mTable_Flags & morkTable_kNotedBit ) != 0; }
  
  mork_bool IsTableRewrite() const
  { return ( mTable_Flags & morkTable_kRewriteBit ) != 0; }
  
  mork_bool IsTableNewMeta() const
  { return ( mTable_Flags & morkTable_kNewMetaBit ) != 0; }

public: 

  void SetTableDirty() { this->SetNodeDirty(); }
  void SetTableClean(morkEnv* ev);
   
  mork_bool IsTableClean() const { return this->IsNodeClean(); }
  mork_bool IsTableDirty() const { return this->IsNodeDirty(); }

public: 
  void* operator new(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev) CPP_THROW_NEW
  { return morkNode::MakeNew(inSize, ioHeap, ev); }
  
 

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkTable(); 
  
public: 
  morkTable(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioNodeHeap, morkStore* ioStore,
    nsIMdbHeap* ioSlotHeap, morkRowSpace* ioRowSpace,
    const mdbOid* inOptionalMetaRowOid, 
    mork_tid inTableId,
    mork_kind inKind, mork_bool inMustBeUnique);
  void CloseTable(morkEnv* ev); 

private: 
  morkTable(const morkTable& other);
  morkTable& operator=(const morkTable& other);

public: 
  mork_bool IsTable() const
  { return IsNode() && mNode_Derived == morkDerived_kTable; }


public: 
  static void NonTableTypeError(morkEnv* ev);
  static void NonTableTypeWarning(morkEnv* ev);
  static void NilRowSpaceError(morkEnv* ev);

public: 
  static void TableGcUsesUnderflowWarning(morkEnv* ev);

public: 

  mork_bool HasChangeOverflow() const
  { return mTable_ChangesCount >= mTable_ChangesMax; }

  void NoteTableSetAll(morkEnv* ev);
  void NoteTableMoveRow(morkEnv* ev, morkRow* ioRow, mork_pos inPos);

  void note_row_change(morkEnv* ev, mork_change inChange, morkRow* ioRow);
  void note_row_move(morkEnv* ev, morkRow* ioRow, mork_pos inNewPos);
  
  void NoteTableAddRow(morkEnv* ev, morkRow* ioRow)
  { this->note_row_change(ev, morkChange_kAdd, ioRow); }
  
  void NoteTableCutRow(morkEnv* ev, morkRow* ioRow)
  { this->note_row_change(ev, morkChange_kCut, ioRow); }
  
protected: 

  morkRow* find_member_row(morkEnv* ev, morkRow* ioRow);
  void build_row_map(morkEnv* ev);

public: 
  
  mork_bool MaybeDirtySpaceStoreAndTable();

  morkRow* GetMetaRow(morkEnv* ev, const mdbOid* inOptionalMetaRowOid);
  
  mork_u2 AddTableGcUse(morkEnv* ev);
  mork_u2 CutTableGcUse(morkEnv* ev);

  

  mork_seed TableSeed() const { return mTable_RowArray.mArray_Seed; }
  
  morkRow* SafeRowAt(morkEnv* ev, mork_pos inPos)
  { return (morkRow*) mTable_RowArray.SafeAt(ev, inPos); }

  nsIMdbTable* AcquireTableHandle(morkEnv* ev); 
  
  mork_count GetRowCount() const { return mTable_RowArray.mArray_Fill; }

  mork_bool IsTableUsed() const
  { return (mTable_GcUses != 0 || this->GetRowCount() != 0); }

  void GetTableOid(morkEnv* ev, mdbOid* outOid);
  mork_pos  ArrayHasOid(morkEnv* ev, const mdbOid* inOid);
  mork_bool MapHasOid(morkEnv* ev, const mdbOid* inOid);
  mork_bool AddRow(morkEnv* ev, morkRow* ioRow); 
  mork_bool CutRow(morkEnv* ev, morkRow* ioRow); 
  mork_bool CutAllRows(morkEnv* ev); 
  
  mork_pos MoveRow(morkEnv* ev, morkRow* ioRow, 
    mork_pos inHintFromPos, 
    mork_pos inToPos); 
    
    


  morkTableRowCursor* NewTableRowCursor(morkEnv* ev, mork_pos inRowPos);

public: 
  static void SlotWeakTable(morkTable* me,
    morkEnv* ev, morkTable** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongTable(morkTable* me,
    morkEnv* ev, morkTable** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};




#define morkTableChange_kCut ((mork_pos) -1) /* shows row was cut */
#define morkTableChange_kAdd ((mork_pos) -2) /* shows row was added */
#define morkTableChange_kNone ((mork_pos) -3) /* unknown change */

class morkTableChange : public morkNext { 
public: 

  morkRow*  mTableChange_Row; 
  
  mork_pos  mTableChange_Pos; 

public:
  morkTableChange(morkEnv* ev, mork_change inChange, morkRow* ioRow);
  
  
  morkTableChange(morkEnv* ev, morkRow* ioRow, mork_pos inPos);
  

public:
  void UnknownChangeError(morkEnv* ev) const; 
  void NegativeMovePosError(morkEnv* ev) const; 
  
public:
  
  mork_bool IsAddRowTableChange() const
  { return ( mTableChange_Pos == morkTableChange_kAdd ); }
  
  mork_bool IsCutRowTableChange() const
  { return ( mTableChange_Pos == morkTableChange_kCut ); }
  
  mork_bool IsMoveRowTableChange() const
  { return ( mTableChange_Pos >= 0 ); }

public:
  
  mork_pos GetMovePos() const { return mTableChange_Pos; }
  
};



#define morkDerived_kTableMap 0x744D /* ascii 'tM' */



#ifdef MORK_BEAD_OVER_NODE_MAPS
class morkTableMap : public morkBeadMap { 
#else
class morkTableMap : public morkNodeMap { 
#endif

public:

  virtual ~morkTableMap();
  morkTableMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);

public: 

#ifdef MORK_BEAD_OVER_NODE_MAPS
  mork_bool  AddTable(morkEnv* ev, morkTable* ioTable)
  { return this->AddBead(ev, ioTable); }
  

  mork_bool  CutTable(morkEnv* ev, mork_tid inTid)
  { return this->CutBead(ev, inTid); }
  
  
  morkTable*  GetTable(morkEnv* ev, mork_tid inTid)
  { return (morkTable*) this->GetBead(ev, inTid); }
  

  mork_num CutAllTables(morkEnv* ev)
  { return this->CutAllBeads(ev); }
  
  
#else 
  mork_bool  AddTable(morkEnv* ev, morkTable* ioTable)
  { return this->AddNode(ev, ioTable->TableId(), ioTable); }
  

  mork_bool  CutTable(morkEnv* ev, mork_tid inTid)
  { return this->CutNode(ev, inTid); }
  
  
  morkTable*  GetTable(morkEnv* ev, mork_tid inTid)
  { return (morkTable*) this->GetNode(ev, inTid); }
  

  mork_num CutAllTables(morkEnv* ev)
  { return this->CutAllNodes(ev); }
  
#endif 

};

#ifdef MORK_BEAD_OVER_NODE_MAPS
class morkTableMapIter: public morkBeadMapIter {
#else
class morkTableMapIter: public morkMapIter{ 
#endif

public:

#ifdef MORK_BEAD_OVER_NODE_MAPS
  morkTableMapIter(morkEnv* ev, morkTableMap* ioMap)
  : morkBeadMapIter(ev, ioMap) { }
 
  morkTableMapIter( ) : morkBeadMapIter()  { }
  void InitTableMapIter(morkEnv* ev, morkTableMap* ioMap)
  { this->InitBeadMapIter(ev, ioMap); }
   
  morkTable* FirstTable(morkEnv* ev)
  { return (morkTable*) this->FirstBead(ev); }
  
  morkTable* NextTable(morkEnv* ev)
  { return (morkTable*) this->NextBead(ev); }
  
  morkTable* HereTable(morkEnv* ev)
  { return (morkTable*) this->HereBead(ev); }
  

#else 
  morkTableMapIter(morkEnv* ev, morkTableMap* ioMap)
  : morkMapIter(ev, ioMap) { }
 
  morkTableMapIter( ) : morkMapIter()  { }
  void InitTableMapIter(morkEnv* ev, morkTableMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  mork_change*
  FirstTable(morkEnv* ev, mork_tid* outTid, morkTable** outTable)
  { return this->First(ev, outTid, outTable); }
  
  mork_change*
  NextTable(morkEnv* ev, mork_tid* outTid, morkTable** outTable)
  { return this->Next(ev, outTid, outTable); }
  
  mork_change*
  HereTable(morkEnv* ev, mork_tid* outTid, morkTable** outTable)
  { return this->Here(ev, outTid, outTable); }
  
  
  mork_change*
  CutHereTable(morkEnv* ev, mork_tid* outTid, morkTable** outTable)
  { return this->CutHere(ev, outTid, outTable); }
#endif 
};



#endif

