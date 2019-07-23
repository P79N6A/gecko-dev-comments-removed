




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKCH_
#include "morkCh.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKFACTORY_
#include "morkFactory.h"
#endif






 void
morkEnv::CloseMorkNode(morkEnv* ev)  
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseEnv(ev);
    this->MarkShut();
  }
}


morkEnv::~morkEnv()  
{
  CloseMorkNode(mMorkEnv);
  if (mEnv_Heap)
  {
    mork_bool ownsHeap = mEnv_OwnsHeap;
    nsIMdbHeap*saveHeap = mEnv_Heap;

    if (ownsHeap)
    {
#ifdef MORK_DEBUG_HEAP_STATS
      printf("%d blocks remaining \n", ((orkinHeap *) saveHeap)->HeapBlockCount());
      mork_u4* array = (mork_u4*) this;
      array -= 3;
      
      
      *array = nsnull;
#endif 
      
      
      delete saveHeap;
    }

  }

  MORK_ASSERT(mEnv_ErrorHook==0);
}


#define morkEnv_kBeVerbose morkBool_kFalse


morkEnv::morkEnv(const morkUsage& inUsage, nsIMdbHeap* ioHeap,
  morkFactory* ioFactory, nsIMdbHeap* ioSlotHeap)
: morkObject(inUsage, ioHeap, morkColor_kNone)
, mEnv_Factory( ioFactory )
, mEnv_Heap( ioSlotHeap )

, mEnv_SelfAsMdbEnv( 0 )
, mEnv_ErrorHook( 0 )
, mEnv_HandlePool( 0 )
  
, mEnv_ErrorCount( 0 ) 
, mEnv_WarningCount( 0 ) 

, mEnv_ErrorCode( 0 )
  
, mEnv_DoTrace( morkBool_kFalse )
, mEnv_AutoClear( morkAble_kDisabled )
, mEnv_ShouldAbort( morkBool_kFalse )
, mEnv_BeVerbose( morkEnv_kBeVerbose )
, mEnv_OwnsHeap ( morkBool_kFalse )
{
  MORK_ASSERT(ioSlotHeap && ioFactory );
  if ( ioSlotHeap )
  {
    
    
    
    mEnv_HandlePool = new morkPool(morkUsage::kGlobal,
      (nsIMdbHeap*) 0, ioSlotHeap);
      
    MORK_ASSERT(mEnv_HandlePool);
    if ( mEnv_HandlePool && this->Good() )
    {
      mNode_Derived = morkDerived_kEnv;
      mNode_Refs += morkEnv_kWeakRefCountEnvBonus;
    }
  }
}


morkEnv::morkEnv(morkEnv* ev, 
  const morkUsage& inUsage, nsIMdbHeap* ioHeap, nsIMdbEnv* inSelfAsMdbEnv,
  morkFactory* ioFactory, nsIMdbHeap* ioSlotHeap)
: morkObject(ev, inUsage, ioHeap, morkColor_kNone, (morkHandle*) 0)
, mEnv_Factory( ioFactory )
, mEnv_Heap( ioSlotHeap )

, mEnv_SelfAsMdbEnv( inSelfAsMdbEnv )
, mEnv_ErrorHook( 0 )
, mEnv_HandlePool( 0 )
  
, mEnv_ErrorCount( 0 ) 
, mEnv_WarningCount( 0 ) 

, mEnv_ErrorCode( 0 )
  
, mEnv_DoTrace( morkBool_kFalse )
, mEnv_AutoClear( morkAble_kDisabled )
, mEnv_ShouldAbort( morkBool_kFalse )
, mEnv_BeVerbose( morkEnv_kBeVerbose )
, mEnv_OwnsHeap ( morkBool_kFalse )
{
  
  
  if ( ioFactory && inSelfAsMdbEnv && ioSlotHeap)
  {
    
    

    mEnv_HandlePool = new(*ioSlotHeap, ev) morkPool(ev, 
      morkUsage::kHeap, ioSlotHeap, ioSlotHeap);
      
    MORK_ASSERT(mEnv_HandlePool);
    if ( mEnv_HandlePool && ev->Good() )
    {
      mNode_Derived = morkDerived_kEnv;
      mNode_Refs += morkEnv_kWeakRefCountEnvBonus;
    }
  }
  else
    ev->NilPointerError();
}

