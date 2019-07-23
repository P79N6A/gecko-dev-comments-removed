




































#ifndef _MORKPARSER_
#define _MORKPARSER_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKBLOB_
#include "morkBlob.h"
#endif

#ifndef _MORKSINK_
#include "morkSink.h"
#endif

#ifndef _MORKYARN_
#include "morkYarn.h"
#endif

#ifndef _MORKCELL_
#include "morkCell.h"
#endif


 




class morkPlace {
public:
  mork_pos   mPlace_Pos;   
  mork_line  mPlace_Line;  
  
  void ClearPlace()
  {
    mPlace_Pos = 0; mPlace_Line = 0;
  }

  void SetPlace(mork_pos inPos, mork_line inLine)
  {
    mPlace_Pos = inPos; mPlace_Line = inLine;
  }

  morkPlace() { mPlace_Pos = 0; mPlace_Line = 0; }
  
  morkPlace(mork_pos inPos, mork_line inLine)
  { mPlace_Pos = inPos; mPlace_Line = inLine; }
  
  morkPlace(const morkPlace& inPlace)
  : mPlace_Pos(inPlace.mPlace_Pos), mPlace_Line(inPlace.mPlace_Line) { }
};





class morkGlitch {
public:
  morkPlace    mGlitch_Place;   
  const char*  mGlitch_Comment; 

  morkGlitch() { mGlitch_Comment = 0; }
  
  morkGlitch(const morkPlace& inPlace, const char* inComment)
  : mGlitch_Place(inPlace), mGlitch_Comment(inComment) { }
};

































class morkMid {
public:
  mdbOid          mMid_Oid;  
  const morkBuf*  mMid_Buf;  
  
  morkMid()
  { mMid_Oid.mOid_Scope = 0; mMid_Oid.mOid_Id = morkId_kMinusOne;
   mMid_Buf = 0; }
  
  void InitMidWithCoil(morkCoil* ioCoil)
  { mMid_Oid.mOid_Scope = 0; mMid_Oid.mOid_Id = morkId_kMinusOne;
   mMid_Buf = ioCoil; }
    
  void ClearMid()
  { mMid_Oid.mOid_Scope = 0; mMid_Oid.mOid_Id = morkId_kMinusOne;
   mMid_Buf = 0; }

  morkMid(const morkMid& other)
  : mMid_Oid(other.mMid_Oid), mMid_Buf(other.mMid_Buf) { }
  
  mork_bool HasNoId() const 
  { return ( mMid_Oid.mOid_Id == morkId_kMinusOne ); }
  
  mork_bool HasSomeId() const 
  { return ( mMid_Oid.mOid_Id != morkId_kMinusOne ); }
};





class morkSpan {
public:
  morkPlace   mSpan_Start;
  morkPlace   mSpan_End;

public: 
  
public: 
  morkSpan() { } 
  
  morkPlace* AsPlace() { return &mSpan_Start; }
  const morkPlace* AsConstPlace() const { return &mSpan_Start; }
  
  void SetSpan(mork_pos inFromPos, mork_line inFromLine,
    mork_pos inToPos, mork_line inToLine)
  {
    mSpan_Start.SetPlace(inFromPos, inFromLine);
    mSpan_End.SetPlace(inToPos,inToLine);
  }

  
  void SetEndWithEnd(const morkSpan& inSpan) 
  { mSpan_End = inSpan.mSpan_End; }

  
  void SetStartWithEnd(const morkSpan& inSpan) 
  { mSpan_Start = inSpan.mSpan_End; }
  
  void ClearSpan()
  {
    mSpan_Start.mPlace_Pos = 0; mSpan_Start.mPlace_Line = 0;
    mSpan_End.mPlace_Pos = 0; mSpan_End.mPlace_Line = 0;
  }

  morkSpan(mork_pos inFromPos, mork_line inFromLine,
    mork_pos inToPos, mork_line inToLine)
  : mSpan_Start(inFromPos, inFromLine), mSpan_End(inToPos, inToLine)
  {   }
};





#define morkParser_kMinGranularity 512 /* parse at least half 0.5K at once */
#define morkParser_kMaxGranularity (64 * 1024) /* parse at most 64 K at once */

#define morkDerived_kParser 0x5073 /* ascii 'Ps' */
#define morkParser_kTag 0x70417253 /* ascii 'pArS' */




#define morkParser_kCellState      0 /* cell is tightest scope */
#define morkParser_kMetaState      1 /* meta is tightest scope */
#define morkParser_kRowState       2 /* row is tightest scope */
#define morkParser_kTableState     3 /* table is tightest scope */
#define morkParser_kDictState      4 /* dict is tightest scope */
#define morkParser_kPortState      5 /* port is tightest scope */

#define morkParser_kStartState     6 /* parsing has not yet begun */
#define morkParser_kDoneState      7 /* parsing is complete */
#define morkParser_kBrokenState    8 /* parsing is to broken to work */

class morkParser  : public morkNode {


  

  
  
  
  
  
  
  
  
  
  


protected: 
  
  nsIMdbHeap*   mParser_Heap;   
  morkStream*   mParser_Stream; 

