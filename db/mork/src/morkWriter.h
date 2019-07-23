




































#ifndef _MORKWRITER_
#define _MORKWRITER_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKROWMAP_
#include "morkRowMap.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKATOMMAP_
#include "morkAtomMap.h"
#endif

#ifndef _MORKATOMSPACE_
#include "morkAtomSpace.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKSTREAM_
#include "morkStream.h"
#endif




#define morkWriter_kStreamBufSize (16 * 1024) /* buffer size for stream */ 

#define morkDerived_kWriter 0x5772 /* ascii 'Wr' */

#define morkWriter_kPhaseNothingDone          0 /* nothing has yet been done */
#define morkWriter_kPhaseDirtyAllDone         1 /* DirtyAll() is done */
#define morkWriter_kPhasePutHeaderDone        2 /* PutHeader() is done */

#define morkWriter_kPhaseRenumberAllDone      3 /* RenumberAll() is done */

#define morkWriter_kPhaseStoreAtomSpaces      4 /*mWriter_StoreAtomSpacesIter*/
#define morkWriter_kPhaseAtomSpaceAtomAids    5 /*mWriter_AtomSpaceAtomAidsIter*/

#define morkWriter_kPhaseStoreRowSpacesTables 6 /*mWriter_StoreRowSpacesIter*/
#define morkWriter_kPhaseRowSpaceTables       7 /*mWriter_RowSpaceTablesIter*/
#define morkWriter_kPhaseTableRowArray        8 /*mWriter_TableRowArrayPos */

#define morkWriter_kPhaseStoreRowSpacesRows   9 /*mWriter_StoreRowSpacesIter*/
#define morkWriter_kPhaseRowSpaceRows        10 /*mWriter_RowSpaceRowsIter*/

#define morkWriter_kPhaseContentDone         11 /* all content written */
#define morkWriter_kPhaseWritingDone         12 /* everthing has been done */

#define morkWriter_kCountNumberOfPhases      13 /* part of mWrite_TotalCount */

#define morkWriter_kMaxColumnNameSize        128 /* longest writable col name */

#define morkWriter_kMaxIndent 66 /* default value for mWriter_MaxIndent */
#define morkWriter_kMaxLine   78 /* default value for mWriter_MaxLine */

#define morkWriter_kYarnEscapeSlop  4 /* guess average yarn escape overhead */

#define morkWriter_kTableMetaCellDepth 4 /* */
#define morkWriter_kTableMetaCellValueDepth 6 /* */

#define morkWriter_kDictMetaCellDepth 4 /* */
#define morkWriter_kDictMetaCellValueDepth 6 /* */

#define morkWriter_kDictAliasDepth 2 /* */
#define morkWriter_kDictAliasValueDepth 4 /* */

#define morkWriter_kRowDepth 2 /* */
#define morkWriter_kRowCellDepth 4 /* */
#define morkWriter_kRowCellValueDepth 6 /* */

#define morkWriter_kGroupBufSize 64 /* */




#define morkWriter_kFileHeader "// <!-- <mdb:mork:z v=\"1.4\"/> -->"

class morkWriter : public morkNode { 


  
  
  
  
  
  
  
  
  

public: 

  morkStore*   mWriter_Store;      
  nsIMdbFile*  mWriter_File;       
  nsIMdbFile*  mWriter_Bud;        
  morkStream*  mWriter_Stream;     
  nsIMdbHeap*  mWriter_SlotHeap;   

  
  mork_gid     mWriter_CommitGroupIdentity; 
  
  
  char         mWriter_GroupBuf[ morkWriter_kGroupBufSize ];
  mork_fill    mWriter_GroupBufFill; 
  
  mork_count   mWriter_TotalCount;  
  mork_count   mWriter_DoneCount;   
  
  mork_size    mWriter_LineSize;  
  mork_size    mWriter_MaxIndent; 
  mork_size    mWriter_MaxLine;   
  
  mork_cscode  mWriter_TableForm;     
  mork_scope   mWriter_TableAtomScope;   
  mork_scope   mWriter_TableRowScope;    
  mork_kind    mWriter_TableKind;        
  
  mork_cscode  mWriter_RowForm;         
  mork_scope   mWriter_RowAtomScope;    
  mork_scope   mWriter_RowScope;        
  
  mork_cscode  mWriter_DictForm;      
  mork_scope   mWriter_DictAtomScope;    
 
  mork_bool    mWriter_NeedDirtyAll;  
  mork_bool    mWriter_Incremental;   
  mork_bool    mWriter_DidStartDict;  
  mork_bool    mWriter_DidEndDict;    

  mork_bool    mWriter_SuppressDirtyRowNewline; 
  mork_bool    mWriter_DidStartGroup; 
  mork_bool    mWriter_DidEndGroup;    
  mork_u1      mWriter_Phase;         

  mork_bool    mWriter_BeVerbose; 
  
  
  mork_u1      mWriter_Pad[ 3 ];  

  mork_pos     mWriter_TableRowArrayPos;  
   
  char         mWriter_SafeNameBuf[ (morkWriter_kMaxColumnNameSize * 2) + 4 ];
  

  char         mWriter_ColNameBuf[ morkWriter_kMaxColumnNameSize + 4 ];
  
  
  mdbYarn      mWriter_ColYarn; 
  
  
  mdbYarn      mWriter_SafeYarn; 
  

  morkAtomSpaceMapIter  mWriter_StoreAtomSpacesIter;   
  morkAtomAidMapIter  mWriter_AtomSpaceAtomAidsIter; 
  