NS_IMPL_ISUPPORTS_INHERITED1(morkEnv, morkObject, nsIMdbEnv)
 void
morkEnv::CloseEnv(morkEnv* ev)  
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      
      
      
      mEnv_SelfAsMdbEnv = 0;
      mEnv_ErrorHook = 0;
      
      morkPool* savePool = mEnv_HandlePool;
      morkPool::SlotStrongPool((morkPool*) 0, ev, &mEnv_HandlePool);
      
      if (mEnv_SelfAsMdbEnv)
      {
        if (savePool && mEnv_Heap)
          mEnv_Heap->Free(this->AsMdbEnv(), savePool);
      }
      else
      {
        if (savePool)
        {
          if (savePool->IsOpenNode())
            savePool->CloseMorkNode(ev);
          delete savePool;
        }
        
      }
      
      
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




mork_size
morkEnv::OidAsHex(void* outBuf, const mdbOid& inOid)

{
  mork_u1* p = (mork_u1*) outBuf;
  mork_size outSize = this->TokenAsHex(p, inOid.mOid_Id);
  p += outSize;
  *p++ = ':';
  
  mork_scope scope = inOid.mOid_Scope;
  if ( scope < 0x80 && morkCh_IsName((mork_ch) scope) )
  {
    *p++ = (mork_u1) scope;
    *p = 0; 
    outSize += 2;
  }
  else
  {
    *p++ = '^';
    mork_size scopeSize = this->TokenAsHex(p, scope);
    outSize += scopeSize + 2;
  }
  return outSize;
}


mork_u1
morkEnv::HexToByte(mork_ch inFirstHex, mork_ch inSecondHex)
{
  mork_u1 hi = 0; 
  mork_flags f = morkCh_GetFlags(inFirstHex);
  if ( morkFlags_IsDigit(f) )
    hi = (mork_u1) (inFirstHex - (mork_ch) '0');
  else if ( morkFlags_IsUpper(f) )
    hi = (mork_u1) ((inFirstHex - (mork_ch) 'A') + 10);
  else if ( morkFlags_IsLower(f) )
    hi = (mork_u1) ((inFirstHex - (mork_ch) 'a') + 10);
  
  mork_u1 lo = 0; 
  f = morkCh_GetFlags(inSecondHex);
  if ( morkFlags_IsDigit(f) )
    lo = (mork_u1) (inSecondHex - (mork_ch) '0');
  else if ( morkFlags_IsUpper(f) )
    lo = (mork_u1) ((inSecondHex - (mork_ch) 'A') + 10);
  else if ( morkFlags_IsLower(f) )
    lo = (mork_u1) ((inSecondHex - (mork_ch) 'a') + 10);
    
  return (mork_u1) ((hi << 4) | lo);
}

mork_size
morkEnv::TokenAsHex(void* outBuf, mork_token inToken)
  
{
  static const char morkEnv_kHexDigits[] = "0123456789ABCDEF";
  char* p = (char*) outBuf;
  char* end = p + 32; 
  if ( inToken )
  {
    
    while ( p < end && inToken ) 
    {
      *p++ = morkEnv_kHexDigits[ inToken & 0x0F ]; 
      inToken >>= 4; 
    }
    *p = 0; 
    char* s = (char*) outBuf; 
    mork_size size = (mork_size) (p - s); 

    
    
    while ( --p > s ) 
    {
      char c = *p; 
      *p = *s;
      *s++ = c; 
    }
    return size;
  }
  else 
  {
    *p++ = '0'; 
    *p = 0; 
    return 1; 
  }
}