  mork_u4       mParser_Tag; 
  mork_count    mParser_MoreGranularity; 

  mork_u4       mParser_State; 

  
  mork_pos      mParser_GroupContentStartPos; 

  morkMid       mParser_TableMid; 
  morkMid       mParser_RowMid;   
  morkMid       mParser_CellMid;  
  mork_gid      mParser_GroupId;  

  mork_bool     mParser_InPort;  
  mork_bool     mParser_InDict;  
  mork_bool     mParser_InCell;  
  mork_bool     mParser_InMeta;  

  mork_bool     mParser_InPortRow;  
  mork_bool     mParser_InRow;    
  mork_bool     mParser_InTable;  
  mork_bool     mParser_InGroup;  

  mork_change   mParser_AtomChange;  
  mork_change   mParser_CellChange;  
  mork_change   mParser_RowChange;   
  mork_change   mParser_TableChange; 

  mork_change   mParser_Change;     
  mork_bool     mParser_IsBroken;   
  mork_bool     mParser_IsDone;     
  mork_bool     mParser_DoMore;     

  morkMid       mParser_Mid;   
  

  
  morkCoil     mParser_ScopeCoil;   
  morkCoil     mParser_ValueCoil;   
  morkCoil     mParser_ColumnCoil;  
  morkCoil     mParser_StringCoil;  

  morkSpool    mParser_ScopeSpool;  
  morkSpool    mParser_ValueSpool;  
  morkSpool    mParser_ColumnSpool; 
  morkSpool    mParser_StringSpool; 

  
  morkYarn      mParser_MidYarn;   

  
  morkSpan      mParser_PortSpan; 

  
  morkSpan      mParser_GroupSpan; 
  morkSpan      mParser_DictSpan;
  morkSpan      mParser_AliasSpan;
  morkSpan      mParser_MetaSpan;
  morkSpan      mParser_TableSpan;
  morkSpan      mParser_RowSpan;
  morkSpan      mParser_CellSpan;
  morkSpan      mParser_ColumnSpan;
  morkSpan      mParser_SlotSpan;

private: 

  mork_pos HerePos() const
  { return mParser_PortSpan.mSpan_End.mPlace_Pos; }

  void SetHerePos(mork_pos inPos)
  { mParser_PortSpan.mSpan_End.mPlace_Pos = inPos; }

  void CountLineBreak()
  { ++mParser_PortSpan.mSpan_End.mPlace_Line; }
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkParser(); 
  
public: 
  morkParser(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap,
    morkStream* ioStream,  
    mdb_count inBytesPerParseSegment, 
    nsIMdbHeap* ioSlotHeap);
      
  void CloseParser(morkEnv* ev); 

private: 
  morkParser(const morkParser& other);
  morkParser& operator=(const morkParser& other);

public: 
  mork_bool IsParser() const
  { return IsNode() && mNode_Derived == morkDerived_kParser; }
  


public: 
  static void UnexpectedEofError(morkEnv* ev);
  static void EofInsteadOfHexError(morkEnv* ev);
  static void ExpectedEqualError(morkEnv* ev);
  static void ExpectedHexDigitError(morkEnv* ev, int c);
  static void NonParserTypeError(morkEnv* ev);
  static void UnexpectedByteInMetaWarning(morkEnv* ev);

public: 
  mork_bool GoodParserTag() const { return mParser_Tag == morkParser_kTag; }
  void NonGoodParserError(morkEnv* ev);
  void NonUsableParserError(morkEnv* ev);
  
  

public: 

    virtual void MidToYarn(morkEnv* ev,
      const morkMid& inMid,  
      mdbYarn* outYarn) = 0;
    
    
    
    
  

public: 
































  
  
  

  virtual void OnNewPort(morkEnv* ev, const morkPlace& inPlace) = 0;
  virtual void OnPortGlitch(morkEnv* ev, const morkGlitch& inGlitch) = 0;  
  virtual void OnPortEnd(morkEnv* ev, const morkSpan& inSpan) = 0;  

  virtual void OnNewGroup(morkEnv* ev, const morkPlace& inPlace, mork_gid inGid) = 0;
  virtual void OnGroupGlitch(morkEnv* ev, const morkGlitch& inGlitch) = 0;  
  virtual void OnGroupCommitEnd(morkEnv* ev, const morkSpan& inSpan) = 0;  
  virtual void OnGroupAbortEnd(morkEnv* ev, const morkSpan& inSpan) = 0;  

  virtual void OnNewPortRow(morkEnv* ev, const morkPlace& inPlace, 
    const morkMid& inMid, mork_change inChange) = 0;
  virtual void OnPortRowGlitch(morkEnv* ev, const morkGlitch& inGlitch) = 0;  
  virtual void OnPortRowEnd(morkEnv* ev, const morkSpan& inSpan) = 0;  

  virtual void OnNewTable(morkEnv* ev, const morkPlace& inPlace,
    const morkMid& inMid, mork_bool inCutAllRows) = 0;
  virtual void OnTableGlitch(morkEnv* ev, const morkGlitch& inGlitch) = 0;
  virtual void OnTableEnd(morkEnv* ev, const morkSpan& inSpan) = 0;
    
