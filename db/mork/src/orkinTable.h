




































#ifndef _ORKINTABLE_
#define _ORKINTABLE_ 1

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKHANDLE_
#include "morkHandle.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif



#define morkMagic_kTable 0x5461626C /* ascii 'Tabl' */



class orkinTable : public morkHandle, public nsIMdbTable { 


public: 
  
  virtual ~orkinTable(); 
  
protected: 
  orkinTable(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkTable* ioObject); 
    
  

private: 
  orkinTable(const orkinTable& other);
  orkinTable& operator=(const orkinTable& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
  
public: 

  static orkinTable* MakeTable(morkEnv* ev, morkTable* ioObject);

public: 

  morkEnv* CanUseTable(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;

public: 
  mork_bool IsOrkinTable() const
  { return mHandle_Magic == morkMagic_kTable; }

  mork_bool IsOrkinTableHandle() const
  { return this->IsHandle() && this->IsOrkinTable(); }

  NS_DECL_ISUPPORTS


  
  NS_IMETHOD IsFrozenMdbObject(nsIMdbEnv* ev, mdb_bool* outIsReadonly);
  
  

  
  NS_IMETHOD GetMdbFactory(nsIMdbEnv* ev, nsIMdbFactory** acqFactory); 
  

  
  NS_IMETHOD GetWeakRefCount(nsIMdbEnv* ev, 
    mdb_count* outCount);  
  NS_IMETHOD GetStrongRefCount(nsIMdbEnv* ev, 
    mdb_count* outCount);

  NS_IMETHOD AddWeakRef(nsIMdbEnv* ev);
  NS_IMETHOD AddStrongRef(nsIMdbEnv* ev);

  NS_IMETHOD CutWeakRef(nsIMdbEnv* ev);
  NS_IMETHOD CutStrongRef(nsIMdbEnv* ev);
  
  NS_IMETHOD CloseMdbObject(nsIMdbEnv* ev); 
  NS_IMETHOD IsOpenMdbObject(nsIMdbEnv* ev, mdb_bool* outOpen);
  
  




  
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
  


};
 


#endif 