void
morkEnv::StringToYarn(const char* inString, mdbYarn* outYarn)
{
  if ( outYarn )
  {
    mdb_fill fill = ( inString )? (mdb_fill) MORK_STRLEN(inString) : 0; 
      
    if ( fill ) 
    {
      mdb_size size = outYarn->mYarn_Size; 
      if ( fill > size ) 
      {
        outYarn->mYarn_More = fill - size; 
        fill = size; 
      }
      void* dest = outYarn->mYarn_Buf; 
      if ( !dest ) 
        fill = 0; 
        
      if ( fill ) 
        MORK_MEMCPY(dest, inString, fill); 
        
      outYarn->mYarn_Fill = fill; 
    }
    else 
    {
      outYarn->mYarn_Fill = 0; 
    }
    outYarn->mYarn_Form = 0; 
  }
  else
    this->NilPointerError();
}

char*
morkEnv::CopyString(nsIMdbHeap* ioHeap, const char* inString)
{
  char* outString = 0;
  if ( ioHeap && inString )
  {
    mork_size size = MORK_STRLEN(inString) + 1;
    ioHeap->Alloc(this->AsMdbEnv(), size, (void**) &outString);
    if ( outString )
      MORK_STRCPY(outString, inString);
  }
  else
    this->NilPointerError();
  return outString;
}

void
morkEnv::FreeString(nsIMdbHeap* ioHeap, char* ioString)
{
  if ( ioHeap )
  {
    if ( ioString )
      ioHeap->Free(this->AsMdbEnv(), ioString);
  }
  else
    this->NilPointerError();
}

void
morkEnv::NewErrorAndCode(const char* inString, mork_u2 inCode)
{
  MORK_ASSERT(morkBool_kFalse); 

  ++mEnv_ErrorCount;
  mEnv_ErrorCode = (mork_u4) ((inCode)? inCode: morkEnv_kGenericError);
  
  if ( mEnv_ErrorHook )
    mEnv_ErrorHook->OnErrorString(this->AsMdbEnv(), inString);
}

void
morkEnv::NewError(const char* inString)
{
  MORK_ASSERT(morkBool_kFalse); 

  ++mEnv_ErrorCount;
  mEnv_ErrorCode = morkEnv_kGenericError;
  
  if ( mEnv_ErrorHook )
    mEnv_ErrorHook->OnErrorString(this->AsMdbEnv(), inString);
}

void
morkEnv::NewWarning(const char* inString)
{
  MORK_ASSERT(morkBool_kFalse); 
  
  ++mEnv_WarningCount;
  if ( mEnv_ErrorHook )
    mEnv_ErrorHook->OnWarningString(this->AsMdbEnv(), inString);
}

void
morkEnv::StubMethodOnlyError()
{
  this->NewError("method is stub only");
}

void
morkEnv::OutOfMemoryError()
{
  this->NewError("out of memory");
}

void
morkEnv::CantMakeWhenBadError()
{
  this->NewError("can't make an object when ev->Bad()");
}

static const char morkEnv_kNilPointer[] = "nil pointer";

void
morkEnv::NilPointerError()
{
  this->NewError(morkEnv_kNilPointer);
}

void
morkEnv::NilPointerWarning()
{
  this->NewWarning(morkEnv_kNilPointer);
}

void
morkEnv::NewNonEnvError()
{
  this->NewError("non-env instance");
}

void
morkEnv::NilEnvSlotError()
{
  if ( !mEnv_HandlePool || !mEnv_Factory )
  {
    if ( !mEnv_HandlePool )
      this->NewError("nil mEnv_HandlePool");
    if ( !mEnv_Factory )
      this->NewError("nil mEnv_Factory");
  }
  else
    this->NewError("unknown nil env slot");
}


void morkEnv::NonEnvTypeError(morkEnv* ev)
{
  ev->NewError("non morkEnv");
}

void
morkEnv::ClearMorkErrorsAndWarnings()
{
  mEnv_ErrorCount = 0;
  mEnv_WarningCount = 0;
  mEnv_ErrorCode = 0;
  mEnv_ShouldAbort = morkBool_kFalse;
}

