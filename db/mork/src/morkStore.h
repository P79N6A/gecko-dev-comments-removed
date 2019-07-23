




































#ifndef _MORKSTORE_
#define _MORKSTORE_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif

#ifndef _MORKNODEMAP_
#include "morkNodeMap.h"
#endif

#ifndef _MORKPOOL_
#include "morkPool.h"
#endif

#ifndef _MORKZONE_
#include "morkZone.h"
#endif

#ifndef _MORKATOM_
#include "morkAtom.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKATOMSPACE_
#include "morkAtomSpace.h"
#endif



#define morkDerived_kPort 0x7054 /* ascii 'pT' */

#define morkDerived_kStore 0x7354 /* ascii 'sT' */





#define morkStore_kGroundColumnSpace 'c' /* for mStore_GroundColumnSpace*/
#define morkStore_kColumnSpaceScope ((mork_scope) 'c') /*kGroundColumnSpace*/
#define morkStore_kValueSpaceScope ((mork_scope) 'v')
#define morkStore_kStreamBufSize (8 * 1024) /* okay buffer size */

#define morkStore_kReservedColumnCount 0x20 /* for well-known columns */

#define morkStore_kNoneToken ((mork_token) 'n')
#define morkStore_kFormColumn ((mork_column) 'f')
#define morkStore_kAtomScopeColumn ((mork_column) 'a')
#define morkStore_kRowScopeColumn ((mork_column) 'r')
#define morkStore_kMetaScope ((mork_scope) 'm')
#define morkStore_kKindColumn ((mork_column) 'k')
#define morkStore_kStatusColumn ((mork_column) 's')



class morkStore :  public morkObject, public nsIMdbStore {

public: 

  NS_DECL_ISUPPORTS_INHERITED

  morkEnv*        mPort_Env;      
  morkFactory*    mPort_Factory;  
  nsIMdbHeap*     mPort_Heap;     
  

public: 
  
  void ClosePort(morkEnv* ev); 

public: 
  mork_bool IsPort() const
  { return IsNode() && mNode_Derived == morkDerived_kPort; }


public: 

  

  
  

  

  

  
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
  
  




  
  NS_IMETHOD GetIsPortReadonly(nsIMdbEnv* ev, mdb_bool* outBool);
  NS_IMETHOD GetIsStore(nsIMdbEnv* ev, mdb_bool* outBool);
  NS_IMETHOD GetIsStoreAndDirty(nsIMdbEnv* ev, mdb_bool* outBool);

  NS_IMETHOD GetUsagePolicy(nsIMdbEnv* ev, 
    mdbUsagePolicy* ioUsagePolicy);

  NS_IMETHOD SetUsagePolicy(nsIMdbEnv* ev, 
    const mdbUsagePolicy* inUsagePolicy);
  

  
  NS_IMETHOD IdleMemoryPurge( 
    nsIMdbEnv* ev, 
    mdb_size* outEstimatedBytesFreed); 

  NS_IMETHOD SessionMemoryPurge( 
    nsIMdbEnv* ev, 
    mdb_size inDesiredBytesFreed, 
    mdb_size* outEstimatedBytesFreed); 

  NS_IMETHOD PanicMemoryPurge( 
    nsIMdbEnv* ev, 
    mdb_size* outEstimatedBytesFreed); 
  

  
  NS_IMETHOD GetPortFilePath(
    nsIMdbEnv* ev, 
    mdbYarn* outFilePath, 
    mdbYarn* outFormatVersion); 

  NS_IMETHOD GetPortFile(
    nsIMdbEnv* ev, 
    nsIMdbFile** acqFile); 
  

  
  NS_IMETHOD BestExportFormat( 
    nsIMdbEnv* ev, 
    mdbYarn* outFormatVersion); 

  NS_IMETHOD
  CanExportToFormat( 
    nsIMdbEnv* ev, 
    const char* inFormatVersion, 
    mdb_bool* outCanExport); 

  NS_IMETHOD ExportToFormat( 
    nsIMdbEnv* ev, 
    
    nsIMdbFile* ioFile, 
    const char* inFormatVersion, 
    nsIMdbThumb** acqThumb); 
  
  

  

  
  NS_IMETHOD TokenToString( 
    nsIMdbEnv* ev, 
    mdb_token inToken, 
    mdbYarn* outTokenName); 
  
