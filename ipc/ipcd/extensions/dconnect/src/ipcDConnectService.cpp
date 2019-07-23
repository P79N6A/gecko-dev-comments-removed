




































#include <stdio.h>

#include "ipcDConnectService.h"
#include "ipcMessageWriter.h"
#include "ipcMessageReader.h"
#include "ipcLog.h"

#include "nsServiceManagerUtils.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsCRT.h"
#include "xptcall.h"








#define DCONNECT_IPC_TARGETID                      \
{ /* 43ca47ef-ebc8-47a2-9679-a4703218089f */       \
  0x43ca47ef,                                      \
  0xebc8,                                          \
  0x47a2,                                          \
  {0x96, 0x79, 0xa4, 0x70, 0x32, 0x18, 0x08, 0x9f} \
}
static const nsID kDConnectTargetID = DCONNECT_IPC_TARGETID;



#define DCON_WAIT_TIMEOUT PR_INTERVAL_NO_TIMEOUT



typedef unsigned long PtrBits;
















#define DCON_OP_SETUP   1
#define DCON_OP_RELEASE 2
#define DCON_OP_INVOKE  3

#define DCON_OP_SETUP_REPLY  4
#define DCON_OP_INVOKE_REPLY 5


#define DCON_OP_SETUP_NEW_INST_CLASSID    1
#define DCON_OP_SETUP_NEW_INST_CONTRACTID 2
#define DCON_OP_SETUP_GET_SERV_CLASSID    3
#define DCON_OP_SETUP_GET_SERV_CONTRACTID 4
#define DCON_OP_SETUP_QUERY_INTERFACE     5




struct DConnectOp
{
  PRUint8  opcode_major;
  PRUint8  opcode_minor;
  PRUint32 request_index; 
};

typedef class DConnectInstance* DConAddr;



struct DConnectSetup : DConnectOp
{
  nsID iid;
};

struct DConnectSetupClassID : DConnectSetup
{
  nsID classid;
};

struct DConnectSetupContractID : DConnectSetup
{
  char contractid[1]; 
};

struct DConnectSetupQueryInterface : DConnectSetup
{
  DConAddr instance;
};



struct DConnectSetupReply : DConnectOp
{
  DConAddr instance;
  nsresult status;
};



struct DConnectRelease : DConnectOp
{
  DConAddr instance;
};



struct DConnectInvoke : DConnectOp
{
  DConAddr instance;
  PRUint16 method_index;
  
};



struct DConnectInvokeReply : DConnectOp
{
  nsresult result;
  
};



static ipcDConnectService *gDConnect;



static nsresult
SetupPeerInstance(PRUint32 aPeerID, DConnectSetup *aMsg, PRUint32 aMsgLen,
                  void **aInstancePtr);





class DConnectInstance
{
public:
  DConnectInstance(PRUint32 peer, nsIInterfaceInfo *iinfo, nsISupports *instance)
    : mPeer(peer)
    , mIInfo(iinfo)
    , mInstance(instance)
  {}

  nsISupports      *RealInstance()  { return mInstance; }
  nsIInterfaceInfo *InterfaceInfo() { return mIInfo; }
  
private:
  PRUint32                   mPeer;  
  nsCOMPtr<nsIInterfaceInfo> mIInfo;
  nsCOMPtr<nsISupports>      mInstance;
};

static void
DeleteWrappers(nsVoidArray &wrappers)
{
  for (PRInt32 i=0; i<wrappers.Count(); ++i)
    gDConnect->DeleteInstance((DConnectInstance *) wrappers[i]);
}



