




































#ifndef _MORKROW_
#define _MORKROW_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKCELL_
#include "morkCell.h"
#endif



class nsIMdbRow;
class nsIMdbCell;
#define morkDerived_kRow 0x5277 /* ascii 'Rw' */

#define morkRow_kMaxGcUses 0x0FF /* max for 8-bit unsigned int */
#define morkRow_kMaxLength 0x0FFFF /* max for 16-bit unsigned int */
#define morkRow_kMinusOneRid ((mork_rid) -1)

#define morkRow_kTag 'r' /* magic signature for mRow_Tag */

#define morkRow_kNotedBit   ((mork_u1) (1 << 0)) /* space has change notes */
#define morkRow_kRewriteBit ((mork_u1) (1 << 1)) /* must rewrite all cells */
#define morkRow_kDirtyBit   ((mork_u1) (1 << 2)) /* row has been changed */

class morkRow{ 

public: 

  morkRowSpace*   mRow_Space;  
  morkRowObject*  mRow_Object; 
  morkCell*       mRow_Cells;
  mdbOid          mRow_Oid;
  
  mork_delta      mRow_Delta;   

  mork_u2         mRow_Length;     
  mork_u2         mRow_Seed;       

  mork_u1         mRow_GcUses;  
  mork_u1         mRow_Pad;     
  mork_u1         mRow_Flags;   
  mork_u1         mRow_Tag;     

public: 
  
  mork_bool HasRowDelta() const { return ( mRow_Delta != 0 ); }
  
  void ClearRowDelta() { mRow_Delta = 0; }
  
  void SetRowDelta(mork_column inCol, mork_change inChange)
  { morkDelta_Init(mRow_Delta, inCol, inChange); }
  
  mork_column  GetDeltaColumn() const { return morkDelta_Column(mRow_Delta); }
  mork_change  GetDeltaChange() const { return morkDelta_Change(mRow_Delta); }

public: 

  void NoteRowSetAll(morkEnv* ev);
  void NoteRowSetCol(morkEnv* ev, mork_column inCol);
  void NoteRowAddCol(morkEnv* ev, mork_column inCol);
  void NoteRowCutCol(morkEnv* ev, mork_column inCol);

public: 

  void SetRowNoted() { mRow_Flags |= morkRow_kNotedBit; }
  void SetRowRewrite() { mRow_Flags |= morkRow_kRewriteBit; }
  void SetRowDirty() { mRow_Flags |= morkRow_kDirtyBit; }

  void ClearRowNoted() { mRow_Flags &= (mork_u1) ~morkRow_kNotedBit; }
  void ClearRowRewrite() { mRow_Flags &= (mork_u1) ~morkRow_kRewriteBit; }
  void SetRowClean() { mRow_Flags = 0; mRow_Delta = 0; }
  
  mork_bool IsRowNoted() const
  { return ( mRow_Flags & morkRow_kNotedBit ) != 0; }
  
  mork_bool IsRowRewrite() const
  { return ( mRow_Flags & morkRow_kRewriteBit ) != 0; }
   
  mork_bool IsRowClean() const
  { return ( mRow_Flags & morkRow_kDirtyBit ) == 0; }
  
  mork_bool IsRowDirty() const
  { return ( mRow_Flags & morkRow_kDirtyBit ) != 0; }
  
  mork_bool IsRowUsed() const
  { return mRow_GcUses != 0; }

public: 
  morkRow( ) { }
  morkRow(const mdbOid* inOid) :mRow_Oid(*inOid) { }
  void InitRow(morkEnv* ev, const mdbOid* inOid, morkRowSpace* ioSpace,
    mork_size inLength, morkPool* ioPool);
    

  morkRowObject* AcquireRowObject(morkEnv* ev, morkStore* ioStore);
  nsIMdbRow* AcquireRowHandle(morkEnv* ev, morkStore* ioStore);
  nsIMdbCell* AcquireCellHandle(morkEnv* ev, morkCell* ioCell,
    mdb_column inColumn, mork_pos inPos);
  
  mork_u2 AddRowGcUse(morkEnv* ev);
  mork_u2 CutRowGcUse(morkEnv* ev);

  
  mork_bool MaybeDirtySpaceStoreAndRow();

public: 

  void cut_all_index_entries(morkEnv* ev);

  

  mork_count CountOverlap(morkEnv* ev, morkCell* ioVector, mork_fill inFill);
  
  
  
  
  
  
  
  
  
  

  void MergeCells(morkEnv* ev, morkCell* ioVector,
    mork_fill inVecLength, mork_fill inOldRowFill, mork_fill inOverlap);
  
  
  

  void TakeCells(morkEnv* ev, morkCell* ioVector, mork_fill inVecLength,
    morkStore* ioStore);

  morkCell* NewCell(morkEnv* ev, mdb_column inColumn, mork_pos* outPos,
    morkStore* ioStore);
  morkCell* GetCell(morkEnv* ev, mdb_column inColumn, mork_pos* outPos) const;
  morkCell* CellAt(morkEnv* ev, mork_pos inPos) const;

  mork_aid GetCellAtomAid(morkEnv* ev, mdb_column inColumn) const;
  
  
  
  
  

public: 

  void DirtyAllRowContent(morkEnv* ev);

  morkStore* GetRowSpaceStore(morkEnv* ev) const;

  void AddColumn(morkEnv* ev, mdb_column inColumn,
    const mdbYarn* inYarn, morkStore* ioStore);

  morkAtom* GetColumnAtom(morkEnv* ev, mdb_column inColumn);

  void NextColumn(morkEnv* ev, mdb_column* ioColumn, mdbYarn* outYarn);

  void SeekColumn(morkEnv* ev, mdb_pos inPos, 
	  mdb_column* outColumn, mdbYarn* outYarn);

  void CutColumn(morkEnv* ev, mdb_column inColumn);

  morkRowCellCursor* NewRowCellCursor(morkEnv* ev, mdb_pos inPos);
  
  void EmptyAllCells(morkEnv* ev);
  void AddRow(morkEnv* ev, const morkRow* inSourceRow);
  void SetRow(morkEnv* ev, const morkRow* inSourceRow);
  void CutAllColumns(morkEnv* ev);

  void OnZeroRowGcUse(morkEnv* ev);
  

public: 

  mork_bool IsRow() const { return mRow_Tag == morkRow_kTag; }

public: 

  mork_u4 HashRow() const
  {
    return (mRow_Oid.mOid_Scope << 16) ^ mRow_Oid.mOid_Id;
  }

  mork_bool EqualRow(const morkRow* ioRow) const
  {
    return
    (
      ( mRow_Oid.mOid_Scope == ioRow->mRow_Oid.mOid_Scope ) 
      && ( mRow_Oid.mOid_Id == ioRow->mRow_Oid.mOid_Id )
    );
  }

  mork_bool EqualOid(const mdbOid* ioOid) const
  {
    return
    (
      ( mRow_Oid.mOid_Scope == ioOid->mOid_Scope ) 
      && ( mRow_Oid.mOid_Id == ioOid->mOid_Id )
    );
  }

public: 
  static void ZeroColumnError(morkEnv* ev);
  static void LengthBeyondMaxError(morkEnv* ev);
  static void NilCellsError(morkEnv* ev);
  static void NonRowTypeError(morkEnv* ev);
  static void NonRowTypeWarning(morkEnv* ev);
  static void GcUsesUnderflowWarning(morkEnv* ev);

private: 
  morkRow(const morkRow& other);
  morkRow& operator=(const morkRow& other);
};



#endif 

