




































#ifndef _MORKBUILDER_
#define _MORKBUILDER_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKPARSER_
#include "morkParser.h"
#endif


 


#define morkBuilder_kCellsVecSize 64

#define morkBuilder_kDefaultBytesPerParseSegment 512 /* plausible to big */

#define morkDerived_kBuilder 0x4275 /* ascii 'Bu' */

class morkBuilder  : public morkParser {


  

  
  
  
  
  
  
  
  
  
  


  
  
    
  
  

  
 
  
  
    
  
  
  
    
  
  
  
  
    
  
  
    
  
  
  
  
  
    
  
  
  
  

  
  
  
  
  
    
  
  
  
  
  
  
  
  
  
  
  
  


protected: 
  
  
  morkStore*       mBuilder_Store; 
  
  
  morkTable*       mBuilder_Table;    
  morkRow*         mBuilder_Row;      
  morkCell*        mBuilder_Cell;     
  
  morkRowSpace*    mBuilder_RowSpace;  
  morkAtomSpace*   mBuilder_AtomSpace; 
  
  morkAtomSpace*   mBuilder_OidAtomSpace;   
  morkAtomSpace*   mBuilder_ScopeAtomSpace; 
  
  
  mdbOid           mBuilder_TableOid; 
  mdbOid           mBuilder_RowOid;   
      
  
  mork_cscode      mBuilder_PortForm;       
  mork_scope       mBuilder_PortRowScope;   
  mork_scope       mBuilder_PortAtomScope;  

  
  mork_cscode      mBuilder_TableForm;       
  mork_scope       mBuilder_TableRowScope;   
  mork_scope       mBuilder_TableAtomScope;  
  mork_kind        mBuilder_TableKind;       
  
  mork_token       mBuilder_TableStatus;  
  
  mork_priority    mBuilder_TablePriority;   
  mork_bool        mBuilder_TableIsUnique;   
  mork_bool        mBuilder_TableIsVerbose;  
  mork_u1          mBuilder_TablePadByte;    
  
  
  mork_cscode      mBuilder_RowForm;       
  mork_scope       mBuilder_RowRowScope;   
  mork_scope       mBuilder_RowAtomScope;  

  
  mork_cscode      mBuilder_CellForm;       
  mork_scope       mBuilder_CellAtomScope;  

  mork_cscode      mBuilder_DictForm;       
  mork_scope       mBuilder_DictAtomScope;  

  mork_token*      mBuilder_MetaTokenSlot; 
  
  
  
  
  
  
  mork_bool        mBuilder_DoCutRow;    
  mork_bool        mBuilder_DoCutCell;   
  mork_u1          mBuilder_row_pad;    
  mork_u1          mBuilder_cell_pad;   
  
  morkCell         mBuilder_CellsVec[ morkBuilder_kCellsVecSize + 1 ];
  mork_fill        mBuilder_CellsVecFill; 
  
  
  
  
protected: 

  mork_bool  CellVectorIsFull() const
  { return ( mBuilder_CellsVecFill == morkBuilder_kCellsVecSize ); }
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkBuilder(); 
  
public: 
  morkBuilder(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap,
    morkStream* ioStream,  
    mdb_count inBytesPerParseSegment, 
    nsIMdbHeap* ioSlotHeap, morkStore* ioStore
    );
      
  void CloseBuilder(morkEnv* ev); 

private: 
  morkBuilder(const morkBuilder& other);
  morkBuilder& operator=(const morkBuilder& other);

public: 
  mork_bool IsBuilder() const
  { return IsNode() && mNode_Derived == morkDerived_kBuilder; }


public: 
  static void NonBuilderTypeError(morkEnv* ev);
  static void NilBuilderCellError(morkEnv* ev);
  static void NilBuilderRowError(morkEnv* ev);
  static void NilBuilderTableError(morkEnv* ev);
  static void NonColumnSpaceScopeError(morkEnv* ev);
  
  void LogGlitch(morkEnv* ev, const morkGlitch& inGlitch, 
    const char* inKind);

public: 

  morkCell* AddBuilderCell(morkEnv* ev,
    const morkMid& inMid, mork_change inChange);