static nsresult
SerializeParam(ipcMessageWriter &writer, const nsXPTType &t, const nsXPTCMiniVariant &v)
{
  switch (t.TagPart())
  {
    case nsXPTType::T_I8:
    case nsXPTType::T_U8:
      writer.PutInt8(v.val.u8);
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U16:
      writer.PutInt16(v.val.u16);
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U32:
      writer.PutInt32(v.val.u32);
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U64:
      writer.PutBytes(&v.val.u64, sizeof(PRUint64));
      break;

    case nsXPTType::T_FLOAT:
      writer.PutBytes(&v.val.f, sizeof(float));
      break;

    case nsXPTType::T_DOUBLE:
      writer.PutBytes(&v.val.d, sizeof(double));
      break;

    case nsXPTType::T_BOOL:
      writer.PutBytes(&v.val.b, sizeof(PRBool));
      break;

    case nsXPTType::T_CHAR:
      writer.PutBytes(&v.val.c, sizeof(char));
      break;

    case nsXPTType::T_WCHAR:
      writer.PutBytes(&v.val.wc, sizeof(PRUnichar));
      break;

    case nsXPTType::T_IID:
      writer.PutBytes(v.val.p, sizeof(nsID));
      break;

    case nsXPTType::T_CHAR_STR:
      {
        int len = strlen((const char *) v.val.p);
        writer.PutInt32(len);
        writer.PutBytes(v.val.p, len);
      }
      break;

    case nsXPTType::T_WCHAR_STR:
      {
        int len = 2 * nsCRT::strlen((const PRUnichar *) v.val.p);
        writer.PutInt32(len);
        writer.PutBytes(v.val.p, len);
      }
      break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
      NS_NOTREACHED("this should be handled elsewhere");
      return NS_ERROR_UNEXPECTED;

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
      {
        const nsAString *str = (const nsAString *) v.val.p;

        PRUint32 len = 2 * str->Length();
        nsAString::const_iterator begin;
        const PRUnichar *data = str->BeginReading(begin).get();

        writer.PutInt32(len);
        writer.PutBytes(data, len);
      }
      break;

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
      {
        const nsACString *str = (const nsACString *) v.val.p;

        PRUint32 len = str->Length();
        nsACString::const_iterator begin;
        const char *data = str->BeginReading(begin).get();

        writer.PutInt32(len);
        writer.PutBytes(data, len);
      }
      break;

    case nsXPTType::T_ARRAY:
      LOG(("array types are not yet supported\n"));
      return NS_ERROR_NOT_IMPLEMENTED;

    case nsXPTType::T_VOID:
    case nsXPTType::T_PSTRING_SIZE_IS:
    case nsXPTType::T_PWSTRING_SIZE_IS:
    default:
      LOG(("unexpected parameter type\n"));
      return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}

static nsresult
DeserializeParam(ipcMessageReader &reader, const nsXPTType &t, nsXPTCVariant &v)
{
  
  v.ptr = nsnull;
  v.type = t;
  v.flags = 0;

  switch (t.TagPart())
  {
    case nsXPTType::T_I8:
    case nsXPTType::T_U8:
      v.val.u8 = reader.GetInt8();
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U16:
      v.val.u16 = reader.GetInt16();
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U32:
      v.val.u32 = reader.GetInt32();
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U64:
      reader.GetBytes(&v.val.u64, sizeof(v.val.u64));
      break;

    case nsXPTType::T_FLOAT:
      reader.GetBytes(&v.val.f, sizeof(v.val.f));
      break;

    case nsXPTType::T_DOUBLE:
      reader.GetBytes(&v.val.d, sizeof(v.val.d));
      break;

    case nsXPTType::T_BOOL:
      reader.GetBytes(&v.val.b, sizeof(v.val.b));
      break;

    case nsXPTType::T_CHAR:
      reader.GetBytes(&v.val.c, sizeof(v.val.c));
      break;

    case nsXPTType::T_WCHAR:
      reader.GetBytes(&v.val.wc, sizeof(v.val.wc));
      break;

    case nsXPTType::T_IID:
      {
        nsID *buf = (nsID *) malloc(sizeof(nsID));
        NS_ENSURE_TRUE(buf, NS_ERROR_OUT_OF_MEMORY);
        reader.GetBytes(buf, sizeof(nsID));
        v.val.p = v.ptr = buf;
        v.flags = nsXPTCVariant::PTR_IS_DATA | nsXPTCVariant::VAL_IS_ALLOCD;
      }
      break;

    case nsXPTType::T_CHAR_STR:
      {
        PRUint32 len = reader.GetInt32();
        char *buf = (char *) malloc(len + 1);
        NS_ENSURE_TRUE(buf, NS_ERROR_OUT_OF_MEMORY);
        reader.GetBytes(buf, len);
        buf[len] = char(0);

        v.val.p = v.ptr = buf;
        v.flags = nsXPTCVariant::PTR_IS_DATA | nsXPTCVariant::VAL_IS_ALLOCD;
      }
      break;

    case nsXPTType::T_WCHAR_STR:
      {
        PRUint32 len = reader.GetInt32();
        PRUnichar *buf = (PRUnichar *) malloc(len + 2);
        NS_ENSURE_TRUE(buf, NS_ERROR_OUT_OF_MEMORY);
        reader.GetBytes(buf, len);
        buf[len] = PRUnichar(0);

        v.val.p = v.ptr = buf;
        v.flags = nsXPTCVariant::PTR_IS_DATA | nsXPTCVariant::VAL_IS_ALLOCD;
      }
      break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
      {
        reader.GetBytes(&v.ptr, sizeof(void *));
        v.val.p = nsnull;
        v.flags = nsXPTCVariant::PTR_IS_DATA;
      }
      break;

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
      {
        PRUint32 len = reader.GetInt32();

        nsString *str = new nsString();
        if (!str || !(EnsureStringLength(*str, len/2)))
          return NS_ERROR_OUT_OF_MEMORY;
        PRUnichar *buf = str->BeginWriting();
        reader.GetBytes(buf, len);

        v.val.p = v.ptr = str;
        v.flags = nsXPTCVariant::PTR_IS_DATA | nsXPTCVariant::VAL_IS_DOMSTR;
      }
      break;

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
      {
        PRUint32 len = reader.GetInt32();

        nsCString *str = new nsCString();
        if (!str || !(EnsureStringLength(*str, len)))
          return NS_ERROR_OUT_OF_MEMORY;
        char *buf = str->BeginWriting();
        reader.GetBytes(buf, len);

        v.val.p = v.ptr = str;
        v.flags = nsXPTCVariant::PTR_IS_DATA;

        
        if (t.TagPart() == nsXPTType::T_CSTRING)
          v.flags |= nsXPTCVariant::VAL_IS_CSTR;
        else
          v.flags |= nsXPTCVariant::VAL_IS_UTF8STR;
      }
      break;

    case nsXPTType::T_ARRAY:
      LOG(("array types are not yet supported\n"));
      return NS_ERROR_NOT_IMPLEMENTED;

    case nsXPTType::T_VOID:
    case nsXPTType::T_PSTRING_SIZE_IS:
    case nsXPTType::T_PWSTRING_SIZE_IS:
    default:
      LOG(("unexpected parameter type\n"));
      return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}

static nsresult
SetupParam(const nsXPTParamInfo &p, nsXPTCVariant &v)
{
  const nsXPTType &t = p.GetType();

  if (p.IsIn() && p.IsDipper())
  {
    v.ptr = nsnull;

    switch (t.TagPart())
    {
      case nsXPTType::T_ASTRING:
      case nsXPTType::T_DOMSTRING:
        v.ptr = new nsString();
        if (!v.ptr)
          return NS_ERROR_OUT_OF_MEMORY;
        v.val.p = v.ptr;
        v.type = t;
        v.flags = nsXPTCVariant::PTR_IS_DATA | nsXPTCVariant::VAL_IS_DOMSTR;
        break;

      case nsXPTType::T_UTF8STRING:
      case nsXPTType::T_CSTRING:
        v.ptr = new nsCString();
        if (!v.ptr)
          return NS_ERROR_OUT_OF_MEMORY;
        v.val.p = v.ptr;
        v.type = t;
        v.flags = nsXPTCVariant::PTR_IS_DATA | nsXPTCVariant::VAL_IS_CSTR;
        break;

      default:
        LOG(("unhandled dipper: type=%d\n", t.TagPart()));
        return NS_ERROR_UNEXPECTED;
    }
  }
  else if (p.IsOut())
  {
    v.ptr = &v.val;
    v.type = t;
    v.flags = nsXPTCVariant::PTR_IS_DATA;
  }

  return NS_OK;
}

static void
FinishParam(nsXPTCVariant &v)
{
  if (!v.val.p)
    return;

  if (v.IsValAllocated())
    free(v.val.p);
  else if (v.IsValInterface())
    ((nsISupports *) v.val.p)->Release();
  else if (v.IsValDOMString())
    delete (nsAString *) v.val.p;
  else if (v.IsValUTF8String() || v.IsValCString())
    delete (nsACString *) v.val.p;
}

static nsresult
DeserializeResult(ipcMessageReader &reader, const nsXPTType &t, nsXPTCMiniVariant &v)
{
  if (v.val.p == nsnull)
    return NS_OK;

  switch (t.TagPart())
  {
    case nsXPTType::T_I8:
    case nsXPTType::T_U8:
      *((PRUint8 *) v.val.p) = reader.GetInt8();
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U16:
      *((PRUint16 *) v.val.p) = reader.GetInt16();
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U32:
      *((PRUint32 *) v.val.p) = reader.GetInt32();
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U64:
      reader.GetBytes(v.val.p, sizeof(PRUint64));
      break;

    case nsXPTType::T_FLOAT:
      reader.GetBytes(v.val.p, sizeof(float));
      break;

    case nsXPTType::T_DOUBLE:
      reader.GetBytes(v.val.p, sizeof(double));
      break;

    case nsXPTType::T_BOOL:
      reader.GetBytes(v.val.p, sizeof(PRBool));
      break;

    case nsXPTType::T_CHAR:
      reader.GetBytes(v.val.p, sizeof(char));
      break;

    case nsXPTType::T_WCHAR:
      reader.GetBytes(v.val.p, sizeof(PRUnichar));
      break;

    case nsXPTType::T_IID:
      {
        nsID *buf = (nsID *) nsMemory::Alloc(sizeof(nsID));
        reader.GetBytes(buf, sizeof(nsID));
        *((nsID **) v.val.p) = buf;
      }
      break;

    case nsXPTType::T_CHAR_STR:
      {
        PRUint32 len = reader.GetInt32();
        char *buf = (char *) nsMemory::Alloc(len + 1);
        reader.GetBytes(buf, len);
        buf[len] = char(0);

        *((char **) v.val.p) = buf;
      }
      break;

    case nsXPTType::T_WCHAR_STR:
      {
        PRUint32 len = reader.GetInt32();
        PRUnichar *buf = (PRUnichar *) nsMemory::Alloc(len + 2);
        reader.GetBytes(buf, len);
        buf[len] = PRUnichar(0);

        *((PRUnichar **) v.val.p) = buf;
      }
      break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
      {
        
        
        void *ptr;
        reader.GetBytes(&ptr, sizeof(void *));
        *((void **) v.val.p) = ptr;
      }
      break;

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
      {
        PRUint32 len = reader.GetInt32();

        nsAString *str = (nsAString *) v.val.p;

        if (!str || !(EnsureStringLength(*str, len/2)))
          return NS_ERROR_OUT_OF_MEMORY;
        nsAString::iterator begin;
        str->BeginWriting(begin);

        reader.GetBytes(begin.get(), len);
      }
      break;

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
      {
        PRUint32 len = reader.GetInt32();

        nsACString *str = (nsACString *) v.val.p;

        if (!str || !(EnsureStringLength(*str, len)))
          return NS_ERROR_OUT_OF_MEMORY;
        nsACString::iterator begin;
        str->BeginWriting(begin);

        reader.GetBytes(begin.get(), len);
      }
      break;

    case nsXPTType::T_ARRAY:
      LOG(("array types are not yet supported\n"));
      return NS_ERROR_NOT_IMPLEMENTED;

    case nsXPTType::T_VOID:
    case nsXPTType::T_PSTRING_SIZE_IS:
    case nsXPTType::T_PWSTRING_SIZE_IS:
    default:
      LOG(("unexpected parameter type\n"));
      return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}



static PRUint32
NewRequestIndex()
{
  static PRUint32 sRequestIndex;
  return ++sRequestIndex;
}



class DConnectCompletion : public ipcIMessageObserver
{
public:
  DConnectCompletion(PRUint32 requestIndex)
    : mRequestIndex(requestIndex)
  {}

  
  NS_IMETHOD_(nsrefcnt) AddRef() { return 1; }
  NS_IMETHOD_(nsrefcnt) Release() { return 1; }

  NS_IMETHOD QueryInterface(const nsIID &aIID, void **aInstancePtr);

  NS_IMETHOD OnMessageAvailable(PRUint32 aSenderID, const nsID &aTarget,
                                const PRUint8 *aData, PRUint32 aDataLen)
  {
    const DConnectOp *op = (const DConnectOp *) aData;
    if ((aDataLen >= sizeof(DConnectOp)) && (op->request_index == mRequestIndex))
      OnResponseAvailable(aSenderID, op, aDataLen);
    else
      gDConnect->OnMessageAvailable(aSenderID, aTarget, aData, aDataLen);
    return NS_OK;
  }

  virtual void OnResponseAvailable(PRUint32 sender, const DConnectOp *op, PRUint32 opLen) = 0;

protected:
  PRUint32 mRequestIndex;
};
NS_IMPL_QUERY_INTERFACE1(DConnectCompletion, ipcIMessageObserver)



class DConnectInvokeCompletion : public DConnectCompletion
{
public:
  DConnectInvokeCompletion(const DConnectInvoke *invoke)
    : DConnectCompletion(invoke->request_index)
    , mReply(nsnull)
    , mParamsLen(0)
  {}

  ~DConnectInvokeCompletion() { if (mReply) free(mReply); }

  void OnResponseAvailable(PRUint32 sender, const DConnectOp *op, PRUint32 opLen)
  {
    mReply = (DConnectInvokeReply *) malloc(opLen);
    if (!mReply)
      return; 
    memcpy(mReply, op, opLen);

    
    mParamsLen = opLen - sizeof(*mReply);
  }

  PRBool IsPending() const { return mReply == nsnull; }
  nsresult GetResult() const { return mReply->result; }

  const PRUint8 *Params() const { return (const PRUint8 *) (mReply + 1); }
  PRUint32 ParamsLen() const { return mParamsLen; }

  const DConnectInvokeReply *Reply() const { return mReply; }

private:
  DConnectInvokeReply *mReply;
  PRUint32             mParamsLen;
};



#define DCONNECT_STUB_ID                           \
{ /* 132c1f14-5442-49cb-8fe6-e60214bbf1db */       \
  0x132c1f14,                                      \
  0x5442,                                          \
  0x49cb,                                          \
  {0x8f, 0xe6, 0xe6, 0x02, 0x14, 0xbb, 0xf1, 0xdb} \
}
static NS_DEFINE_IID(kDConnectStubID, DCONNECT_STUB_ID);



class DConnectStub : public nsXPTCStubBase
{
public:
  NS_DECL_ISUPPORTS

  DConnectStub(nsIInterfaceInfo *aIInfo,
               DConAddr aInstance,
               PRUint32 aPeerID)
    : mIInfo(aIInfo)
    , mMaster(nsnull)
    , mInstance(aInstance)
    , mPeerID(aPeerID)
    {}

  
  
  NS_IMETHOD GetInterfaceInfo(nsIInterfaceInfo **aInfo);

  
  NS_IMETHOD CallMethod(PRUint16 aMethodIndex,
                        const nsXPTMethodInfo *aInfo,
                        nsXPTCMiniVariant *aParams);

  DConAddr Instance() { return mInstance; }
  PRUint32 PeerID()   { return mPeerID; }

private:
  NS_HIDDEN ~DConnectStub();

  
  NS_HIDDEN_(DConnectStub *) FindStubSupportingIID(const nsID &aIID);

  
  NS_HIDDEN_(PRBool) SupportsIID(const nsID &aIID);

private:
  nsCOMPtr<nsIInterfaceInfo> mIInfo;

  nsVoidArray   mChildren; 
  DConnectStub *mMaster;   

  
  DConAddr mInstance;

  
  PRUint32 mPeerID;
};

static nsresult
CreateStub(const nsID &iid, PRUint32 peer, DConAddr instance, DConnectStub **result)
{
  nsresult rv;

  nsCOMPtr<nsIInterfaceInfo> iinfo;
  rv = gDConnect->GetInterfaceInfo(iid, getter_AddRefs(iinfo));
  if (NS_FAILED(rv))
    return rv;

  DConnectStub *stub = new DConnectStub(iinfo,
                                        instance,
                                        peer);
  if (NS_UNLIKELY(!stub))
    rv = NS_ERROR_OUT_OF_MEMORY;
  else
  {
    NS_ADDREF(stub);
    *result = stub;
  }

  return rv;
}

static nsresult
SerializeInterfaceParam(ipcMessageWriter &writer,
                        PRUint32 peer, const nsID &iid,
                        const nsXPTType &type, const nsXPTCMiniVariant &v,
                        nsVoidArray &wrappers)
{
  
  
  
  
  

  
  
  

  nsISupports *obj = (nsISupports *) v.val.p;
  if (!obj)
  {
    
    writer.PutBytes(&obj, sizeof(obj));
  }
  else
  {
    DConnectStub *stub = nsnull;
    nsresult rv = obj->QueryInterface(kDConnectStubID, (void **) &stub);
    if (NS_SUCCEEDED(rv) && (stub->PeerID() == peer))
    {
      void *p = stub->Instance();
      writer.PutBytes(&p, sizeof(p));
    }
    else
    {
      

      nsCOMPtr<nsIInterfaceInfo> iinfo;
      rv = gDConnect->GetInterfaceInfo(iid, getter_AddRefs(iinfo));
      if (NS_FAILED(rv))
        return rv;

      DConnectInstance *wrapper = nsnull;

      wrapper = new DConnectInstance(peer, iinfo, obj);
      if (!wrapper)
        return NS_ERROR_OUT_OF_MEMORY;

      if (!wrappers.AppendElement(wrapper))
      {
        delete wrapper;
        return NS_ERROR_OUT_OF_MEMORY;
      }

      rv = gDConnect->StoreInstance(wrapper);
      if (NS_FAILED(rv))
      {
        wrappers.RemoveElement(wrapper);
        delete wrapper;
        return rv;
      }

      
      
      PtrBits bits = ((PtrBits) wrapper) | 0x1;
      writer.PutBytes(&bits, sizeof(bits));
    }
    NS_IF_RELEASE(stub);
  }
  return NS_OK;
}

DConnectStub::~DConnectStub()
{
  

  nsresult rv;

  DConnectRelease msg;
  msg.opcode_major = DCON_OP_RELEASE;
  msg.opcode_minor = 0;
  msg.instance = mInstance;

  
  rv = IPC_SendMessage(mPeerID, kDConnectTargetID,
                       (const PRUint8 *) &msg, sizeof(msg));
  if (NS_FAILED(rv))
    NS_WARNING("failed to send RELEASE event");

  if (!mMaster)
  {
    
    for (PRInt32 i=0; i<mChildren.Count(); ++i)
      delete (DConnectStub *) mChildren[i];
  }
}

PRBool
DConnectStub::SupportsIID(const nsID &iid)
{
  PRBool match;
  nsCOMPtr<nsIInterfaceInfo> iter = mIInfo;
  do
  {
    if (NS_SUCCEEDED(iter->IsIID(&iid, &match)) && match)
      return PR_TRUE;

    nsCOMPtr<nsIInterfaceInfo> parent;
    iter->GetParent(getter_AddRefs(parent));
    iter = parent;
  }
  while (iter != nsnull);

  return PR_FALSE;
}

DConnectStub *
DConnectStub::FindStubSupportingIID(const nsID &iid)
{
  NS_ASSERTION(mMaster == nsnull, "this is not a master stub");

  if (SupportsIID(iid))
    return this;

  for (PRInt32 i=0; i<mChildren.Count(); ++i)
  {
    DConnectStub *child = (DConnectStub *) mChildren[i];
    if (child->SupportsIID(iid))
      return child;
  }
  return nsnull;
}

NS_IMETHODIMP_(nsrefcnt)
DConnectStub::AddRef()
{
  if (mMaster)
    return mMaster->AddRef();

  NS_ASSERT_OWNINGTHREAD("DConnectStub");
  ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "DConnectStub", sizeof(*this));
  return mRefCnt;
}

NS_IMETHODIMP_(nsrefcnt)
DConnectStub::Release()
{
  if (mMaster)
    return mMaster->Release();

  NS_ASSERT_OWNINGTHREAD("DConnectStub");
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "DConnectStub");
  if (mRefCnt == 0)
  {
    mRefCnt = 1; 
    delete this;
    return 0;
  }                                                                           \
  return mRefCnt;        
}

NS_IMETHODIMP
DConnectStub::QueryInterface(const nsID &aIID, void **aInstancePtr)
{
  DConnectStub *master = mMaster ? mMaster : this;

  
  if (aIID.Equals(NS_GET_IID(nsISupports)))
  {
    *aInstancePtr = master;
    NS_ADDREF(master);
    return NS_OK;
  }

  
  if (aIID.Equals(kDConnectStubID))
  {
    *aInstancePtr = this;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  
  
  
  
#if 0
  if (SupportsIID(aIID))
  {
    *aInstancePtr = this;
    NS_ADDREF(this);
    return NS_OK;
  }
#endif

  
  DConnectStub *stub = master->FindStubSupportingIID(aIID);
  if (stub)
  {
    *aInstancePtr = stub;
    NS_ADDREF(stub);
    return NS_OK;
  }

  
  LOG(("calling QueryInterface on peer object\n"));

  DConnectSetupQueryInterface msg;
  msg.opcode_minor = DCON_OP_SETUP_QUERY_INTERFACE;
  msg.iid = aIID;
  msg.instance = mInstance;

  void *result;

  nsresult rv = SetupPeerInstance(mPeerID, &msg, sizeof(msg), &result);
  if (NS_FAILED(rv))
    return rv;

  stub = (DConnectStub *) result;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  stub->mMaster = master;
  master->mChildren.AppendElement(stub);

  *aInstancePtr = stub;
  NS_ADDREF(stub);
  return NS_OK;
}

NS_IMETHODIMP
DConnectStub::GetInterfaceInfo(nsIInterfaceInfo **aInfo)
{
  NS_ADDREF(*aInfo = mIInfo);
  return NS_OK;
}

NS_IMETHODIMP
DConnectStub::CallMethod(PRUint16 aMethodIndex,
                         const nsXPTMethodInfo *aInfo,
                         nsXPTCMiniVariant *aParams)
{
  nsresult rv;

  LOG(("DConnectStub::CallMethod [methodIndex=%hu]\n", aMethodIndex));

  

  PRUint8 i, paramCount = aInfo->GetParamCount();

  LOG(("  name=%s\n", aInfo->GetName()));
  LOG(("  param-count=%u\n", (PRUint32) paramCount));

  ipcMessageWriter writer(16 * paramCount);

  
  DConnectInvoke invoke;
  invoke.opcode_major = DCON_OP_INVOKE;
  invoke.opcode_minor = 0;
  invoke.request_index = NewRequestIndex();
  invoke.instance = mInstance;
  invoke.method_index = aMethodIndex;

  writer.PutBytes(&invoke, sizeof(invoke));

  
  
  nsVoidArray wrappers;

  for (i=0; i<paramCount; ++i)
  {
    const nsXPTParamInfo &paramInfo = aInfo->GetParam(i);

    if (paramInfo.IsIn() && !paramInfo.IsDipper())
    {
      const nsXPTType &type = paramInfo.GetType();

      if (type.IsInterfacePointer())
      {
        nsID iid;
        rv = gDConnect->GetIIDForMethodParam(mIInfo, aInfo, paramInfo, type,
                                             aMethodIndex, i, aParams, PR_FALSE, iid);
        if (NS_SUCCEEDED(rv))
          rv = SerializeInterfaceParam(writer, mPeerID, iid, type, aParams[i], wrappers);
      }
      else
        rv = SerializeParam(writer, type, aParams[i]);

      if (NS_FAILED(rv))
        return rv;
    }
  }

  rv = IPC_SendMessage(mPeerID, kDConnectTargetID,
                       writer.GetBuffer(),
                       writer.GetSize());
  if (NS_FAILED(rv))
  {
    
    DeleteWrappers(wrappers);
    return rv;
  }

  
  
  
  
  

  DConnectInvokeCompletion completion(&invoke);

  do
  {
    rv = IPC_WaitMessage(mPeerID, kDConnectTargetID, &completion,
                         DCON_WAIT_TIMEOUT);
    if (NS_FAILED(rv))
    {
      
      DeleteWrappers(wrappers);
      return rv;
    }
  }
  while (completion.IsPending());

  rv = completion.GetResult();
  if (NS_SUCCEEDED(rv) && completion.ParamsLen() > 0)
  {
    ipcMessageReader reader(completion.Params(), completion.ParamsLen());

    PRUint8 i;

    
    for (i=0; i<paramCount; ++i)
    {
      const nsXPTParamInfo &paramInfo = aInfo->GetParam(i);

      if (paramInfo.IsOut() || paramInfo.IsRetval())
        DeserializeResult(reader, paramInfo.GetType(), aParams[i]);
    }

    
    
    for (i=0; i<paramCount; ++i)
    {
      const nsXPTParamInfo &paramInfo = aInfo->GetParam(i);
      if (aParams[i].val.p && (paramInfo.IsOut() || paramInfo.IsRetval()))
      {
        const nsXPTType &type = paramInfo.GetType();
        if (type.IsInterfacePointer())
        {
          PtrBits bits = (PtrBits) *((void **) aParams[i].val.p);
          if (bits & 0x1)
          {
            *((void **) aParams[i].val.p) = (void *) (bits & ~0x1);

            nsID iid;
            rv = gDConnect->GetIIDForMethodParam(mIInfo, aInfo, paramInfo, type,
                                                 aMethodIndex, i, aParams, PR_FALSE, iid);
            if (NS_SUCCEEDED(rv))
            {
              DConnectStub *stub;
              void **pptr = (void **) aParams[i].val.p;
              rv = CreateStub(iid, mPeerID, (DConAddr) *pptr, &stub);
              if (NS_SUCCEEDED(rv))
                *((nsISupports **) aParams[i].val.p) = stub;
            }
          }
          else if (bits)
          {
            

            DConnectInstance *wrapper = (DConnectInstance *) aParams[i].val.p;
            *((void **) aParams[i].val.p) = wrapper->RealInstance();
          }
          else
          {
            *((void **) aParams[i].val.p) = nsnull;
          }
        }
      }
    }
  }

  return rv;
}



class DConnectSetupCompletion : public DConnectCompletion
{
public:
  DConnectSetupCompletion(const DConnectSetup *setup)
    : DConnectCompletion(setup->request_index)
    , mSetup(setup)
    , mStatus(NS_OK)
  {}

  void OnResponseAvailable(PRUint32 sender, const DConnectOp *op, PRUint32 opLen)
  {
    if (op->opcode_major != DCON_OP_SETUP_REPLY)
    {
      NS_NOTREACHED("unexpected response");
      mStatus = NS_ERROR_UNEXPECTED;
      return;
    }

    const DConnectSetupReply *reply = (const DConnectSetupReply *) op;

    LOG(("got SETUP_REPLY: status=%x instance=%p\n", reply->status, reply->instance));

    if (NS_FAILED(reply->status))
    {
      NS_ASSERTION(!reply->instance, "non-null instance on failure");
      mStatus = reply->status;
    }
    else
    {
      nsresult rv = CreateStub(mSetup->iid, sender, reply->instance,
                               getter_AddRefs(mStub));
      if (NS_FAILED(rv))
        mStatus = rv;
    }
  }

  nsresult GetStub(void **aInstancePtr)
  {
    if (NS_FAILED(mStatus))
      return mStatus;

    DConnectStub *stub = mStub;
    NS_IF_ADDREF(stub);
    *aInstancePtr = stub;
    return NS_OK;
  }

private:
  const DConnectSetup    *mSetup;
  nsresult                mStatus;
  nsRefPtr<DConnectStub>  mStub;
};


nsresult
SetupPeerInstance(PRUint32 aPeerID, DConnectSetup *aMsg, PRUint32 aMsgLen,
                  void **aInstancePtr)
{
  *aInstancePtr = nsnull;

  aMsg->opcode_major = DCON_OP_SETUP;
  aMsg->request_index = NewRequestIndex();

  

  nsresult rv = IPC_SendMessage(aPeerID, kDConnectTargetID,
                                (const PRUint8 *) aMsg, aMsgLen);
  if (NS_FAILED(rv))
    return rv;

  DConnectSetupCompletion completion(aMsg);

  
  
  
  

  do
  {
    rv = IPC_WaitMessage(aPeerID, kDConnectTargetID, &completion, DCON_WAIT_TIMEOUT);
    if (NS_FAILED(rv))
      break;

    rv = completion.GetStub(aInstancePtr);
  }
  while (NS_SUCCEEDED(rv) && *aInstancePtr == nsnull);

  return rv;
} 



PR_STATIC_CALLBACK(PLDHashOperator)
DestroyDConnectInstance(const void *key,
                        DConnectInstance *wrapper,
                        void *userArg)
{
  delete wrapper;
  return PL_DHASH_NEXT;
}



ipcDConnectService::~ipcDConnectService()
{
  
  mInstances.EnumerateRead(DestroyDConnectInstance, nsnull);
  mInstances.Clear();

  gDConnect = nsnull;
}

nsresult
ipcDConnectService::Init()
{
  nsresult rv;

  rv = IPC_DefineTarget(kDConnectTargetID, this);
  if (NS_FAILED(rv))
    return rv;

  if (!mInstances.Init())
    return NS_ERROR_OUT_OF_MEMORY;

  mIIM = do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  gDConnect = this;
  return NS_OK;
}


nsresult
ipcDConnectService::GetInterfaceInfo(const nsID &iid, nsIInterfaceInfo **result)
{
  return mIIM->GetInfoForIID(&iid, result);
}


nsresult
ipcDConnectService::GetIIDForMethodParam(nsIInterfaceInfo *iinfo,
                                         const nsXPTMethodInfo *methodInfo,
                                         const nsXPTParamInfo &paramInfo,
                                         const nsXPTType &type,
                                         PRUint16 methodIndex,
                                         PRUint8 paramIndex,
                                         nsXPTCMiniVariant *dispatchParams,
                                         PRBool isFullVariantArray,
                                         nsID &result)
{
  PRUint8 argnum, tag = type.TagPart();
  nsresult rv;

  if (tag == nsXPTType::T_INTERFACE)
  {
    rv = iinfo->GetIIDForParamNoAlloc(methodIndex, &paramInfo, &result);
  }
  else if (tag == nsXPTType::T_INTERFACE_IS)
  {
    rv = iinfo->GetInterfaceIsArgNumberForParam(methodIndex, &paramInfo, &argnum);
    if (NS_FAILED(rv))
      return rv;

    const nsXPTParamInfo& arg_param = methodInfo->GetParam(argnum);
    const nsXPTType& arg_type = arg_param.GetType();

    
    if (!arg_type.IsPointer() || arg_type.TagPart() != nsXPTType::T_IID)
      return NS_ERROR_UNEXPECTED;

    nsID *p;
    if (isFullVariantArray)
      p = (nsID *) ((nsXPTCVariant *) dispatchParams)[argnum].val.p;
    else
      p = (nsID *) dispatchParams[argnum].val.p;
    if (!p)
      return NS_ERROR_UNEXPECTED;

    result = *p;
  }
  else
    rv = NS_ERROR_UNEXPECTED;
  return rv;
}

nsresult
ipcDConnectService::StoreInstance(DConnectInstance *wrapper)
{
  return mInstances.Put(nsVoidPtrHashKey(wrapper).GetKey(), wrapper)
      ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
ipcDConnectService::DeleteInstance(DConnectInstance *wrapper)
{
  
  mInstances.Remove(nsVoidPtrHashKey(wrapper).GetKey());
}

NS_IMPL_ISUPPORTS2(ipcDConnectService, ipcIDConnectService, ipcIMessageObserver)

NS_IMETHODIMP
ipcDConnectService::CreateInstance(PRUint32 aPeerID,
                                   const nsID &aCID,
                                   const nsID &aIID,
                                   void **aInstancePtr)
{
  DConnectSetupClassID msg;
  msg.opcode_minor = DCON_OP_SETUP_NEW_INST_CLASSID;
  msg.iid = aIID;
  msg.classid = aCID;

  return SetupPeerInstance(aPeerID, &msg, sizeof(msg), aInstancePtr);
}

NS_IMETHODIMP
ipcDConnectService::CreateInstanceByContractID(PRUint32 aPeerID,
                                               const char *aContractID,
                                               const nsID &aIID,
                                               void **aInstancePtr)
{
  size_t slen = strlen(aContractID);
  size_t size = sizeof(DConnectSetupContractID) + slen;

  DConnectSetupContractID *msg =
      (DConnectSetupContractID *) malloc(size);
  NS_ENSURE_TRUE(msg, NS_ERROR_OUT_OF_MEMORY);

  msg->opcode_minor = DCON_OP_SETUP_NEW_INST_CONTRACTID;
  msg->iid = aIID;
  memcpy(&msg->contractid, aContractID, slen + 1);

  nsresult rv = SetupPeerInstance(aPeerID, msg, size, aInstancePtr);

  free(msg);
  return rv;
}

NS_IMETHODIMP
ipcDConnectService::GetService(PRUint32 aPeerID,
                               const nsID &aCID,
                               const nsID &aIID,
                               void **aInstancePtr)
{
  DConnectSetupClassID msg;
  msg.opcode_minor = DCON_OP_SETUP_GET_SERV_CLASSID;
  msg.iid = aIID;
  msg.classid = aCID;

  return SetupPeerInstance(aPeerID, &msg, sizeof(msg), aInstancePtr);
}

NS_IMETHODIMP
ipcDConnectService::GetServiceByContractID(PRUint32 aPeerID,
                                           const char *aContractID,
                                           const nsID &aIID,
                                           void **aInstancePtr)
{
  size_t slen = strlen(aContractID);
  size_t size = sizeof(DConnectSetupContractID) + slen;

  DConnectSetupContractID *msg =
      (DConnectSetupContractID *) malloc(size);
  NS_ENSURE_TRUE(msg, NS_ERROR_OUT_OF_MEMORY);

  msg->opcode_minor = DCON_OP_SETUP_GET_SERV_CONTRACTID;
  msg->iid = aIID;
  memcpy(&msg->contractid, aContractID, slen + 1);

  nsresult rv = SetupPeerInstance(aPeerID, msg, size, aInstancePtr);

  free(msg);
  return rv;
}



NS_IMETHODIMP
ipcDConnectService::OnMessageAvailable(PRUint32 aSenderID,
                                       const nsID &aTarget,
                                       const PRUint8 *aData,
                                       PRUint32 aDataLen)
{
  const DConnectOp *op = (const DConnectOp *) aData;
  switch (op->opcode_major)
  {
    case DCON_OP_SETUP:
      OnSetup(aSenderID, (const DConnectSetup *) aData, aDataLen);
      break;
    case DCON_OP_RELEASE:
      OnRelease(aSenderID, (const DConnectRelease *) aData);
      break;
    case DCON_OP_INVOKE:
      OnInvoke(aSenderID, (const DConnectInvoke *) aData, aDataLen);
      break;
    default:
      NS_NOTREACHED("unknown opcode major");
  }

  return NS_OK;
}



void
ipcDConnectService::OnSetup(PRUint32 peer, const DConnectSetup *setup, PRUint32 opLen)
{
  nsISupports *instance = nsnull;
  nsresult rv = NS_ERROR_FAILURE;

  switch (setup->opcode_minor)
  {
    
    case DCON_OP_SETUP_NEW_INST_CLASSID:
    {
      const DConnectSetupClassID *setupCI = (const DConnectSetupClassID *) setup;

      nsCOMPtr<nsIComponentManager> compMgr;
      rv = NS_GetComponentManager(getter_AddRefs(compMgr));
      if (NS_SUCCEEDED(rv))
        rv = compMgr->CreateInstance(setupCI->classid, nsnull, setupCI->iid, (void **) &instance);

      break;
    }

    
    case DCON_OP_SETUP_NEW_INST_CONTRACTID:
    {
      const DConnectSetupContractID *setupCI = (const DConnectSetupContractID *) setup;

      nsCOMPtr<nsIComponentManager> compMgr;
      rv = NS_GetComponentManager(getter_AddRefs(compMgr));
      if (NS_SUCCEEDED(rv))
        rv = compMgr->CreateInstanceByContractID(setupCI->contractid, nsnull, setupCI->iid, (void **) &instance);

      break;
    }

    
    case DCON_OP_SETUP_GET_SERV_CLASSID:
    {
      const DConnectSetupClassID *setupCI = (const DConnectSetupClassID *) setup;

      nsCOMPtr<nsIServiceManager> svcMgr;
      rv = NS_GetServiceManager(getter_AddRefs(svcMgr));
      if (NS_SUCCEEDED(rv))
        rv = svcMgr->GetService(setupCI->classid, setupCI->iid, (void **) &instance);
      break;
    }

    
    case DCON_OP_SETUP_GET_SERV_CONTRACTID:
    {
      const DConnectSetupContractID *setupCI = (const DConnectSetupContractID *) setup;

      nsCOMPtr<nsIServiceManager> svcMgr;
      rv = NS_GetServiceManager(getter_AddRefs(svcMgr));
      if (NS_SUCCEEDED(rv))
        rv = svcMgr->GetServiceByContractID(setupCI->contractid, setupCI->iid, (void **) &instance);

      break;
    }

    
    case DCON_OP_SETUP_QUERY_INTERFACE:
    {
      const DConnectSetupQueryInterface *setupQI = (const DConnectSetupQueryInterface *) setup;
  
      
      if (!mInstances.Get(nsVoidPtrHashKey(setupQI->instance).GetKey(), nsnull))
      {
        NS_NOTREACHED("instance wrapper not found");
        rv = NS_ERROR_INVALID_ARG;
      }
      else
        rv = setupQI->instance->RealInstance()->QueryInterface(setupQI->iid, (void **) &instance);
      break;
    }

    default:
      NS_NOTREACHED("unexpected minor opcode");
      rv = NS_ERROR_UNEXPECTED;
      break;
  }

  
  
  
  
  DConnectInstance *wrapper = nsnull;
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIInterfaceInfo> iinfo;
    rv = gDConnect->GetInterfaceInfo(setup->iid, getter_AddRefs(iinfo));
    if (NS_SUCCEEDED(rv))
    {
      wrapper = new DConnectInstance(peer, iinfo, instance);
      if (!wrapper)
        rv = NS_ERROR_OUT_OF_MEMORY;
      else
        rv = StoreInstance(wrapper);
    }
  }

  if (NS_FAILED(rv) && instance)
    NS_RELEASE(instance);

  DConnectSetupReply msg;
  msg.opcode_major = DCON_OP_SETUP_REPLY;
  msg.opcode_minor = 0;
  msg.request_index = setup->request_index;
  msg.instance = wrapper;
  msg.status = rv;

  
  IPC_SendMessage(peer, kDConnectTargetID,
                  (const PRUint8 *) &msg, sizeof(msg));
}

void
ipcDConnectService::OnRelease(PRUint32 peer, const DConnectRelease *release)
{
  LOG(("ipcDConnectService::OnRelease [peer=%u instance=%p]\n", peer, release->instance));

#ifdef DEBUG
  
  if (!mInstances.Get(nsVoidPtrHashKey(release->instance).GetKey(), nsnull))
    NS_NOTREACHED("instance wrapper not found");
#endif

  DeleteInstance(release->instance);
}

void
ipcDConnectService::OnInvoke(PRUint32 peer, const DConnectInvoke *invoke, PRUint32 opLen)
{
  LOG(("ipcDConnectService::OnInvoke [peer=%u instance=%p method=%u]\n",
      peer, invoke->instance, invoke->method_index));

  DConnectInstance *wrapper = invoke->instance;

  ipcMessageReader reader((const PRUint8 *) (invoke + 1), opLen - sizeof(*invoke));

  const nsXPTMethodInfo *methodInfo;
  nsXPTCVariant *params = nsnull;
  nsIInterfaceInfo *iinfo = nsnull;
  PRUint8 i, paramCount = 0, paramUsed = 0;
  nsresult rv;
  
  
  if (!mInstances.Get(nsVoidPtrHashKey(wrapper).GetKey(), nsnull))
  {
    NS_NOTREACHED("instance wrapper not found");
    rv = NS_ERROR_INVALID_ARG;
    goto end;
  }

  iinfo = wrapper->InterfaceInfo();

  rv = iinfo->GetMethodInfo(invoke->method_index, &methodInfo);
  if (NS_FAILED(rv))
    goto end;

  paramCount = methodInfo->GetParamCount();

  params = new nsXPTCVariant[paramCount];
  if (!params)
    goto end;

  

  for (i=0; i<paramCount; ++i, ++paramUsed)
  {
    const nsXPTParamInfo &paramInfo = methodInfo->GetParam(i);

    

    if (paramInfo.IsIn() && !paramInfo.IsDipper())
      rv = DeserializeParam(reader, paramInfo.GetType(), params[i]);
    else
      rv = SetupParam(paramInfo, params[i]);

    if (NS_FAILED(rv))
      goto end;
  }

  
  
  for (i=0; i<paramCount; ++i)
  {
    const nsXPTParamInfo &paramInfo = methodInfo->GetParam(i);
    const nsXPTType &type = paramInfo.GetType();

    if (paramInfo.IsIn() && type.IsInterfacePointer())
    {
      PtrBits bits = (PtrBits) params[i].ptr;
      if (bits & 0x1)
      {
        
        params[i].ptr = (void *) (bits & ~0x1);

        nsID iid;
        rv = GetIIDForMethodParam(iinfo, methodInfo, paramInfo, type,
                                  invoke->method_index, i, params, PR_TRUE, iid);
        if (NS_SUCCEEDED(rv))
        {
          DConnectStub *stub;
          rv = CreateStub(iid, peer, (DConAddr) params[i].ptr, &stub);
          if (NS_SUCCEEDED(rv))
          {
            params[i].val.p = params[i].ptr = stub;
            params[i].SetValIsInterface();
          }
        }
      }
      else if (bits)
      {
        

        DConnectInstance *wrapper = (DConnectInstance *) params[i].ptr;
        params[i].val.p = params[i].ptr = wrapper->RealInstance();
        params[i].SetValIsInterface();
      }
      else
      {
        params[i].val.p = params[i].ptr = nsnull;
        params[i].SetValIsInterface();
      }
    }
  }

  rv = NS_InvokeByIndex(wrapper->RealInstance(),
                        invoke->method_index, 
                        paramCount,
                        params);

end:
  LOG(("sending INVOKE_REPLY: rv=%x\n", rv));

  ipcMessageWriter writer(64);

  DConnectInvokeReply reply;
  reply.opcode_major = DCON_OP_INVOKE_REPLY;
  reply.opcode_minor = 0;
  reply.request_index = invoke->request_index;
  reply.result = rv;

  writer.PutBytes(&reply, sizeof(reply));

  nsVoidArray wrappers;

  if (NS_SUCCEEDED(rv) && params)
  {
    
    for (i=0; i<paramCount; ++i)
    {
      const nsXPTParamInfo paramInfo = methodInfo->GetParam(i);

      if (paramInfo.IsRetval() || paramInfo.IsOut())
      {
        const nsXPTType &type = paramInfo.GetType();

        if (type.IsInterfacePointer())
        {
          nsID iid;
          rv = GetIIDForMethodParam(iinfo, methodInfo, paramInfo, type,
                                    invoke->method_index, i, params, PR_TRUE, iid);
          if (NS_SUCCEEDED(rv))
            rv = SerializeInterfaceParam(writer, peer, iid, type, params[i], wrappers);
        }
        else
          rv = SerializeParam(writer, type, params[i]);

        if (NS_FAILED(rv))
        {
          reply.result = rv;
          break;
        }
      }
    }
  }

  if (NS_FAILED(rv))
    rv = IPC_SendMessage(peer, kDConnectTargetID, (const PRUint8 *) &reply, sizeof(reply));
  else
    rv = IPC_SendMessage(peer, kDConnectTargetID, writer.GetBuffer(), writer.GetSize());
  if (NS_FAILED(rv))
  {
    LOG(("unable to send INVOKE_REPLY: rv=%x\n", rv));
    DeleteWrappers(wrappers);
  }

  if (params)
  {
    for (i=0; i<paramUsed; ++i)
      FinishParam(params[i]);
    delete[] params;
  }
}