  morkRowSpaceMapIter  mWriter_StoreRowSpacesIter;    
  morkTableMapIter  mWriter_RowSpaceTablesIter;    
  
#ifdef MORK_ENABLE_PROBE_MAPS
  morkRowProbeMapIter  mWriter_RowSpaceRowsIter; 
#else 
  morkRowMapIter  mWriter_RowSpaceRowsIter;      
#endif 
   

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkWriter(); 
  
public: 
  morkWriter(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, morkStore* ioStore, nsIMdbFile* ioFile,
    nsIMdbHeap* ioSlotHeap);
  void CloseWriter(morkEnv* ev); 

private: 
  morkWriter(const morkWriter& other);
  morkWriter& operator=(const morkWriter& other);

public: 
  mork_bool IsWriter() const
  { return IsNode() && mNode_Derived == morkDerived_kWriter; }


public: 
  static void NonWriterTypeError(morkEnv* ev);
  static void NilWriterStoreError(morkEnv* ev);
  static void NilWriterBudError(morkEnv* ev);
  static void NilWriterStreamError(morkEnv* ev);
  static void NilWriterFileError(morkEnv* ev);
  static void UnsupportedPhaseError(morkEnv* ev);

public: 
  void ChangeRowForm(morkEnv* ev, mork_cscode inNewForm);
  void ChangeDictForm(morkEnv* ev, mork_cscode inNewForm);
  void ChangeDictAtomScope(morkEnv* ev, mork_scope inScope);

public: 
  mork_bool DidStartDict() const { return mWriter_DidStartDict; }
  mork_bool DidEndDict() const { return mWriter_DidEndDict; }
  
  void IndentAsNeeded(morkEnv* ev, mork_size inDepth)
  { 
    if ( mWriter_LineSize > mWriter_MaxIndent )
      mWriter_LineSize = mWriter_Stream->PutIndent(ev, inDepth);
  }
  
  void IndentOverMaxLine(morkEnv* ev,
    mork_size inPendingSize, mork_size inDepth)
  { 
    if ( mWriter_LineSize + inPendingSize > mWriter_MaxLine )
      mWriter_LineSize = mWriter_Stream->PutIndent(ev, inDepth);
  }

public: 

  void MakeWriterStream(morkEnv* ev); 

public: 
  
  mork_bool WriteMore(morkEnv* ev); 
  
  mork_bool IsWritingDone() const 
  { return mWriter_Phase == morkWriter_kPhaseWritingDone; }

public: 
  mork_bool DirtyAll(morkEnv* ev);
  
  
  
  
  
  
  
  
  

public: 

  mork_bool StartGroup(morkEnv* ev);
  mork_bool CommitGroup(morkEnv* ev);
  mork_bool AbortGroup(morkEnv* ev);

public: 
  mork_bool OnNothingDone(morkEnv* ev);
  mork_bool OnDirtyAllDone(morkEnv* ev);
  mork_bool OnPutHeaderDone(morkEnv* ev);

  mork_bool OnRenumberAllDone(morkEnv* ev);

  mork_bool OnStoreAtomSpaces(morkEnv* ev);
  mork_bool OnAtomSpaceAtomAids(morkEnv* ev);

  mork_bool OnStoreRowSpacesTables(morkEnv* ev);
  mork_bool OnRowSpaceTables(morkEnv* ev);
  mork_bool OnTableRowArray(morkEnv* ev);

  mork_bool OnStoreRowSpacesRows(morkEnv* ev);
  mork_bool OnRowSpaceRows(morkEnv* ev);

  mork_bool OnContentDone(morkEnv* ev);
  mork_bool OnWritingDone(morkEnv* ev);

public: 
  mork_bool PutTableDict(morkEnv* ev, morkTable* ioTable);
  mork_bool PutRowDict(morkEnv* ev, morkRow* ioRow);

public: 
  mork_bool PutTable(morkEnv* ev, morkTable* ioTable);
  mork_bool PutRow(morkEnv* ev, morkRow* ioRow);
  mork_bool PutRowCells(morkEnv* ev, morkRow* ioRow);
  mork_bool PutVerboseRowCells(morkEnv* ev, morkRow* ioRow);
  
  mork_bool PutCell(morkEnv* ev, morkCell* ioCell, mork_bool inWithVal);
  mork_bool PutVerboseCell(morkEnv* ev, morkCell* ioCell, mork_bool inWithVal);
  
  mork_bool PutTableChange(morkEnv* ev, const morkTableChange* inChange);

public: 

  mork_bool IsYarnAllValue(const mdbYarn* inYarn);

  mork_size WriteYarn(morkEnv* ev, const mdbYarn* inYarn);
  
  
  

  mork_size WriteAtom(morkEnv* ev, const morkAtom* inAtom);
  
  
  

  void WriteAllStoreTables(morkEnv* ev);
  void WriteAtomSpaceAsDict(morkEnv* ev, morkAtomSpace* ioSpace);
  
  void WriteTokenToTokenMetaCell(morkEnv* ev, mork_token inCol,
    mork_token inValue);
  void WriteStringToTokenDictCell(morkEnv* ev, const char* inCol, 
    mork_token inValue);
  

  void StartDict(morkEnv* ev);
  void EndDict(morkEnv* ev);

  void StartTable(morkEnv* ev, morkTable* ioTable);
  void EndTable(morkEnv* ev);

public: 
  static void SlotWeakWriter(morkWriter* me,
    morkEnv* ev, morkWriter** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongWriter(morkWriter* me,
    morkEnv* ev, morkWriter** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
