




#include "prlog.h"

#include <stdlib.h>
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsNSSShutDown.h"
#include "nsNTLMAuthModule.h"
#include "nsNativeCharsetUtils.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "prsystem.h"
#include "nss.h"
#include "pk11func.h"
#include "md4.h"

#ifdef PR_LOGGING
PRLogModuleInfo *gNTLMLog = PR_NewLogModule("NTLM");

#define LOG(x) PR_LOG(gNTLMLog, PR_LOG_DEBUG, x)
#define LOG_ENABLED() PR_LOG_TEST(gNTLMLog, PR_LOG_DEBUG)
#else
#define LOG(x)
#endif

static void des_makekey(const uint8_t *raw, uint8_t *key);
static void des_encrypt(const uint8_t *key, const uint8_t *src, uint8_t *hash);
static void md5sum(const uint8_t *input, uint32_t inputLen, uint8_t *result);






#define NTLM_NegotiateUnicode               0x00000001
#define NTLM_NegotiateOEM                   0x00000002
#define NTLM_RequestTarget                  0x00000004
#define NTLM_Unknown1                       0x00000008
#define NTLM_NegotiateSign                  0x00000010
#define NTLM_NegotiateSeal                  0x00000020
#define NTLM_NegotiateDatagramStyle         0x00000040
#define NTLM_NegotiateLanManagerKey         0x00000080
#define NTLM_NegotiateNetware               0x00000100
#define NTLM_NegotiateNTLMKey               0x00000200
#define NTLM_Unknown2                       0x00000400
#define NTLM_Unknown3                       0x00000800
#define NTLM_NegotiateDomainSupplied        0x00001000
#define NTLM_NegotiateWorkstationSupplied   0x00002000
#define NTLM_NegotiateLocalCall             0x00004000
#define NTLM_NegotiateAlwaysSign            0x00008000
#define NTLM_TargetTypeDomain               0x00010000
#define NTLM_TargetTypeServer               0x00020000
#define NTLM_TargetTypeShare                0x00040000
#define NTLM_NegotiateNTLM2Key              0x00080000
#define NTLM_RequestInitResponse            0x00100000
#define NTLM_RequestAcceptResponse          0x00200000
#define NTLM_RequestNonNTSessionKey         0x00400000
#define NTLM_NegotiateTargetInfo            0x00800000
#define NTLM_Unknown4                       0x01000000
#define NTLM_Unknown5                       0x02000000
#define NTLM_Unknown6                       0x04000000
#define NTLM_Unknown7                       0x08000000
#define NTLM_Unknown8                       0x10000000
#define NTLM_Negotiate128                   0x20000000
#define NTLM_NegotiateKeyExchange           0x40000000
#define NTLM_Negotiate56                    0x80000000


#define NTLM_TYPE1_FLAGS      \
  (NTLM_NegotiateUnicode |    \
   NTLM_NegotiateOEM |        \
   NTLM_RequestTarget |       \
   NTLM_NegotiateNTLMKey |    \
   NTLM_NegotiateAlwaysSign | \
   NTLM_NegotiateNTLM2Key)

static const char NTLM_SIGNATURE[] = "NTLMSSP";
static const char NTLM_TYPE1_MARKER[] = { 0x01, 0x00, 0x00, 0x00 };
static const char NTLM_TYPE2_MARKER[] = { 0x02, 0x00, 0x00, 0x00 };
static const char NTLM_TYPE3_MARKER[] = { 0x03, 0x00, 0x00, 0x00 };

#define NTLM_TYPE1_HEADER_LEN 32
#define NTLM_TYPE2_HEADER_LEN 32
#define NTLM_TYPE3_HEADER_LEN 64

#define LM_HASH_LEN 16
#define LM_RESP_LEN 24

#define NTLM_HASH_LEN 16
#define NTLM_RESP_LEN 24



static bool SendLM()
{
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (!prefs)
    return false;

  bool val;
  nsresult rv = prefs->GetBoolPref("network.ntlm.send-lm-response", &val);
  return NS_SUCCEEDED(rv) && val;
}



#ifdef PR_LOGGING




