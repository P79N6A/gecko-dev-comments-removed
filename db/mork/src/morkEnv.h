




































#ifndef _MORKENV_
#define _MORKENV_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif

#ifndef _MORKPOOL_
#include "morkPool.h"
#endif


#include "nsError.h"



#define morkDerived_kEnv 0x4576 /* ascii 'Ev' */


#define morkEnv_kNoError         NS_SUCCEEDED /* no error has happened */
#define morkEnv_kGenericError    NS_ERROR_FAILURE /* non-specific error code */
#define morkEnv_kNonEnvTypeError NS_ERROR_FAILURE /* morkEnv::IsEnv() is false */

#define morkEnv_kStubMethodOnlyError NS_ERROR_NO_INTERFACE
#define morkEnv_kOutOfMemoryError    NS_ERROR_OUT_OF_MEMORY
#define morkEnv_kNilPointerError     NS_ERROR_NULL_POINTER
#define morkEnv_kNewNonEnvError      NS_ERROR_FAILURE 
#define morkEnv_kNilEnvSlotError     NS_ERROR_FAILURE

#define morkEnv_kBadFactoryError     NS_ERROR_FACTORY_NOT_LOADED
#define morkEnv_kBadFactoryEnvError  NS_ERROR_FACTORY_NOT_LOADED
#define morkEnv_kBadEnvError         NS_ERROR_FAILURE

#define morkEnv_kNonHandleTypeError  NS_ERROR_FAILURE
#define morkEnv_kNonOpenNodeError    NS_ERROR_FAILURE 


#define morkEnv_kWeakRefCountEnvBonus 0 /* try NOT to leak all env instances */



class morkEnv : public morkObject, public nsIMdbEnv {
  NS_DECL_ISUPPORTS_INHERITED


  

  
  
  
  
  
  
  
  
  
  

  
  

public: 
  
  morkFactory*      mEnv_Factory;  
  nsIMdbHeap*       mEnv_Heap;     

  nsIMdbEnv*        mEnv_SelfAsMdbEnv;
  nsIMdbErrorHook*  mEnv_ErrorHook;
  
  morkPool*         mEnv_HandlePool; 
    
  mork_u2           mEnv_ErrorCount; 
  mork_u2           mEnv_WarningCount; 
  
  mork_u4           mEnv_ErrorCode; 
  
  mork_bool         mEnv_DoTrace;
  mork_able         mEnv_AutoClear;
  mork_bool         mEnv_ShouldAbort;
  mork_bool         mEnv_BeVerbose;
  mork_bool         mEnv_OwnsHeap;
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkEnv(); 
  
  
  NS_IMETHOD GetErrorCount(mdb_count* outCount,
    mdb_bool* outShouldAbort);
  NS_IMETHOD GetWarningCount(mdb_count* outCount,
    mdb_bool* outShouldAbort);
  
  NS_IMETHOD GetEnvBeVerbose(mdb_bool* outBeVerbose);
  NS_IMETHOD SetEnvBeVerbose(mdb_bool inBeVerbose);
  
  NS_IMETHOD GetDoTrace(mdb_bool* outDoTrace);
  NS_IMETHOD SetDoTrace(mdb_bool inDoTrace);
  
  NS_IMETHOD GetAutoClear(mdb_bool* outAutoClear);
  NS_IMETHOD SetAutoClear(mdb_bool inAutoClear);
  
  NS_IMETHOD GetErrorHook(nsIMdbErrorHook** acqErrorHook);
  NS_IMETHOD SetErrorHook(
    nsIMdbErrorHook* ioErrorHook); 
  
  NS_IMETHOD GetHeap(nsIMdbHeap** acqHeap);
  NS_IMETHOD SetHeap(
    nsIMdbHeap* ioHeap); 
  
  
  NS_IMETHOD ClearErrors(); 
  NS_IMETHOD ClearWarnings(); 
  NS_IMETHOD ClearErrorsAndWarnings(); 

public: 
  morkEnv(const morkUsage& inUsage, nsIMdbHeap* ioHeap,
    morkFactory* ioFactory, nsIMdbHeap* ioSlotHeap);
  morkEnv(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap, 
     nsIMdbEnv* inSelfAsMdbEnv, morkFactory* ioFactory,
     nsIMdbHeap* ioSlotHeap);
  void CloseEnv(morkEnv* ev); 

private: 
  morkEnv(const morkEnv& other);
  morkEnv& operator=(const morkEnv& other);

public: 
  mork_bool IsEnv() const
  { return IsNode() && mNode_Derived == morkDerived_kEnv; }


public: 

  mork_u1 HexToByte(mork_ch inFirstHex, mork_ch inSecondHex);

  mork_size TokenAsHex(void* outBuf, mork_token inToken);
  
 
  mork_size OidAsHex(void* outBuf, const mdbOid& inOid);
  
 
  char* CopyString(nsIMdbHeap* ioHeap, const char* inString);
  void  FreeString(nsIMdbHeap* ioHeap, char* ioString);
  void  StringToYarn(const char* inString, mdbYarn* outYarn);

public: 

  morkHandleFace*  NewHandle(mork_size inSize)
  { return mEnv_HandlePool->NewHandle(this, inSize, (morkZone*) 0); }
  
  void ZapHandle(morkHandleFace* ioHandle)
  { mEnv_HandlePool->ZapHandle(this, ioHandle); }

  void EnableAutoClear() { mEnv_AutoClear = morkAble_kEnabled; }
  void DisableAutoClear() { mEnv_AutoClear = morkAble_kDisabled; }
  
  mork_bool DoAutoClear() const
  { return mEnv_AutoClear == morkAble_kEnabled; }

  void NewErrorAndCode(const char* inString, mork_u2 inCode);
  void NewError(const char* inString);
  void NewWarning(const char* inString);

  void ClearMorkErrorsAndWarnings(); 
  void AutoClearMorkErrorsAndWarnings(); 
  
  void StubMethodOnlyError();
  void OutOfMemoryError();
  void NilPointerError();
  void NilPointerWarning();
  void CantMakeWhenBadError();
  void NewNonEnvError();
  void NilEnvSlotError();
    
  void NonEnvTypeError(morkEnv* ev);
  
  
  mork_bool Good() const { return ( mEnv_ErrorCount == 0 ); }
  mork_bool Bad() const { return ( mEnv_ErrorCount != 0 ); }
  
  nsIMdbEnv* AsMdbEnv() { return (nsIMdbEnv *) this; }
  static morkEnv* FromMdbEnv(nsIMdbEnv* ioEnv); 
  
  mork_u4 ErrorCode() const { return mEnv_ErrorCode; }
  
  mdb_err AsErr() const { return (mdb_err) mEnv_ErrorCode; }
  

public: 
  static void SlotWeakEnv(morkEnv* me,
    morkEnv* ev, morkEnv** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongEnv(morkEnv* me,
    morkEnv* ev, morkEnv** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