void
morkEnv::AutoClearMorkErrorsAndWarnings()
{
  if ( this->DoAutoClear() )
  {
    mEnv_ErrorCount = 0;
    mEnv_WarningCount = 0;
    mEnv_ErrorCode = 0;
    mEnv_ShouldAbort = morkBool_kFalse;
  }
}

 morkEnv*
morkEnv::FromMdbEnv(nsIMdbEnv* ioEnv) 
{
  morkEnv* outEnv = 0;
  if ( ioEnv )
  {
    
    
    
    morkEnv* ev = (morkEnv*) ioEnv;
    if ( ev && ev->IsEnv() )
    {
      if ( ev->DoAutoClear() )
      {
        ev->mEnv_ErrorCount = 0;
        ev->mEnv_WarningCount = 0;
        ev->mEnv_ErrorCode = 0;
      }
      outEnv = ev;
    }
    else
      MORK_ASSERT(outEnv);
  }
  else
    MORK_ASSERT(outEnv);
  return outEnv;
}


NS_IMETHODIMP
morkEnv::GetErrorCount(mdb_count* outCount,
  mdb_bool* outShouldAbort)
{
  if ( outCount )
    *outCount = mEnv_ErrorCount;
  if ( outShouldAbort )
    *outShouldAbort = mEnv_ShouldAbort;
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::GetWarningCount(mdb_count* outCount,
  mdb_bool* outShouldAbort)
{
  if ( outCount )
    *outCount = mEnv_WarningCount;
  if ( outShouldAbort )
    *outShouldAbort = mEnv_ShouldAbort;
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::GetEnvBeVerbose(mdb_bool* outBeVerbose)
{
  NS_ENSURE_ARG_POINTER(outBeVerbose);
  *outBeVerbose = mEnv_BeVerbose;
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::SetEnvBeVerbose(mdb_bool inBeVerbose)
{
  mEnv_BeVerbose = inBeVerbose;
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::GetDoTrace(mdb_bool* outDoTrace)
{
  NS_ENSURE_ARG_POINTER(outDoTrace);
  *outDoTrace = mEnv_DoTrace;
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::SetDoTrace(mdb_bool inDoTrace)
{
  mEnv_DoTrace = inDoTrace;
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::GetAutoClear(mdb_bool* outAutoClear)
{
  NS_ENSURE_ARG_POINTER(outAutoClear);
  *outAutoClear = DoAutoClear();
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::SetAutoClear(mdb_bool inAutoClear)
{
  if ( inAutoClear )
    EnableAutoClear();
  else
    DisableAutoClear();
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::GetErrorHook(nsIMdbErrorHook** acqErrorHook)
{
  NS_ENSURE_ARG_POINTER(acqErrorHook);
  *acqErrorHook = mEnv_ErrorHook;
  NS_IF_ADDREF(mEnv_ErrorHook);
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::SetErrorHook(
  nsIMdbErrorHook* ioErrorHook) 
{
  mEnv_ErrorHook = ioErrorHook;
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::GetHeap(nsIMdbHeap** acqHeap)
{
  NS_ENSURE_ARG_POINTER(acqHeap);
  nsIMdbHeap* outHeap = 0;
  nsIMdbHeap* heap = mEnv_Heap;
  if ( heap && heap->HeapAddStrongRef(this) == 0 )
    outHeap = heap;

  if ( acqHeap )
    *acqHeap = outHeap;
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::SetHeap(
  nsIMdbHeap* ioHeap) 
{
  nsIMdbHeap_SlotStrongHeap(ioHeap, this, &mEnv_Heap);
  return NS_OK;
}


NS_IMETHODIMP
morkEnv::ClearErrors() 
{
  mEnv_ErrorCount = 0;
  mEnv_ErrorCode = 0;
  mEnv_ShouldAbort = morkBool_kFalse;

  return NS_OK;
}

NS_IMETHODIMP
morkEnv::ClearWarnings() 
{
  mEnv_WarningCount = 0;
  return NS_OK;
}

NS_IMETHODIMP
morkEnv::ClearErrorsAndWarnings() 
{
  ClearMorkErrorsAndWarnings();
  return NS_OK;
}




