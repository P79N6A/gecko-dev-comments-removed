




































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

#ifndef _MORKSTREAM_
#include "morkStream.h"
#endif

#ifndef _MORKBLOB_
#include "morkBlob.h"
#endif

#ifndef _MORKSINK_
#include "morkSink.h"
#endif

#ifndef _MORKCH_
#include "morkCh.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif






 void
morkParser::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseParser(ev);
    this->MarkShut();
  }
}


morkParser::~morkParser() 
{
  MORK_ASSERT(mParser_Heap==0);
  MORK_ASSERT(mParser_Stream==0);
}


morkParser::morkParser(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap, 
  morkStream* ioStream, mdb_count inBytesPerParseSegment,
  nsIMdbHeap* ioSlotHeap)
: morkNode(ev, inUsage, ioHeap)
, mParser_Heap( 0 )
, mParser_Stream( 0 )
, mParser_MoreGranularity( inBytesPerParseSegment )
, mParser_State( morkParser_kStartState )

, mParser_GroupContentStartPos( 0 )

, mParser_TableMid(  )
, mParser_RowMid(  )
, mParser_CellMid(  )
    
, mParser_InPort( morkBool_kFalse )
, mParser_InDict( morkBool_kFalse )
, mParser_InCell( morkBool_kFalse )
, mParser_InMeta( morkBool_kFalse )
    
, mParser_InPortRow( morkBool_kFalse )
, mParser_InRow( morkBool_kFalse )
, mParser_InTable( morkBool_kFalse )
, mParser_InGroup( morkBool_kFalse )

, mParser_AtomChange( morkChange_kNil )
, mParser_CellChange( morkChange_kNil )
, mParser_RowChange( morkChange_kNil )
, mParser_TableChange( morkChange_kNil )

, mParser_Change( morkChange_kNil )
, mParser_IsBroken( morkBool_kFalse )
, mParser_IsDone( morkBool_kFalse )
, mParser_DoMore( morkBool_kTrue )
    
, mParser_Mid()

, mParser_ScopeCoil(ev, ioSlotHeap)
, mParser_ValueCoil(ev, ioSlotHeap)
, mParser_ColumnCoil(ev, ioSlotHeap)
, mParser_StringCoil(ev, ioSlotHeap)

, mParser_ScopeSpool(ev, &mParser_ScopeCoil)
, mParser_ValueSpool(ev, &mParser_ValueCoil)
, mParser_ColumnSpool(ev, &mParser_ColumnCoil)
, mParser_StringSpool(ev, &mParser_StringCoil)

, mParser_MidYarn(ev, morkUsage_kMember, ioSlotHeap)
{
  if ( inBytesPerParseSegment < morkParser_kMinGranularity )
    inBytesPerParseSegment = morkParser_kMinGranularity;
  else if ( inBytesPerParseSegment > morkParser_kMaxGranularity )
    inBytesPerParseSegment = morkParser_kMaxGranularity;
    
  mParser_MoreGranularity = inBytesPerParseSegment;

  if ( ioSlotHeap && ioStream )
  {
    nsIMdbHeap_SlotStrongHeap(ioSlotHeap, ev, &mParser_Heap);
    morkStream::SlotStrongStream(ioStream, ev, &mParser_Stream);
    
    if ( ev->Good() )
    {
      mParser_Tag = morkParser_kTag;
      mNode_Derived = morkDerived_kParser;
    }
  }
  else
    ev->NilPointerError();
}

 void