  virtual void OnNewMeta(morkEnv* ev, const morkPlace& inPlace) = 0;
  virtual void OnMetaGlitch(morkEnv* ev, const morkGlitch& inGlitch) = 0;
  virtual void OnMetaEnd(morkEnv* ev, const morkSpan& inSpan) = 0;

  virtual void OnMinusRow(morkEnv* ev) = 0;
  virtual void OnNewRow(morkEnv* ev, const morkPlace& inPlace, 
    const morkMid& inMid, mork_bool inCutAllCols) = 0;
  virtual void OnRowPos(morkEnv* ev, mork_pos inRowPos) = 0;  
  virtual void OnRowGlitch(morkEnv* ev, const morkGlitch& inGlitch) = 0;  
  virtual void OnRowEnd(morkEnv* ev, const morkSpan& inSpan) = 0;  

  virtual void OnNewDict(morkEnv* ev, const morkPlace& inPlace) = 0;
  virtual void OnDictGlitch(morkEnv* ev, const morkGlitch& inGlitch) = 0;  
  virtual void OnDictEnd(morkEnv* ev, const morkSpan& inSpan) = 0;  

  virtual void OnAlias(morkEnv* ev, const morkSpan& inSpan,
    const morkMid& inMid) = 0;

  virtual void OnAliasGlitch(morkEnv* ev, const morkGlitch& inGlitch) = 0;

  virtual void OnMinusCell(morkEnv* ev) = 0;
  virtual void OnNewCell(morkEnv* ev, const morkPlace& inPlace,
    const morkMid* inMid, const morkBuf* inBuf) = 0;
  
  
  
    
  virtual void OnCellGlitch(morkEnv* ev, const morkGlitch& inGlitch) = 0;
  virtual void OnCellForm(morkEnv* ev, mork_cscode inCharsetFormat) = 0;
  virtual void OnCellEnd(morkEnv* ev, const morkSpan& inSpan) = 0;
    
  virtual void OnValue(morkEnv* ev, const morkSpan& inSpan,
    const morkBuf& inBuf) = 0;

  virtual void OnValueMid(morkEnv* ev, const morkSpan& inSpan,
    const morkMid& inMid) = 0;

  virtual void OnRowMid(morkEnv* ev, const morkSpan& inSpan,
    const morkMid& inMid) = 0;

  virtual void OnTableMid(morkEnv* ev, const morkSpan& inSpan,
    const morkMid& inMid) = 0;
  

protected: 

  void ParseLoop(morkEnv* ev); 

  void StartParse(morkEnv* ev); 
  void StopParse(morkEnv* ev); 

  int NextChar(morkEnv* ev); 

  void OnCellState(morkEnv* ev);
  void OnMetaState(morkEnv* ev);
  void OnRowState(morkEnv* ev);
  void OnTableState(morkEnv* ev);
  void OnDictState(morkEnv* ev);
  void OnPortState(morkEnv* ev);
  void OnStartState(morkEnv* ev);
  
  void ReadCell(morkEnv* ev);
  void ReadRow(morkEnv* ev, int c);
  void ReadRowPos(morkEnv* ev);
  void ReadTable(morkEnv* ev);
  void ReadTableMeta(morkEnv* ev);
  void ReadDict(morkEnv* ev);
  mork_bool ReadContent(morkEnv* ev, mork_bool inInsideGroup);
  void ReadGroup(morkEnv* ev);
  mork_bool ReadEndGroupId(morkEnv* ev);
  mork_bool ReadAt(morkEnv* ev, mork_bool inInsideGroup);
  mork_bool FindGroupEnd(morkEnv* ev);
  void ReadMeta(morkEnv* ev, int inEndMeta);
  void ReadAlias(morkEnv* ev);
  mork_id ReadHex(morkEnv* ev, int* outNextChar);
  morkBuf* ReadValue(morkEnv* ev);
  morkBuf* ReadName(morkEnv* ev, int c);
  mork_bool ReadMid(morkEnv* ev, morkMid* outMid);
  void ReadDictForm(morkEnv *ev);
  void ReadCellForm(morkEnv *ev, int c);
  
  mork_bool MatchPattern(morkEnv* ev, const char* inPattern);
  
  void EndSpanOnThisByte(morkEnv* ev, morkSpan* ioSpan);
  void EndSpanOnLastByte(morkEnv* ev, morkSpan* ioSpan);
  void StartSpanOnLastByte(morkEnv* ev, morkSpan* ioSpan);
  
  void StartSpanOnThisByte(morkEnv* ev, morkSpan* ioSpan);
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int eat_line_break(morkEnv* ev, int inLast);
  int eat_line_continue(morkEnv* ev); 
  int eat_comment(morkEnv* ev); 
  

public: 
    
  mdb_count ParseMore( 
    morkEnv* ev,          
    mork_pos* outPos,     
    mork_bool* outDone,   
    mork_bool* outBroken  
  );
  
  
public: 
  static void SlotWeakParser(morkParser* me,
    morkEnv* ev, morkParser** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongParser(morkParser* me,
    morkEnv* ev, morkParser** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 