static void LogFlags(uint32_t flags)
{
  if (!LOG_ENABLED())
    return;
#define TEST(_flag) \
  if (flags & NTLM_ ## _flag) \
    PR_LogPrint("    0x%08x (" # _flag ")\n", NTLM_ ## _flag)

  TEST(NegotiateUnicode);
  TEST(NegotiateOEM);
  TEST(RequestTarget);
  TEST(Unknown1);
  TEST(NegotiateSign);
  TEST(NegotiateSeal);
  TEST(NegotiateDatagramStyle);
  TEST(NegotiateLanManagerKey);
  TEST(NegotiateNetware);
  TEST(NegotiateNTLMKey);
  TEST(Unknown2);
  TEST(Unknown3);
  TEST(NegotiateDomainSupplied);
  TEST(NegotiateWorkstationSupplied);
  TEST(NegotiateLocalCall);
  TEST(NegotiateAlwaysSign);
  TEST(TargetTypeDomain);
  TEST(TargetTypeServer);
  TEST(TargetTypeShare);
  TEST(NegotiateNTLM2Key);
  TEST(RequestInitResponse);
  TEST(RequestAcceptResponse);
  TEST(RequestNonNTSessionKey);
  TEST(NegotiateTargetInfo);
  TEST(Unknown4);
  TEST(Unknown5);
  TEST(Unknown6);
  TEST(Unknown7);
  TEST(Unknown8);
  TEST(Negotiate128);
  TEST(NegotiateKeyExchange);
  TEST(Negotiate56);

#undef TEST
}







static void
LogBuf(const char *tag, const uint8_t *buf, uint32_t bufLen)
{
  int i;

  if (!LOG_ENABLED())
    return;

  PR_LogPrint("%s =\n", tag);
  char line[80];
  while (bufLen > 0)
  {
    int count = bufLen;
    if (count > 8)
      count = 8;

    strcpy(line, "    ");
    for (i=0; i<count; ++i)
    {
      int len = strlen(line);
      PR_snprintf(line + len, sizeof(line) - len, "0x%02x ", int(buf[i]));
    }
    for (; i<8; ++i)
    {
      int len = strlen(line);
      PR_snprintf(line + len, sizeof(line) - len, "     ");
    }

    int len = strlen(line);
    PR_snprintf(line + len, sizeof(line) - len, "   ");
    for (i=0; i<count; ++i)
    {
      len = strlen(line);
      if (isprint(buf[i]))
        PR_snprintf(line + len, sizeof(line) - len, "%c", buf[i]);
      else
        PR_snprintf(line + len, sizeof(line) - len, ".");
    }
    PR_LogPrint("%s\n", line);

    bufLen -= count;
    buf += count;
  }
}

#include "plbase64.h"
#include "prmem.h"






static void LogToken(const char *name, const void *token, uint32_t tokenLen)
{
  if (!LOG_ENABLED())
    return;

  char *b64data = PL_Base64Encode((const char *) token, tokenLen, NULL);
  if (b64data)
  {
    PR_LogPrint("%s: %s\n", name, b64data);
    PR_Free(b64data);
  }
}

#else
#define LogFlags(x)
#define LogBuf(a,b,c)
#define LogToken(a,b,c)

#endif 




#define SWAP16(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))
#define SWAP32(x) ((SWAP16((x) & 0xffff) << 16) | (SWAP16((x) >> 16)))

static void *
WriteBytes(void *buf, const void *data, uint32_t dataLen)
{
  memcpy(buf, data, dataLen);
  return (uint8_t *) buf + dataLen;
}

static void *
WriteDWORD(void *buf, uint32_t dword)
{
#ifdef IS_BIG_ENDIAN 
  
  dword = SWAP32(dword);
#endif
  return WriteBytes(buf, &dword, sizeof(dword));
}

static void *
WriteSecBuf(void *buf, uint16_t length, uint32_t offset)
{
#ifdef IS_BIG_ENDIAN
  length = SWAP16(length);
  offset = SWAP32(offset);
#endif
  buf = WriteBytes(buf, &length, sizeof(length));
  buf = WriteBytes(buf, &length, sizeof(length));
  buf = WriteBytes(buf, &offset, sizeof(offset));
  return buf;
}

#ifdef IS_BIG_ENDIAN







static void *
WriteUnicodeLE(void *buf, const PRUnichar *str, uint32_t strLen)
{
  
  uint8_t *cursor = (uint8_t *) buf,
          *input  = (uint8_t *) str;
  for (uint32_t i=0; i<strLen; ++i, input+=2, cursor+=2)
  {
    
    uint8_t temp = input[0];
    cursor[0] = input[1];
    cursor[1] = temp;
  }
  return buf;
}
#endif

static uint16_t
ReadUint16(const uint8_t *&buf)
{
  uint16_t x = ((uint16_t) buf[0]) | ((uint16_t) buf[1] << 8);
  buf += sizeof(x);
  return x;
}

static uint32_t
ReadUint32(const uint8_t *&buf)
{
  uint32_t x = ( (uint32_t) buf[0])        |
               (((uint32_t) buf[1]) << 8)  |
               (((uint32_t) buf[2]) << 16) |
               (((uint32_t) buf[3]) << 24);
  buf += sizeof(x);
  return x;
}



static void
ZapBuf(void *buf, size_t bufLen)
{
  memset(buf, 0, bufLen);
}

static void
ZapString(nsCString &s)
{
  ZapBuf(s.BeginWriting(), s.Length());
}

static void
ZapString(nsString &s)
{
  ZapBuf(s.BeginWriting(), s.Length() * 2);
}

static const unsigned char LM_MAGIC[] = "KGS!@#$%";









static void
LM_Hash(const nsString &password, unsigned char *hash)
{
  
  
  nsCAutoString passbuf;
  NS_CopyUnicodeToNative(password, passbuf);
  ToUpperCase(passbuf);
  uint32_t n = passbuf.Length();
  passbuf.SetLength(14);
  for (uint32_t i=n; i<14; ++i)
    passbuf.SetCharAt('\0', i);

  unsigned char k1[8], k2[8];
  des_makekey((const unsigned char *) passbuf.get()    , k1);
  des_makekey((const unsigned char *) passbuf.get() + 7, k2);
  ZapString(passbuf);

  
  des_encrypt(k1, LM_MAGIC, hash);
  des_encrypt(k2, LM_MAGIC, hash + 8);
}









static void
NTLM_Hash(const nsString &password, unsigned char *hash)
{
  uint32_t len = password.Length();
  uint8_t *passbuf;
  
#ifdef IS_BIG_ENDIAN
  passbuf = (uint8_t *) malloc(len * 2);
  WriteUnicodeLE(passbuf, password.get(), len);
#else
  passbuf = (uint8_t *) password.get();
#endif

  md4sum(passbuf, len * 2, hash);

#ifdef IS_BIG_ENDIAN
  ZapBuf(passbuf, len * 2);
  free(passbuf);
#endif
}














static void
LM_Response(const uint8_t *hash, const uint8_t *challenge, uint8_t *response)
{
  uint8_t keybytes[21], k1[8], k2[8], k3[8];

  memcpy(keybytes, hash, 16);
  ZapBuf(keybytes + 16, 5);

  des_makekey(keybytes     , k1);
  des_makekey(keybytes +  7, k2);
  des_makekey(keybytes + 14, k3);

  des_encrypt(k1, challenge, response);
  des_encrypt(k2, challenge, response + 8);
  des_encrypt(k3, challenge, response + 16);
}



static nsresult
GenerateType1Msg(void **outBuf, uint32_t *outLen)
{
  
  
  
  *outLen = NTLM_TYPE1_HEADER_LEN;
  *outBuf = nsMemory::Alloc(*outLen);
  if (!*outBuf)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  void *cursor = *outBuf;

  
  cursor = WriteBytes(cursor, NTLM_SIGNATURE, sizeof(NTLM_SIGNATURE));

  
  cursor = WriteBytes(cursor, NTLM_TYPE1_MARKER, sizeof(NTLM_TYPE1_MARKER));

  
  cursor = WriteDWORD(cursor, NTLM_TYPE1_FLAGS);

  
  
  
  
  
  
  

  
  cursor = WriteSecBuf(cursor, 0, 0);

  
  cursor = WriteSecBuf(cursor, 0, 0);

  return NS_OK;
}

struct Type2Msg
{
  uint32_t    flags;         
  uint8_t     challenge[8];  
  const void *target;        
  uint32_t    targetLen;     
};

static nsresult
ParseType2Msg(const void *inBuf, uint32_t inLen, Type2Msg *msg)
{
  
  
  
  
  
  
  
  
  
  if (inLen < NTLM_TYPE2_HEADER_LEN)
    return NS_ERROR_UNEXPECTED;

  const uint8_t *cursor = (const uint8_t *) inBuf;

  
  if (memcmp(cursor, NTLM_SIGNATURE, sizeof(NTLM_SIGNATURE)) != 0)
    return NS_ERROR_UNEXPECTED;

  cursor += sizeof(NTLM_SIGNATURE);

  
  if (memcmp(cursor, NTLM_TYPE2_MARKER, sizeof(NTLM_TYPE2_MARKER)) != 0)
    return NS_ERROR_UNEXPECTED;

  cursor += sizeof(NTLM_TYPE2_MARKER);

  
  
  uint32_t targetLen = ReadUint16(cursor);
  
  ReadUint16(cursor);
  
  uint32_t offset = ReadUint32(cursor);
  
  
  if (NS_LIKELY(offset < offset + targetLen && offset + targetLen <= inLen)) {
    msg->targetLen = targetLen;
    msg->target = ((const uint8_t *) inBuf) + offset;
  }
  else
  {
    
    msg->targetLen = 0;
    msg->target = NULL;
  }

  
  msg->flags = ReadUint32(cursor);

  
  memcpy(msg->challenge, cursor, sizeof(msg->challenge));
  cursor += sizeof(msg->challenge);


  LOG(("NTLM type 2 message:\n"));
  LogBuf("target", (const uint8_t *) msg->target, msg->targetLen);
  LogBuf("flags", (const uint8_t *) &msg->flags, 4);
  LogFlags(msg->flags);
  LogBuf("challenge", msg->challenge, sizeof(msg->challenge));

  
  
  
  return NS_OK;
}

static nsresult
GenerateType3Msg(const nsString &domain,
                 const nsString &username,
                 const nsString &password,
                 const void     *inBuf,
                 uint32_t        inLen,
                 void          **outBuf,
                 uint32_t       *outLen)
{
  

  nsresult rv;
  Type2Msg msg;

  rv = ParseType2Msg(inBuf, inLen, &msg);
  if (NS_FAILED(rv))
    return rv;

  bool unicode = (msg.flags & NTLM_NegotiateUnicode);

  
#ifdef IS_BIG_ENDIAN
  nsAutoString ucsDomainBuf, ucsUserBuf;
#endif
  nsAutoString ucsHostBuf; 
  
  nsCAutoString oemDomainBuf, oemUserBuf, oemHostBuf;
  
  
  const void *domainPtr, *userPtr, *hostPtr;
  uint32_t domainLen, userLen, hostLen;

  
  
  
  if (unicode)
  {
#ifdef IS_BIG_ENDIAN
    ucsDomainBuf = domain;
    domainPtr = ucsDomainBuf.get();
    domainLen = ucsDomainBuf.Length() * 2;
    WriteUnicodeLE((void *) domainPtr, (const PRUnichar *) domainPtr,
                   ucsDomainBuf.Length());
#else
    domainPtr = domain.get();
    domainLen = domain.Length() * 2;
#endif
  }
  else
  {
    NS_CopyUnicodeToNative(domain, oemDomainBuf);
    domainPtr = oemDomainBuf.get();
    domainLen = oemDomainBuf.Length();
  }

  
  
  
  if (unicode)
  {
#ifdef IS_BIG_ENDIAN
    ucsUserBuf = username;
    userPtr = ucsUserBuf.get();
    userLen = ucsUserBuf.Length() * 2;
    WriteUnicodeLE((void *) userPtr, (const PRUnichar *) userPtr,
                   ucsUserBuf.Length());
#else
    userPtr = username.get();
    userLen = username.Length() * 2;
#endif
  }
  else
  {
    NS_CopyUnicodeToNative(username, oemUserBuf);
    userPtr = oemUserBuf.get();
    userLen = oemUserBuf.Length();
  }

  
  
  
  char hostBuf[SYS_INFO_BUFFER_LENGTH];
  if (PR_GetSystemInfo(PR_SI_HOSTNAME, hostBuf, sizeof(hostBuf)) == PR_FAILURE)
    return NS_ERROR_UNEXPECTED;
  hostLen = strlen(hostBuf);
  if (unicode)
  {
    
    CopyASCIItoUTF16(nsDependentCString(hostBuf, hostLen), ucsHostBuf);
    hostPtr = ucsHostBuf.get();
    hostLen = ucsHostBuf.Length() * 2;
#ifdef IS_BIG_ENDIAN
    WriteUnicodeLE((void *) hostPtr, (const PRUnichar *) hostPtr,
                   ucsHostBuf.Length());
#endif
  }
  else
    hostPtr = hostBuf;

  
  
  
  *outLen = NTLM_TYPE3_HEADER_LEN + hostLen + domainLen + userLen +
            LM_RESP_LEN + NTLM_RESP_LEN;
  *outBuf = nsMemory::Alloc(*outLen);
  if (!*outBuf)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  uint8_t lmResp[LM_RESP_LEN], ntlmResp[NTLM_RESP_LEN], ntlmHash[NTLM_HASH_LEN];
  if (msg.flags & NTLM_NegotiateNTLM2Key)
  {
    
    uint8_t sessionHash[16], temp[16];

    PK11_GenerateRandom(lmResp, 8);
    memset(lmResp + 8, 0, LM_RESP_LEN - 8);

    memcpy(temp, msg.challenge, 8);
    memcpy(temp + 8, lmResp, 8);
    md5sum(temp, 16, sessionHash);

    NTLM_Hash(password, ntlmHash);
    LM_Response(ntlmHash, sessionHash, ntlmResp);
  }
  else
  {
    NTLM_Hash(password, ntlmHash);
    LM_Response(ntlmHash, msg.challenge, ntlmResp);

    if (SendLM())
    {
      uint8_t lmHash[LM_HASH_LEN];
      LM_Hash(password, lmHash);
      LM_Response(lmHash, msg.challenge, lmResp);
    }
    else
    {
      
      
      
      LM_Response(ntlmHash, msg.challenge, lmResp);
    }
  }

  
  
  
  void *cursor = *outBuf;
  uint32_t offset;

  
  cursor = WriteBytes(cursor, NTLM_SIGNATURE, sizeof(NTLM_SIGNATURE));

  
  cursor = WriteBytes(cursor, NTLM_TYPE3_MARKER, sizeof(NTLM_TYPE3_MARKER));

  
  offset = NTLM_TYPE3_HEADER_LEN + domainLen + userLen + hostLen;
  cursor = WriteSecBuf(cursor, LM_RESP_LEN, offset);
  memcpy((uint8_t *) *outBuf + offset, lmResp, LM_RESP_LEN);

  
  offset += LM_RESP_LEN;
  cursor = WriteSecBuf(cursor, NTLM_RESP_LEN, offset);
  memcpy((uint8_t *) *outBuf + offset, ntlmResp, NTLM_RESP_LEN);

  
  offset = NTLM_TYPE3_HEADER_LEN;
  cursor = WriteSecBuf(cursor, domainLen, offset);
  memcpy((uint8_t *) *outBuf + offset, domainPtr, domainLen);

  
  offset += domainLen;
  cursor = WriteSecBuf(cursor, userLen, offset);
  memcpy((uint8_t *) *outBuf + offset, userPtr, userLen);

  
  offset += userLen;
  cursor = WriteSecBuf(cursor, hostLen, offset);
  memcpy((uint8_t *) *outBuf + offset, hostPtr, hostLen);

  
  cursor = WriteSecBuf(cursor, 0, 0);

  
  cursor = WriteDWORD(cursor, msg.flags & NTLM_TYPE1_FLAGS);

  return NS_OK;
}



NS_IMPL_ISUPPORTS1(nsNTLMAuthModule, nsIAuthModule)

nsNTLMAuthModule::~nsNTLMAuthModule()
{
  ZapString(mPassword);
}

nsresult
nsNTLMAuthModule::InitTest()
{
  nsNSSShutDownPreventionLock locker;
  
  
  
  return PK11_IsFIPS() ? NS_ERROR_NOT_AVAILABLE : NS_OK;
}

NS_IMETHODIMP
nsNTLMAuthModule::Init(const char      *serviceName,
                       uint32_t         serviceFlags,
                       const PRUnichar *domain,
                       const PRUnichar *username,
                       const PRUnichar *password)
{
  NS_ASSERTION(serviceFlags == nsIAuthModule::REQ_DEFAULT, "unexpected service flags");

  mDomain = domain;
  mUsername = username;
  mPassword = password;
  return NS_OK;
}

NS_IMETHODIMP
nsNTLMAuthModule::GetNextToken(const void *inToken,
                               uint32_t    inTokenLen,
                               void      **outToken,
                               uint32_t   *outTokenLen)
{
  nsresult rv;
  nsNSSShutDownPreventionLock locker;
  
  
  
  if (PK11_IsFIPS())
    return NS_ERROR_NOT_AVAILABLE;

  
  if (inToken)
  {
    LogToken("in-token", inToken, inTokenLen);
    rv = GenerateType3Msg(mDomain, mUsername, mPassword, inToken,
                          inTokenLen, outToken, outTokenLen);
  }
  else
  {
    rv = GenerateType1Msg(outToken, outTokenLen);
  }

#ifdef PR_LOGGING
  if (NS_SUCCEEDED(rv))
    LogToken("out-token", *outToken, *outTokenLen);
#endif

  return rv;
}

NS_IMETHODIMP
nsNTLMAuthModule::Unwrap(const void *inToken,
                        uint32_t    inTokenLen,
                        void      **outToken,
                        uint32_t   *outTokenLen)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsNTLMAuthModule::Wrap(const void *inToken,
                       uint32_t    inTokenLen,
                       bool        confidential,
                       void      **outToken,
                       uint32_t   *outTokenLen)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsNTLMAuthModule::GetModuleProperties(uint32_t *flags)
{
    *flags = 0;
    return NS_OK;
}





static uint8_t
des_setkeyparity(uint8_t x)
{
  if ((((x >> 7) ^ (x >> 6) ^ (x >> 5) ^
        (x >> 4) ^ (x >> 3) ^ (x >> 2) ^
        (x >> 1)) & 0x01) == 0)
    x |= 0x01;
  else
    x &= 0xfe;
  return x;
}


static void
des_makekey(const uint8_t *raw, uint8_t *key)
{
  key[0] = des_setkeyparity(raw[0]);
  key[1] = des_setkeyparity((raw[0] << 7) | (raw[1] >> 1));
  key[2] = des_setkeyparity((raw[1] << 6) | (raw[2] >> 2));
  key[3] = des_setkeyparity((raw[2] << 5) | (raw[3] >> 3));
  key[4] = des_setkeyparity((raw[3] << 4) | (raw[4] >> 4));
  key[5] = des_setkeyparity((raw[4] << 3) | (raw[5] >> 5));
  key[6] = des_setkeyparity((raw[5] << 2) | (raw[6] >> 6));
  key[7] = des_setkeyparity((raw[6] << 1));
}


static void
des_encrypt(const uint8_t *key, const uint8_t *src, uint8_t *hash)
{
  CK_MECHANISM_TYPE cipherMech = CKM_DES_ECB;
  PK11SlotInfo *slot = nullptr;
  PK11SymKey *symkey = nullptr;
  PK11Context *ctxt = nullptr;
  SECItem keyItem, *param = nullptr;
  SECStatus rv;
  unsigned int n;
  
  slot = PK11_GetBestSlot(cipherMech, nullptr);
  if (!slot)
  {
    NS_ERROR("no slot");
    goto done;
  }

  keyItem.data = (uint8_t *) key;
  keyItem.len = 8;
  symkey = PK11_ImportSymKey(slot, cipherMech,
                             PK11_OriginUnwrap, CKA_ENCRYPT,
                             &keyItem, nullptr);
  if (!symkey)
  {
    NS_ERROR("no symkey");
    goto done;
  }

  
  param = PK11_ParamFromIV(cipherMech, nullptr);
  if (!param)
  {
    NS_ERROR("no param");
    goto done;
  }

  ctxt = PK11_CreateContextBySymKey(cipherMech, CKA_ENCRYPT,
                                    symkey, param);
  if (!ctxt)
  {
    NS_ERROR("no context");
    goto done;
  }

  rv = PK11_CipherOp(ctxt, hash, (int *) &n, 8, (uint8_t *) src, 8);
  if (rv != SECSuccess)
  {
    NS_ERROR("des failure");
    goto done;
  }

  rv = PK11_DigestFinal(ctxt, hash+8, &n, 0);
  if (rv != SECSuccess)
  {
    NS_ERROR("des failure");
    goto done;
  }

done:
  if (ctxt)
    PK11_DestroyContext(ctxt, true);
  if (symkey)
    PK11_FreeSymKey(symkey);
  if (param)
    SECITEM_FreeItem(param, true);
  if (slot)
    PK11_FreeSlot(slot);
}




static void md5sum(const uint8_t *input, uint32_t inputLen, uint8_t *result)
{
  PK11Context *ctxt = PK11_CreateDigestContext(SEC_OID_MD5);
  if (ctxt)
  {
    if (PK11_DigestBegin(ctxt) == SECSuccess)
    {
      if (PK11_DigestOp(ctxt, input, inputLen) == SECSuccess)
      {
        uint32_t resultLen = 16;
        PK11_DigestFinal(ctxt, result, &resultLen, resultLen);
      }
    }
    PK11_DestroyContext(ctxt, true);
  }
}