morkParser::CloseParser(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      if ( !this->IsShutNode() )
      {
        mParser_ScopeCoil.CloseCoil(ev);
        mParser_ValueCoil.CloseCoil(ev);
        mParser_ColumnCoil.CloseCoil(ev);
        mParser_StringCoil.CloseCoil(ev);
        nsIMdbHeap_SlotStrongHeap((nsIMdbHeap*) 0, ev, &mParser_Heap);
        morkStream::SlotStrongStream((morkStream*) 0, ev, &mParser_Stream);
        this->MarkShut();
      }
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkParser::NonGoodParserError(morkEnv* ev) 
{
  ev->NewError("non-morkNode");
}

 void
morkParser::NonUsableParserError(morkEnv* ev) 
{
  if ( this->IsNode() )
  {
    if ( this->IsOpenNode() )
    {
      if (  this->GoodParserTag() )
      {
         
      }
      else
        this->NonGoodParserError(ev);
    }
    else
      this->NonOpenNodeError(ev);
  }
  else
    this->NonNodeError(ev);
}


 void
morkParser::StartParse(morkEnv* ev)
{
  MORK_USED_1(ev);
  mParser_InCell = morkBool_kFalse;
  mParser_InMeta = morkBool_kFalse;
  mParser_InDict = morkBool_kFalse;
  mParser_InPortRow = morkBool_kFalse;
  
  mParser_RowMid.ClearMid();
  mParser_TableMid.ClearMid();
  mParser_CellMid.ClearMid();
  
  mParser_GroupId = 0;
  mParser_InPort = morkBool_kTrue;

  mParser_GroupSpan.ClearSpan();
  mParser_DictSpan.ClearSpan();
  mParser_AliasSpan.ClearSpan();
  mParser_MetaSpan.ClearSpan();
  mParser_TableSpan.ClearSpan();
  mParser_RowSpan.ClearSpan();
  mParser_CellSpan.ClearSpan();
  mParser_ColumnSpan.ClearSpan();
  mParser_SlotSpan.ClearSpan();

   mParser_PortSpan.ClearSpan();
}

 void
morkParser::StopParse(morkEnv* ev)
{
  if ( mParser_InCell )
  {
    mParser_InCell = morkBool_kFalse;
    mParser_CellSpan.SetEndWithEnd(mParser_PortSpan);
    this->OnCellEnd(ev, mParser_CellSpan);
  }
  if ( mParser_InMeta )
  {
    mParser_InMeta = morkBool_kFalse;
    mParser_MetaSpan.SetEndWithEnd(mParser_PortSpan);
    this->OnMetaEnd(ev, mParser_MetaSpan);
  }
  if ( mParser_InDict )
  {
    mParser_InDict = morkBool_kFalse;
    mParser_DictSpan.SetEndWithEnd(mParser_PortSpan);
    this->OnDictEnd(ev, mParser_DictSpan);
  }
  if ( mParser_InPortRow )
  {
    mParser_InPortRow = morkBool_kFalse;
    mParser_RowSpan.SetEndWithEnd(mParser_PortSpan);
    this->OnPortRowEnd(ev, mParser_RowSpan);
  }
  if ( mParser_InRow )
  {
    mParser_InRow = morkBool_kFalse;
    mParser_RowMid.ClearMid();
    mParser_RowSpan.SetEndWithEnd(mParser_PortSpan);
    this->OnRowEnd(ev, mParser_RowSpan);
  }
  if ( mParser_InTable )
  {
    mParser_InTable = morkBool_kFalse;
    mParser_TableMid.ClearMid();
    mParser_TableSpan.SetEndWithEnd(mParser_PortSpan);
    this->OnTableEnd(ev, mParser_TableSpan);
  }
  if ( mParser_GroupId )
  {
    mParser_GroupId = 0;
    mParser_GroupSpan.SetEndWithEnd(mParser_PortSpan);
    this->OnGroupAbortEnd(ev, mParser_GroupSpan);
  }
  if ( mParser_InPort )
  {
    mParser_InPort = morkBool_kFalse;
    this->OnPortEnd(ev, mParser_PortSpan);
  }
}

int morkParser::eat_comment(morkEnv* ev) 
{
  morkStream* s = mParser_Stream;
  
  
  
  register int c = s->Getc(ev);
  if ( c == '/' ) 
  {
    while ( (c = s->Getc(ev)) != EOF && c != 0xA && c != 0xD )
      ;
      
    if ( c == 0xA || c == 0xD )
      c = this->eat_line_break(ev, c);
  }
  else if ( c == '*' ) 
  {
    int depth = 1; 
    
    while ( depth > 0 && c != EOF ) 
    {
      while ( (c = s->Getc(ev)) != EOF && c != '/' && c != '*' )
      {
        if ( c == 0xA || c == 0xD ) 
        {
          c = this->eat_line_break(ev, c);
          if ( c == '/' || c == '*' )
            break; 
        }
      }
        
      if ( c == '*' ) 
      {
        if ( (c = s->Getc(ev)) == '/' ) 
        {
          --depth; 
          if ( !depth ) 
            c = s->Getc(ev); 
        }
        else if ( c != EOF ) 
          s->Ungetc(c); 
      }
      else if ( c == '/' ) 
      {
        if ( (c = s->Getc(ev)) == '*' ) 
          ++depth; 
        else if ( c != EOF ) 
          s->Ungetc(c); 
      }
        
      if ( ev->Bad() )
        c = EOF;
    }
    if ( c == EOF && depth > 0 )
      ev->NewWarning("EOF before end of comment");
  }
  else
    ev->NewWarning("expected / or *");
  
  return c;
}

int morkParser::eat_line_break(morkEnv* ev, int inLast)
{
  morkStream* s = mParser_Stream;
  register int c = s->Getc(ev); 
  this->CountLineBreak();
  if ( c == 0xA || c == 0xD ) 
  {
    if ( c != inLast ) 
      c = s->Getc(ev); 
  }
  return c;
}

int morkParser::eat_line_continue(morkEnv* ev) 
{
  morkStream* s = mParser_Stream;
  register int c = s->Getc(ev);
  if ( c == 0xA || c == 0xD ) 
  {
    c = this->eat_line_break(ev, c);
  }
  else
    ev->NewWarning("expected linebreak");
  
  return c;
}

int morkParser::NextChar(morkEnv* ev) 
{
  morkStream* s = mParser_Stream;
  register int c = s->Getc(ev);
  while ( c > 0 && ev->Good() )
  {
    if ( c == '/' )
      c = this->eat_comment(ev);
    else if ( c == 0xA || c == 0xD )
      c = this->eat_line_break(ev, c);
    else if ( c == '\\' )
      c = this->eat_line_continue(ev);
    else if ( morkCh_IsWhite(c) )
      c = s->Getc(ev);
    else  
      break; 
  }
  if ( ev->Bad() )
  {
    mParser_State = morkParser_kBrokenState;
    mParser_DoMore = morkBool_kFalse;
    mParser_IsDone = morkBool_kTrue;
    mParser_IsBroken = morkBool_kTrue;
    c = EOF;
  }
  else if ( c == EOF )
  {
    mParser_DoMore = morkBool_kFalse;
    mParser_IsDone = morkBool_kTrue;
  }
  return c;
}

void
morkParser::OnCellState(morkEnv* ev)
{
  ev->StubMethodOnlyError();
}

void
morkParser::OnMetaState(morkEnv* ev)
{
  ev->StubMethodOnlyError();
}

void
morkParser::OnRowState(morkEnv* ev)
{
  ev->StubMethodOnlyError();
}

void
morkParser::OnTableState(morkEnv* ev)
{
  ev->StubMethodOnlyError();
}

void
morkParser::OnDictState(morkEnv* ev)
{
  ev->StubMethodOnlyError();
}

morkBuf* morkParser::ReadName(morkEnv* ev, register int c)
{
  morkBuf* outBuf = 0;
  
  if ( !morkCh_IsName(c) )
    ev->NewError("not a name char");

  morkCoil* coil = &mParser_ColumnCoil;
  coil->ClearBufFill();

  morkSpool* spool = &mParser_ColumnSpool;
  spool->Seek(ev,  0);
  
  if ( ev->Good() )
  {
    spool->Putc(ev, c);
    
    morkStream* s = mParser_Stream;
    while ( (c = s->Getc(ev)) != EOF && morkCh_IsMore(c) && ev->Good() )
      spool->Putc(ev, c);
      
    if ( ev->Good() )
    {
      if ( c != EOF )
      {
        s->Ungetc(c);
        spool->FlushSink(ev); 
      }
      else
        this->UnexpectedEofError(ev);
        
      if ( ev->Good() )
        outBuf = coil;
    }
  }  
  return outBuf;
}

mork_bool
morkParser::ReadMid(morkEnv* ev, morkMid* outMid)
{
  outMid->ClearMid();
  
  morkStream* s = mParser_Stream;
  int next;
  outMid->mMid_Oid.mOid_Id = this->ReadHex(ev, &next);
  register int c = next;
  if ( c == ':' )
  {
    if ( (c = s->Getc(ev)) != EOF && ev->Good() )
    {
      if ( c == '^' )
      {
        outMid->mMid_Oid.mOid_Scope = this->ReadHex(ev, &next);
        if ( ev->Good() )
          s->Ungetc(next);
      }
      else if ( morkCh_IsName(c) )
      {
        outMid->mMid_Buf = this->ReadName(ev, c); 
      }
      else
        ev->NewError("expected name or hex after ':' following ID");
    }
    
    if ( c == EOF && ev->Good() )
      this->UnexpectedEofError(ev);
  }
  else
    s->Ungetc(c);
  
  return ev->Good();
}

void
morkParser::ReadCell(morkEnv* ev)
{
  mParser_CellMid.ClearMid();
  
  morkMid* cellMid = 0; 
  morkBuf* cellBuf = 0; 

  morkStream* s = mParser_Stream;
  register int c;
  if ( (c = s->Getc(ev)) != EOF && ev->Good() )
  {
    
    if ( c == '^' )
    {
      cellMid = &mParser_CellMid;
      this->ReadMid(ev, cellMid);
      
      
    }
    else
    {
      if (mParser_InMeta && c == morkStore_kFormColumn)
      {
        ReadCellForm(ev, c);
        return;
      }
      else
        cellBuf = this->ReadName(ev, c); 
    }
    if ( ev->Good() )
    {
      

      mParser_InCell = morkBool_kTrue;
      this->OnNewCell(ev, *mParser_CellSpan.AsPlace(),
        cellMid, cellBuf); 

      mParser_CellChange = morkChange_kNil;
      if ( (c = this->NextChar(ev)) != EOF && ev->Good() )
      {
        
        if ( c == '=' )
        {
          morkBuf* buf = this->ReadValue(ev);
          if ( buf )
          {
            
            this->OnValue(ev, mParser_SlotSpan, *buf);
          }
        }
        else if ( c == '^' )
        {
          if ( this->ReadMid(ev, &mParser_Mid) )
          {
            
            if ( (c = this->NextChar(ev)) != EOF && ev->Good() )
            {
              if ( c != ')' )
                ev->NewError("expected ')' after cell ^ID value");
            }
            else if ( c == EOF )
              this->UnexpectedEofError(ev);
            
            if ( ev->Good() )
              this->OnValueMid(ev, mParser_SlotSpan, mParser_Mid);
          }
        }
        else if ( c == 'r' || c == 't' || c == '"' || c == '\'' )
        {
          ev->NewError("cell syntax not yet supported");
        }
        else
        {
          ev->NewError("unknown cell syntax");
        }
      }
      
      
      mParser_InCell = morkBool_kFalse;
      this->OnCellEnd(ev, mParser_CellSpan);
    }
  }
  mParser_CellChange = morkChange_kNil;
  
  if ( c == EOF && ev->Good() )
    this->UnexpectedEofError(ev);
}

void morkParser::ReadRowPos(morkEnv* ev)
{
  int c; 
  mork_pos rowPos = this->ReadHex(ev, &c);
  
  if ( ev->Good() && c != EOF ) 
    mParser_Stream->Ungetc(c);

  this->OnRowPos(ev, rowPos);
}

void morkParser::ReadRow(morkEnv* ev, int c)




{
  if ( ev->Good() )
  {
    
    if ( mParser_Change )
      mParser_RowChange = mParser_Change;

    mork_bool cutAllRowCols = morkBool_kFalse;

    if ( c == '[' )
    {
      if ( ( c = this->NextChar(ev) ) == '-' )
        cutAllRowCols = morkBool_kTrue;
      else if ( ev->Good() && c != EOF )
        mParser_Stream->Ungetc(c);

      if ( this->ReadMid(ev, &mParser_RowMid) )
      {
        mParser_InRow = morkBool_kTrue;
        this->OnNewRow(ev, *mParser_RowSpan.AsPlace(),
          mParser_RowMid, cutAllRowCols);

        mParser_Change = mParser_RowChange = morkChange_kNil;

        while ( (c = this->NextChar(ev)) != EOF && ev->Good() && c != ']' )
        {
          switch ( c )
          {
            case '(': 
              this->ReadCell(ev);
              break;
              
            case '[': 
              this->ReadMeta(ev, ']');
              break;
            
            
            
            
              
            case '-': 
              
              this->OnMinusCell(ev);
              break;
              
            
            
            
              
            default:
              ev->NewWarning("unexpected byte in row");
              break;
          } 
        } 
        
        if ( ev->Good() )
        {
          if ( (c = this->NextChar(ev)) == '!' )
            this->ReadRowPos(ev);
          else if ( c != EOF && ev->Good() )
            mParser_Stream->Ungetc(c);
        }
        
        
        mParser_InRow = morkBool_kFalse;
        this->OnRowEnd(ev, mParser_RowSpan);

      } 
    } 
    
    else 
    {
      morkStream* s = mParser_Stream;
      s->Ungetc(c);
      if ( this->ReadMid(ev, &mParser_RowMid) )
      {
        mParser_InRow = morkBool_kTrue;
        this->OnNewRow(ev, *mParser_RowSpan.AsPlace(),
          mParser_RowMid, cutAllRowCols);

        mParser_Change = mParser_RowChange = morkChange_kNil;
        
        if ( ev->Good() )
        {
          if ( (c = this->NextChar(ev)) == '!' )
            this->ReadRowPos(ev);
          else if ( c != EOF && ev->Good() )
            s->Ungetc(c);
        }

        
        mParser_InRow = morkBool_kFalse;
        this->OnRowEnd(ev, mParser_RowSpan);
      }
    }
  }
  
  if ( ev->Bad() )
    mParser_State = morkParser_kBrokenState;
  else if ( c == EOF )
    mParser_State = morkParser_kDoneState;
}

void morkParser::ReadTable(morkEnv* ev)



{
  

  if ( mParser_Change )
    mParser_TableChange = mParser_Change;

  mork_bool cutAllTableRows = morkBool_kFalse;
  
  int c = this->NextChar(ev);
  if ( c == '-' )
    cutAllTableRows = morkBool_kTrue;
  else if ( ev->Good() && c != EOF )
    mParser_Stream->Ungetc(c);
  
  if ( ev->Good() && this->ReadMid(ev, &mParser_TableMid) )
  {
    mParser_InTable = morkBool_kTrue;
    this->OnNewTable(ev, *mParser_TableSpan.AsPlace(),  
      mParser_TableMid, cutAllTableRows);

    mParser_Change = mParser_TableChange = morkChange_kNil;

    while ( (c = this->NextChar(ev)) != EOF && ev->Good() && c != '}' )
    {
      if ( morkCh_IsHex(c) )
      {
        this->ReadRow(ev, c);
      }
      else
      {
        switch ( c )
        {
          case '[': 
            this->ReadRow(ev, '[');
            break;
            
          case '{': 
            this->ReadMeta(ev, '}');
            break;
          
          
          
          
            
          case '-': 
            
            this->OnMinusRow(ev);
            break;
            
          
          
          
            
          default:
            ev->NewWarning("unexpected byte in table");
            break;
        }
      }
    }

    
    mParser_InTable = morkBool_kFalse;
    this->OnTableEnd(ev, mParser_TableSpan);

    if ( ev->Bad() )
      mParser_State = morkParser_kBrokenState;
    else if ( c == EOF )
      mParser_State = morkParser_kDoneState;
  }
}

mork_id morkParser::ReadHex(morkEnv* ev, int* outNextChar)


{
  mork_id hex = 0;

  morkStream* s = mParser_Stream;
  register int c = this->NextChar(ev);
    
  if ( ev->Good() )
  {
    if ( c != EOF )
    {
      if ( morkCh_IsHex(c) )
      {
        do
        {
          if ( morkCh_IsDigit(c) ) 
            c -= '0';
          else if ( morkCh_IsUpper(c) ) 
            c -= ('A' - 10) ; 
          else 
            c -= ('a' - 10) ; 

          hex = (hex << 4) + c;
        }
        while ( (c = s->Getc(ev)) != EOF && ev->Good() && morkCh_IsHex(c) );
      }
      else
        this->ExpectedHexDigitError(ev, c);
    }
  }
  if ( c == EOF )
    this->EofInsteadOfHexError(ev);
    
  *outNextChar = c;
  return hex;
}

 void
morkParser::EofInsteadOfHexError(morkEnv* ev)
{
  ev->NewWarning("eof instead of hex");
}

 void
morkParser::ExpectedHexDigitError(morkEnv* ev, int c)
{
  MORK_USED_1(c);
  ev->NewWarning("expected hex digit");
}

 void
morkParser::ExpectedEqualError(morkEnv* ev)
{
  ev->NewWarning("expected '='");
}

 void
morkParser::UnexpectedEofError(morkEnv* ev)
{
  ev->NewWarning("unexpected eof");
}


morkBuf* morkParser::ReadValue(morkEnv* ev)
{
  morkBuf* outBuf = 0;

  morkCoil* coil = &mParser_ValueCoil;
  coil->ClearBufFill();

  morkSpool* spool = &mParser_ValueSpool;
  spool->Seek(ev,  0);
  
  if ( ev->Good() )
  {
    morkStream* s = mParser_Stream;
    register int c;
    while ( (c = s->Getc(ev)) != EOF && c != ')' && ev->Good() )
    {
      if ( c == '\\' ) 
      {
        if ( (c = s->Getc(ev)) == 0xA || c == 0xD ) 
        {
          c = this->eat_line_break(ev, c);
          if ( c == ')' || c == '\\' || c == '$' )
          {
            s->Ungetc(c); 
            continue; 
          }
        }
        if ( c == EOF || ev->Bad() )
          break; 
      }
      else if ( c == '$' ) 
      {
        if ( (c = s->Getc(ev)) != EOF && ev->Good() )
        {
          mork_ch first = (mork_ch) c; 
          if ( (c = s->Getc(ev)) != EOF && ev->Good() )
          {
            mork_ch second = (mork_ch) c; 
            c = ev->HexToByte(first, second);
          }
          else
            break; 
        }
        else
          break; 
      }
      spool->Putc(ev, c);
    }
      
    if ( ev->Good() )
    {
      if ( c != EOF )
        spool->FlushSink(ev); 
      else
        this->UnexpectedEofError(ev);
        
      if ( ev->Good() )
        outBuf = coil;
    }
  }
  return outBuf; 
}

void morkParser::ReadDictForm(morkEnv *ev)
{
  int nextChar;
  nextChar = this->NextChar(ev);
  if (nextChar == '(')
  {
    nextChar = this->NextChar(ev);
    if (nextChar == morkStore_kFormColumn)
    {
      int dictForm;

      nextChar = this->NextChar(ev);
      if (nextChar == '=')
      {
        dictForm = this->NextChar(ev);
        nextChar = this->NextChar(ev);
      }
      else if (nextChar == '^')
      {
        dictForm = this->ReadHex(ev, &nextChar);
      }
      else
      {
        ev->NewWarning("unexpected byte in dict form");
        return;
      }
      mParser_ValueCoil.mText_Form = dictForm;
      if (nextChar == ')')
      {
        nextChar = this->NextChar(ev);
        if (nextChar == '>')
          return;
      }
    }
  }
  ev->NewWarning("unexpected byte in dict form");
}

void morkParser::ReadCellForm(morkEnv *ev, int c)
{
  MORK_ASSERT (c == morkStore_kFormColumn);
  int nextChar;
  nextChar = this->NextChar(ev);
  int cellForm;

  if (nextChar == '=')
  {
    cellForm = this->NextChar(ev);
    nextChar = this->NextChar(ev);
  }
  else if (nextChar == '^')
  {
    cellForm = this->ReadHex(ev, &nextChar);
  }
  else
  {
    ev->NewWarning("unexpected byte in cell form");
    return;
  }
  
  
  if (nextChar == ')')
  {
    OnCellForm(ev, cellForm);
    return;
  }
  ev->NewWarning("unexpected byte in cell form");
}

void morkParser::ReadAlias(morkEnv* ev)


{
  

  int nextChar;
  mork_id hex = this->ReadHex(ev, &nextChar);
  register int c = nextChar;

  mParser_Mid.ClearMid();
  mParser_Mid.mMid_Oid.mOid_Id = hex;

  if ( morkCh_IsWhite(c) && ev->Good() )
    c = this->NextChar(ev);

  if ( ev->Good() )
  {
    if ( c == '<')
    {
      ReadDictForm(ev);
      if (ev->Good())
        c = this->NextChar(ev);
    }
    if ( c == '=' )
    {
      mParser_Mid.mMid_Buf = this->ReadValue(ev);
      if ( mParser_Mid.mMid_Buf )
      {
        
        this->OnAlias(ev, mParser_AliasSpan, mParser_Mid);
        
        mParser_ValueCoil.mText_Form = 0;

      }
    }
    else
      this->ExpectedEqualError(ev);
  }
}

void morkParser::ReadMeta(morkEnv* ev, int inEndMeta)



{
  
  mParser_InMeta = morkBool_kTrue;
  this->OnNewMeta(ev, *mParser_MetaSpan.AsPlace());

  mork_bool more = morkBool_kTrue; 
  int c;
  while ( more && (c = this->NextChar(ev)) != EOF && ev->Good() )
  {
    switch ( c )
    {
      case '(': 
        this->ReadCell(ev);
        break;
        
      case '>': 
        if ( inEndMeta == '>' )
          more = morkBool_kFalse; 
        else
          this->UnexpectedByteInMetaWarning(ev);
        break;
        
      case '}': 
        if ( inEndMeta == '}' )
          more = morkBool_kFalse; 
        else
          this->UnexpectedByteInMetaWarning(ev);
        break;
        
      case ']': 
        if ( inEndMeta == ']' )
          more = morkBool_kFalse; 
        else
          this->UnexpectedByteInMetaWarning(ev);
        break;
        
      case '[': 
        if ( mParser_InTable )
          this->ReadRow(ev, '['); 
        else
          this->UnexpectedByteInMetaWarning(ev);
        break;
        
      default:
        if ( mParser_InTable && morkCh_IsHex(c) )
          this->ReadRow(ev, c);
        else
          this->UnexpectedByteInMetaWarning(ev);
        break;
    }
  }

  
  mParser_InMeta = morkBool_kFalse;
  this->OnMetaEnd(ev, mParser_MetaSpan);
}

 void
morkParser::UnexpectedByteInMetaWarning(morkEnv* ev)
{
  ev->NewWarning("unexpected byte in meta");
}

 void
morkParser::NonParserTypeError(morkEnv* ev)
{
  ev->NewError("non morkParser");
}

mork_bool morkParser::MatchPattern(morkEnv* ev, const char* inPattern)
{
  
  const char* pattern = inPattern; 
  morkStream* s = mParser_Stream;
  register int c;
  while ( *pattern && ev->Good() )
  {
    char byte = *pattern++;
    if ( (c = s->Getc(ev)) != byte )
    {
      ev->NewError("byte not in expected pattern");
    }
  }
  return ev->Good();
}

mork_bool morkParser::FindGroupEnd(morkEnv* ev)
{
  mork_bool foundEnd = morkBool_kFalse;
  
  
  
  
  morkStream* s = mParser_Stream;
  register int c;
  
  while ( (c = s->Getc(ev)) != EOF && ev->Good() && !foundEnd )
  {
    if ( c == '@' ) 
    {
      
      if ( (c = s->Getc(ev)) == '$' ) 
      {
        if ( (c = s->Getc(ev)) == '$' ) 
        {
          if ( (c = s->Getc(ev)) == '}' )
          {
            foundEnd = this->ReadEndGroupId(ev);
            

          }
          else
            ev->NewError("expected '}' after @$$");
        }
      }
      if ( !foundEnd && c == '@' )
        s->Ungetc(c);
    }
  }

  return foundEnd && ev->Good();
}

void morkParser::ReadGroup(morkEnv* mev)
{
  nsIMdbEnv *ev = mev->AsMdbEnv();
  int next = 0;
  mParser_GroupId = this->ReadHex(mev, &next);
  if ( next == '{' )
  {
    morkStream* s = mParser_Stream;
     register int c;
    if ( (c = s->Getc(mev)) == '@' )
    {
    	
      this->StartSpanOnThisByte(mev, &mParser_GroupSpan);
      mork_pos startPos = mParser_GroupSpan.mSpan_Start.mPlace_Pos;

      
      
      
      
      
      if ( this->FindGroupEnd(mev) )
      {
        mork_pos outPos;
        s->Seek(ev, startPos, &outPos);
        if ( mev->Good() )
        {
          this->OnNewGroup(mev, mParser_GroupSpan.mSpan_Start,
            mParser_GroupId);
          
          this->ReadContent(mev,  morkBool_kTrue);

          this->OnGroupCommitEnd(mev, mParser_GroupSpan);
        }
      }
    }
    else
      mev->NewError("expected '@' after @$${id{");
  }
  else
    mev->NewError("expected '{' after @$$id");
    
}

mork_bool morkParser::ReadAt(morkEnv* ev, mork_bool inInsideGroup)








{
  if ( this->MatchPattern(ev, "$$") )
  {
    morkStream* s = mParser_Stream;
     register int c;
    if ( ((c = s->Getc(ev)) == '{' || c == '}') && ev->Good() )
     {
       if ( c == '{' ) 
       {
         if ( !inInsideGroup )
           this->ReadGroup(ev);
         else
           ev->NewError("nested @$${ inside another group");
       }
       else 
       {
         if ( inInsideGroup )
         {
          this->ReadEndGroupId(ev);
          mParser_GroupId = 0;
         }
         else
           ev->NewError("unmatched @$$} outside any group");
       }
     }
     else
       ev->NewError("expected '{' or '}' after @$$");
  }
  return ev->Good();
}

mork_bool morkParser::ReadEndGroupId(morkEnv* ev)
{
  mork_bool outSawGroupId = morkBool_kFalse;
  morkStream* s = mParser_Stream;
  register int c;
  if ( (c = s->Getc(ev)) != EOF && ev->Good() )
  {
    if ( c == '~' ) 
    {
      this->MatchPattern(ev, "~}@"); 
    }
    else 
    {
      s->Ungetc(c);
      int next = 0;
      mork_gid endGroupId = this->ReadHex(ev, &next);
      if ( ev->Good() )
      {
        if ( endGroupId == mParser_GroupId ) 
        {
          if ( next == '}' ) 
          {
            if ( (c = s->Getc(ev)) == '@' ) 
            {
              
              outSawGroupId = morkBool_kTrue;
            }
            else
              ev->NewError("expected '@' after @$$}id}");
          }
          else
            ev->NewError("expected '}' after @$$}id");
        }
        else
          ev->NewError("end group id mismatch");
      }
    }
  }
  return ( outSawGroupId && ev->Good() );
}


void morkParser::ReadDict(morkEnv* ev)




{
  mParser_Change = morkChange_kNil;
  mParser_AtomChange = morkChange_kNil;
  
  
  mParser_InDict = morkBool_kTrue;
  this->OnNewDict(ev, *mParser_DictSpan.AsPlace());
  
  int c;
  while ( (c = this->NextChar(ev)) != EOF && ev->Good() && c != '>' )
  {
    switch ( c )
    {
      case '(': 
        this->ReadAlias(ev);
        break;
        
      case '<': 
        this->ReadMeta(ev, '>');
        break;
        
      default:
        ev->NewWarning("unexpected byte in dict");
        break;
    }
  }

  
  mParser_InDict = morkBool_kFalse;
  this->OnDictEnd(ev, mParser_DictSpan);
  
  if ( ev->Bad() )
    mParser_State = morkParser_kBrokenState;
  else if ( c == EOF )
    mParser_State = morkParser_kDoneState;
}

void morkParser::EndSpanOnThisByte(morkEnv* mev, morkSpan* ioSpan)
{
  mork_pos here;
  nsIMdbEnv *ev = mev->AsMdbEnv();
  nsresult rv = mParser_Stream->Tell(ev, &here);
  if (NS_SUCCEEDED(rv) && mev->Good() )
  {
    this->SetHerePos(here);
    ioSpan->SetEndWithEnd(mParser_PortSpan);
  }
}

void morkParser::EndSpanOnLastByte(morkEnv* mev, morkSpan* ioSpan)
{
  mork_pos here;
  nsIMdbEnv *ev = mev->AsMdbEnv();
  nsresult rv= mParser_Stream->Tell(ev, &here);
  if ( NS_SUCCEEDED(rv) && mev->Good() )
  {
    if ( here > 0 )
      --here;
    else
      here = 0;

    this->SetHerePos(here);
    ioSpan->SetEndWithEnd(mParser_PortSpan);
  }
}

void morkParser::StartSpanOnLastByte(morkEnv* mev, morkSpan* ioSpan)
{
  mork_pos here;
  nsIMdbEnv *ev = mev->AsMdbEnv();
  nsresult rv = mParser_Stream->Tell(ev, &here);
  if ( NS_SUCCEEDED(rv) && mev->Good() )
  {
    if ( here > 0 )
      --here;
    else
      here = 0;

    this->SetHerePos(here);
    ioSpan->SetStartWithEnd(mParser_PortSpan);
    ioSpan->SetEndWithEnd(mParser_PortSpan);
  }
}

void morkParser::StartSpanOnThisByte(morkEnv* mev, morkSpan* ioSpan)
{
  mork_pos here;
  nsIMdbEnv *ev = mev->AsMdbEnv();
  nsresult rv = mParser_Stream->Tell(ev, &here);
  if ( NS_SUCCEEDED(rv) && mev->Good() )
  {
    this->SetHerePos(here);
    ioSpan->SetStartWithEnd(mParser_PortSpan);
    ioSpan->SetEndWithEnd(mParser_PortSpan);
  }
}

mork_bool
morkParser::ReadContent(morkEnv* ev, mork_bool inInsideGroup)
{
  int c;
  while ( (c = this->NextChar(ev)) != EOF && ev->Good() )
  {
    switch ( c )
    {
      case '[': 
        this->ReadRow(ev, '[');
        break;
        
      case '{': 
        this->ReadTable(ev);
        break;
        
      case '<': 
        this->ReadDict(ev);
        break;
        
      case '@': 
        return this->ReadAt(ev, inInsideGroup);
        
        
      
      
      
        
      
      
      
        
      
      
      
        
      default:
        ev->NewWarning("unexpected byte in ReadContent()");
        break;
    }
  }
  if ( ev->Bad() )
    mParser_State = morkParser_kBrokenState;
  else if ( c == EOF )
    mParser_State = morkParser_kDoneState;
    
  return ( ev->Good() && c != EOF );
}

void
morkParser::OnPortState(morkEnv* ev)
{
  mParser_InPort = morkBool_kTrue;
  this->OnNewPort(ev, *mParser_PortSpan.AsPlace());

  while ( this->ReadContent(ev,  morkBool_kFalse) )
    ;
  
  mParser_InPort = morkBool_kFalse;
  this->OnPortEnd(ev, mParser_PortSpan);
  
  if ( ev->Bad() )
    mParser_State = morkParser_kBrokenState;
}

void
morkParser::OnStartState(morkEnv* mev)
{
  morkStream* s = mParser_Stream;
  nsIMdbEnv *ev = mev->AsMdbEnv();
  if ( s && s->IsNode() && s->IsOpenNode() )
  {
    mork_pos outPos;
    nsresult rv = s->Seek(ev, 0, &outPos);
    if (NS_SUCCEEDED(rv) && mev->Good() )
    {
      this->StartParse(mev);
      mParser_State = morkParser_kPortState;
    }
  }
  else
    mev->NilPointerError();

  if ( mev->Bad() )
    mParser_State = morkParser_kBrokenState;
}

 void
morkParser::ParseLoop(morkEnv* ev)
{
  mParser_Change = morkChange_kNil;
  mParser_DoMore = morkBool_kTrue;
            
  while ( mParser_DoMore )
  {
    switch ( mParser_State )
    {
      case morkParser_kCellState: 
        this->OnCellState(ev); break;
        
      case morkParser_kMetaState: 
        this->OnMetaState(ev); break;
        
      case morkParser_kRowState: 
        this->OnRowState(ev); break;
        
      case morkParser_kTableState: 
        this->OnTableState(ev); break;
        
      case morkParser_kDictState: 
        this->OnDictState(ev); break;
        
      case morkParser_kPortState: 
        this->OnPortState(ev); break;
        
      case morkParser_kStartState: 
        this->OnStartState(ev); break;
       
      case morkParser_kDoneState: 
        mParser_DoMore = morkBool_kFalse;
        mParser_IsDone = morkBool_kTrue;
        this->StopParse(ev);
        break;
      case morkParser_kBrokenState: 
        mParser_DoMore = morkBool_kFalse;
        mParser_IsBroken = morkBool_kTrue;
        this->StopParse(ev);
        break;
      default: 
        MORK_ASSERT(morkBool_kFalse);
        mParser_State = morkParser_kBrokenState;
        break;
    }
  }
}
    
 mdb_count
morkParser::ParseMore( 
    morkEnv* ev,          
    mork_pos* outPos,     
    mork_bool* outDone,   
    mork_bool* outBroken  
  )
{
  mdb_count outCount = 0;
  if ( this->IsNode() && this->GoodParserTag() && this->IsOpenNode() )
  {
    mork_pos startPos = this->HerePos();

    if ( !mParser_IsDone && !mParser_IsBroken )
      this->ParseLoop(ev);
  
    mork_pos endPos = this->HerePos();
    if ( outDone )
      *outDone = mParser_IsDone;
    if ( outBroken )
      *outBroken = mParser_IsBroken;
    if ( outPos )
      *outPos = endPos;
      
    if ( endPos > startPos )
      outCount = (mdb_count) (endPos - startPos);
  }
  else
  {
    this->NonUsableParserError(ev);
    if ( outDone )
      *outDone = morkBool_kTrue;
    if ( outBroken )
      *outBroken = morkBool_kTrue;
    if ( outPos )
      *outPos = 0;
  }
  return outCount;
}