  void FlushBuilderCells(morkEnv* ev);
  

public: 

    virtual void MidToYarn(morkEnv* ev,
      const morkMid& inMid,  
      mdbYarn* outYarn);
    
    
    
    
  

public: 

  virtual void OnNewPort(morkEnv* ev, const morkPlace& inPlace);
  virtual void OnPortGlitch(morkEnv* ev, const morkGlitch& inGlitch);  
  virtual void OnPortEnd(morkEnv* ev, const morkSpan& inSpan);  

  virtual void OnNewGroup(morkEnv* ev, const morkPlace& inPlace, mork_gid inGid);
  virtual void OnGroupGlitch(morkEnv* ev, const morkGlitch& inGlitch);  
  virtual void OnGroupCommitEnd(morkEnv* ev, const morkSpan& inSpan);  
  virtual void OnGroupAbortEnd(morkEnv* ev, const morkSpan& inSpan);  

  virtual void OnNewPortRow(morkEnv* ev, const morkPlace& inPlace, 
    const morkMid& inMid, mork_change inChange);
  virtual void OnPortRowGlitch(morkEnv* ev, const morkGlitch& inGlitch);  
  virtual void OnPortRowEnd(morkEnv* ev, const morkSpan& inSpan);  

  virtual void OnNewTable(morkEnv* ev, const morkPlace& inPlace,
    const morkMid& inMid, mork_bool inCutAllRows);
  virtual void OnTableGlitch(morkEnv* ev, const morkGlitch& inGlitch);
  virtual void OnTableEnd(morkEnv* ev, const morkSpan& inSpan);
    
  virtual void OnNewMeta(morkEnv* ev, const morkPlace& inPlace);
  virtual void OnMetaGlitch(morkEnv* ev, const morkGlitch& inGlitch);
  virtual void OnMetaEnd(morkEnv* ev, const morkSpan& inSpan);

  virtual void OnMinusRow(morkEnv* ev);
  virtual void OnNewRow(morkEnv* ev, const morkPlace& inPlace, 
    const morkMid& inMid, mork_bool inCutAllCols);
  virtual void OnRowPos(morkEnv* ev, mork_pos inRowPos);  
  virtual void OnRowGlitch(morkEnv* ev, const morkGlitch& inGlitch);  
  virtual void OnRowEnd(morkEnv* ev, const morkSpan& inSpan);  

  virtual void OnNewDict(morkEnv* ev, const morkPlace& inPlace);
  virtual void OnDictGlitch(morkEnv* ev, const morkGlitch& inGlitch);  
  virtual void OnDictEnd(morkEnv* ev, const morkSpan& inSpan);  

  virtual void OnAlias(morkEnv* ev, const morkSpan& inSpan,
    const morkMid& inMid);

  virtual void OnAliasGlitch(morkEnv* ev, const morkGlitch& inGlitch);

  virtual void OnMinusCell(morkEnv* ev);
  virtual void OnNewCell(morkEnv* ev, const morkPlace& inPlace,
    const morkMid* inMid, const morkBuf* inBuf);
  
  
  

  virtual void OnCellGlitch(morkEnv* ev, const morkGlitch& inGlitch);
  virtual void OnCellForm(morkEnv* ev, mork_cscode inCharsetFormat);
  virtual void OnCellEnd(morkEnv* ev, const morkSpan& inSpan);
    
  virtual void OnValue(morkEnv* ev, const morkSpan& inSpan,
    const morkBuf& inBuf);

  virtual void OnValueMid(morkEnv* ev, const morkSpan& inSpan,
    const morkMid& inMid);

  virtual void OnRowMid(morkEnv* ev, const morkSpan& inSpan,
    const morkMid& inMid);

  virtual void OnTableMid(morkEnv* ev, const morkSpan& inSpan,
    const morkMid& inMid);
  

public: 
  
  
public: 
  static void SlotWeakBuilder(morkBuilder* me,
    morkEnv* ev, morkBuilder** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongBuilder(morkBuilder* me,
    morkEnv* ev, morkBuilder** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