  NS_IMETHOD StringToToken( 
    nsIMdbEnv* ev, 
    const char* inTokenName, 
    mdb_token* outToken); 
    
  
  
  
  

  NS_IMETHOD QueryToken( 
    nsIMdbEnv* ev, 
    const char* inTokenName, 
    mdb_token* outToken); 
  
  
  
  

  

  
  NS_IMETHOD HasRow( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_bool* outHasRow); 

  NS_IMETHOD GetRowRefCount( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_count* outRefCount); 
    
  NS_IMETHOD GetRow( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    nsIMdbRow** acqRow); 

  NS_IMETHOD FindRow(nsIMdbEnv* ev, 
    mdb_scope inRowScope,   
    mdb_column inColumn,   
    const mdbYarn* inTargetCellValue, 
    mdbOid* outRowOid, 
    nsIMdbRow** acqRow); 
                         
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  NS_IMETHOD HasTable( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_bool* outHasTable); 
    
  NS_IMETHOD GetTable( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    nsIMdbTable** acqTable); 
  
  NS_IMETHOD HasTableKind( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope, 
    mdb_kind inTableKind, 
    mdb_count* outTableCount, 
    mdb_bool* outSupportsTable); 
        
  NS_IMETHOD GetTableKind( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope,      
    mdb_kind inTableKind,      
    mdb_count* outTableCount, 
    mdb_bool* outMustBeUnique, 
    nsIMdbTable** acqTable);       
    
  NS_IMETHOD
  GetPortTableCursor( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope, 
    mdb_kind inTableKind, 
    nsIMdbPortTableCursor** acqCursor); 
  


  

  NS_IMETHOD ShouldCompress( 
    nsIMdbEnv* ev, 
    mdb_percent inPercentWaste, 
    mdb_percent* outActualWaste, 
    mdb_bool* outShould); 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  





  
  NS_IMETHOD NewTable( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope,    
    mdb_kind inTableKind,    
    mdb_bool inMustBeUnique, 
    const mdbOid* inOptionalMetaRowOid, 
    nsIMdbTable** acqTable);     
    
  NS_IMETHOD NewTableWithOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,   
    mdb_kind inTableKind,    
    mdb_bool inMustBeUnique, 
    const mdbOid* inOptionalMetaRowOid, 
    nsIMdbTable** acqTable);     
  

  
  NS_IMETHOD RowScopeHasAssignedIds(nsIMdbEnv* ev,
    mdb_scope inRowScope,   
    mdb_bool* outCallerAssigned, 
    mdb_bool* outStoreAssigned); 

  NS_IMETHOD SetCallerAssignedIds(nsIMdbEnv* ev,
    mdb_scope inRowScope,   
    mdb_bool* outCallerAssigned, 
    mdb_bool* outStoreAssigned); 

  NS_IMETHOD SetStoreAssignedIds(nsIMdbEnv* ev,
    mdb_scope inRowScope,   
    mdb_bool* outCallerAssigned, 
    mdb_bool* outStoreAssigned); 
  

  
  NS_IMETHOD NewRowWithOid(nsIMdbEnv* ev, 
    const mdbOid* inOid,   
    nsIMdbRow** acqRow); 

  NS_IMETHOD NewRow(nsIMdbEnv* ev, 
    mdb_scope inRowScope,   
    nsIMdbRow** acqRow); 
  
  

  

  
  NS_IMETHOD ImportContent( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope, 
    nsIMdbPort* ioPort, 
    nsIMdbThumb** acqThumb); 
  
  

  NS_IMETHOD ImportFile( 
    nsIMdbEnv* ev, 
    nsIMdbFile* ioFile, 
    nsIMdbThumb** acqThumb); 
  
  
  

  
  NS_IMETHOD
  ShareAtomColumnsHint( 
    nsIMdbEnv* ev, 
    mdb_scope inScopeHint, 
    const mdbColumnSet* inColumnSet); 

  NS_IMETHOD
  AvoidAtomColumnsHint( 
    nsIMdbEnv* ev, 
    const mdbColumnSet* inColumnSet); 
  

  
  NS_IMETHOD SmallCommit( 
    nsIMdbEnv* ev); 
  
  NS_IMETHOD LargeCommit( 
    nsIMdbEnv* ev, 
    nsIMdbThumb** acqThumb); 
  
  
  
  

  NS_IMETHOD SessionCommit( 
    nsIMdbEnv* ev, 
    nsIMdbThumb** acqThumb); 
  
  
  
  

  NS_IMETHOD
  CompressCommit( 
    nsIMdbEnv* ev, 
    nsIMdbThumb** acqThumb); 
  
  
  
  
  
  



public: 
  static void SlotWeakPort(morkPort* me,
    morkEnv* ev, morkPort** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongPort(morkPort* me,
    morkEnv* ev, morkPort** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }

  
  
  
  
  
  
  
  
  
 
  
  
  

public: 


  morkAtomSpace*   mStore_OidAtomSpace;   
  morkAtomSpace*   mStore_GroundAtomSpace; 
  morkAtomSpace*   mStore_GroundColumnSpace; 

  nsIMdbFile*      mStore_File; 
  morkStream*      mStore_InStream; 
  morkBuilder*     mStore_Builder; 

  morkStream*      mStore_OutStream; 
  
  morkRowSpaceMap  mStore_RowSpaces;  
  morkAtomSpaceMap mStore_AtomSpaces; 
  
  morkZone         mStore_Zone;
  
  morkPool         mStore_Pool;

  
  
  
  morkFarBookAtom  mStore_FarBookAtom; 
  
  
  mork_gid         mStore_CommitGroupIdentity; 
  
  
  mork_pos         mStore_FirstCommitGroupPos; 
  mork_pos         mStore_SecondCommitGroupPos; 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  mork_bool        mStore_CanAutoAssignAtomIdentity;
  mork_bool        mStore_CanDirty; 
  mork_u1          mStore_CanWriteIncremental; 
  mork_u1          mStore_Pad; 
  
  
  
  
  
  
  
  
  
  
  
  
  

public: 
  void SetStoreDirty() { this->SetNodeDirty(); }
  void SetStoreClean() { this->SetNodeClean(); }
  
  mork_bool IsStoreClean() const { return this->IsNodeClean(); }
  mork_bool IsStoreDirty() const { return this->IsNodeDirty(); }
 
public: 
 
  void MaybeDirtyStore()
  { if ( mStore_CanDirty ) this->SetStoreDirty(); }
  
public: 

  mork_percent PercentOfStoreWasted(morkEnv* ev);
 
public: 
 
  void SetStoreAndAllSpacesCanDirty(morkEnv* ev, mork_bool inCanDirty);

public: 

  morkFarBookAtom* StageAliasAsFarBookAtom(morkEnv* ev,
    const morkMid* inMid, morkAtomSpace* ioSpace, mork_cscode inForm);

  morkFarBookAtom* StageYarnAsFarBookAtom(morkEnv* ev,
    const mdbYarn* inYarn, morkAtomSpace* ioSpace);

  morkFarBookAtom* StageStringAsFarBookAtom(morkEnv* ev,
    const char* inString, mork_cscode inForm, morkAtomSpace* ioSpace);

public: 

  mork_bool DoPreferLargeOverCompressCommit(morkEnv* ev);
  

public: 

  morkAtomSpace*   LazyGetOidAtomSpace(morkEnv* ev);
  morkAtomSpace*   LazyGetGroundAtomSpace(morkEnv* ev);
  morkAtomSpace*   LazyGetGroundColumnSpace(morkEnv* ev);

  morkStream*      LazyGetInStream(morkEnv* ev);
  morkBuilder*     LazyGetBuilder(morkEnv* ev); 
  void             ForgetBuilder(morkEnv* ev); 

  morkStream*      LazyGetOutStream(morkEnv* ev);
  
  morkRowSpace*    LazyGetRowSpace(morkEnv* ev, mdb_scope inRowScope);
  morkAtomSpace*   LazyGetAtomSpace(morkEnv* ev, mdb_scope inAtomScope);
 

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkStore(); 
  
public: 
  morkStore(morkEnv* ev, const morkUsage& inUsage,
     nsIMdbHeap* ioNodeHeap, 
     morkFactory* inFactory, 
     nsIMdbHeap* ioPortHeap  
     );
  void CloseStore(morkEnv* ev); 

private: 
  morkStore(const morkStore& other);
  morkStore& operator=(const morkStore& other);

public: 
  morkEnv*  CanUseStore(nsIMdbEnv* mev, mork_bool inMutable, mdb_err* outErr) const;
   mork_bool IsStore() const
  { return IsNode() && mNode_Derived == morkDerived_kStore; }


public: 
  static void NonStoreTypeError(morkEnv* ev);
  static void NilStoreFileError(morkEnv* ev);
  static void CannotAutoAssignAtomIdentityError(morkEnv* ev);
  
public: 
  
  morkAtom* YarnToAtom(morkEnv* ev, const mdbYarn* inYarn, PRBool createIfMissing = PR_TRUE);
  morkAtom* AddAlias(morkEnv* ev, const morkMid& inMid,
    mork_cscode inForm);

public: 

  void RenumberAllCollectableContent(morkEnv* ev);

  nsIMdbStore* AcquireStoreHandle(morkEnv* ev); 

  morkPool* StorePool() { return &mStore_Pool; }

  mork_bool OpenStoreFile(morkEnv* ev, 
    mork_bool inFrozen,
    
    nsIMdbFile* ioFile, 
    const mdbOpenPolicy* inOpenPolicy);

  mork_bool CreateStoreFile(morkEnv* ev, 
    
    nsIMdbFile* ioFile, 
    const mdbOpenPolicy* inOpenPolicy);
    
  morkAtom* CopyAtom(morkEnv* ev, const morkAtom* inAtom);
  
    
  mork_token CopyToken(morkEnv* ev, mdb_token inToken, morkStore* inStore);
  
    
  mork_token BufToToken(morkEnv* ev, const morkBuf* inBuf);
  mork_token StringToToken(morkEnv* ev, const char* inTokenName);
  mork_token QueryToken(morkEnv* ev, const char* inTokenName);
  void TokenToString(morkEnv* ev, mdb_token inToken, mdbYarn* outTokenName);
  
  mork_bool MidToOid(morkEnv* ev, const morkMid& inMid,
     mdbOid* outOid);
  mork_bool OidToYarn(morkEnv* ev, const mdbOid& inOid, mdbYarn* outYarn);
  mork_bool MidToYarn(morkEnv* ev, const morkMid& inMid,
     mdbYarn* outYarn);

  morkBookAtom* MidToAtom(morkEnv* ev, const morkMid& inMid);
  morkRow* MidToRow(morkEnv* ev, const morkMid& inMid);
  morkTable* MidToTable(morkEnv* ev, const morkMid& inMid);
  
  morkRow* OidToRow(morkEnv* ev, const mdbOid* inOid);
  

  morkTable* OidToTable(morkEnv* ev, const mdbOid* inOid,
    const mdbOid* inOptionalMetaRowOid);
  
  
  static void SmallTokenToOneByteYarn(morkEnv* ev, mdb_token inToken,
    mdbYarn* outYarn);
  
  mork_bool HasTableKind(morkEnv* ev, mdb_scope inRowScope, 
    mdb_kind inTableKind, mdb_count* outTableCount);
  
  morkTable* GetTableKind(morkEnv* ev, mdb_scope inRowScope, 
    mdb_kind inTableKind, mdb_count* outTableCount,
    mdb_bool* outMustBeUnique);

  morkRow* FindRow(morkEnv* ev, mdb_scope inScope, mdb_column inColumn,
    const mdbYarn* inTargetCellValue);
  
  morkRow* GetRow(morkEnv* ev, const mdbOid* inOid);
  morkTable* GetTable(morkEnv* ev, const mdbOid* inOid);
    
  morkTable* NewTable(morkEnv* ev, mdb_scope inRowScope,
    mdb_kind inTableKind, mdb_bool inMustBeUnique,
    const mdbOid* inOptionalMetaRowOid);

  morkPortTableCursor* GetPortTableCursor(morkEnv* ev, mdb_scope inRowScope,
    mdb_kind inTableKind) ;

  morkRow* NewRowWithOid(morkEnv* ev, const mdbOid* inOid);
  morkRow* NewRow(morkEnv* ev, mdb_scope inRowScope);

  morkThumb* MakeCompressCommitThumb(morkEnv* ev, mork_bool inDoCollect);

public: 

  mork_bool MarkAllStoreContentDirty(morkEnv* ev);
  
  
  
  
  
  
  
  

public: 
  static void SlotWeakStore(morkStore* me,
    morkEnv* ev, morkStore** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongStore(morkStore* me,
    morkEnv* ev, morkStore** ioSlot)
  { 
    morkStore* store = *ioSlot;
    if ( me != store )
    {
      if ( store )
      {
        
        
        *ioSlot = 0;
        store->Release();
      }
      if ( me && me->AddRef() )
        *ioSlot = me;
    }
  }
};



#endif 

