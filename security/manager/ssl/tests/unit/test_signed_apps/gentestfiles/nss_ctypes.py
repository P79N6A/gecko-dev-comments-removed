from ctypes import *
import os
import sys

if sys.platform == 'darwin':
  libprefix = "lib"
  libsuffix = ".dylib"
elif os.name == 'posix':
  libprefix = "lib"
  libsuffix = ".so"
else: 
  libprefix = ""
  libsuffix = ".dll"

plc   = cdll.LoadLibrary(libprefix + "plc4"   + libsuffix)
nspr  = cdll.LoadLibrary(libprefix + "nspr4"  + libsuffix)
nss   = cdll.LoadLibrary(libprefix + "nss3"   + libsuffix)
smime = cdll.LoadLibrary(libprefix + "smime3" + libsuffix)

nspr.PR_GetError.argtypes = []
nspr.PR_GetError.restype = c_int32
nspr.PR_ErrorToName.argtypes = [c_int32]
nspr.PR_ErrorToName.restype = c_char_p

def raise_if_not_SECSuccess(rv):
  SECSuccess = 0
  if (rv != SECSuccess):
    raise ValueError(nspr.PR_ErrorToName(nspr.PR_GetError()))

def raise_if_NULL(p):
  if not p:
    raise ValueError(nspr.PR_ErrorToName(nspr.PR_GetError()))
  return p

PRBool = c_int
SECStatus = c_int


SEC_OID_SHA1 = 4


certUsageObjectSigner = 6

class SECItem(Structure):
  _fields_ = [("type", c_int),
              ("data", c_char_p),
              ("len", c_uint)]

nss.NSS_Init.argtypes = [c_char_p]
nss.NSS_Init.restype = SECStatus
def NSS_Init(db_dir):
  nss.NSS_Init.argtypes = [c_char_p]
  nss.NSS_Init.restype = SECStatus
  raise_if_not_SECSuccess(nss.NSS_Init(db_dir))

nss.NSS_Shutdown.argtypes = []
nss.NSS_Shutdown.restype = SECStatus
def NSS_Shutdown():
  raise_if_not_SECSuccess(nss.NSS_Shutdown())

PK11PasswordFunc = CFUNCTYPE(c_char_p, c_void_p, PRBool, c_char_p)


nss.PK11_SetPasswordFunc.argtypes = [PK11PasswordFunc]
nss.PK11_SetPasswordFunc.restype = None


plc.PL_strdup.argtypes = [c_char_p]
plc.PL_strdup.restype = c_void_p
def SetPasswordContext(password):
  def callback(slot, retry, arg):
    return plc.PL_strdup(password)
  wincx = PK11PasswordFunc(callback)
  nss.PK11_SetPasswordFunc(wincx)
  return wincx

nss.CERT_GetDefaultCertDB.argtypes = []
nss.CERT_GetDefaultCertDB.restype = c_void_p
def CERT_GetDefaultCertDB():
  return raise_if_NULL(nss.CERT_GetDefaultCertDB())

nss.PK11_FindCertFromNickname.argtypes = [c_char_p, c_void_p]
nss.PK11_FindCertFromNickname.restype = c_void_p
def PK11_FindCertFromNickname(nickname, wincx):
  return raise_if_NULL(nss.PK11_FindCertFromNickname(nickname, wincx))

nss.CERT_DestroyCertificate.argtypes = [c_void_p]
nss.CERT_DestroyCertificate.restype = None
def CERT_DestroyCertificate(cert):
  nss.CERT_DestroyCertificate(cert)

smime.SEC_PKCS7CreateSignedData.argtypes = [c_void_p, c_int, c_void_p,
                                            c_int, c_void_p,
                                            c_void_p, c_void_p]
smime.SEC_PKCS7CreateSignedData.restype = c_void_p
def SEC_PKCS7CreateSignedData(cert, certusage, certdb, digestalg, digest, wincx):
  item = SECItem(0,  c_char_p(digest), len(digest))
  return raise_if_NULL(smime.SEC_PKCS7CreateSignedData(cert, certusage, certdb,
                                                       digestalg,
                                                       pointer(item),
                                                       None, wincx))

smime.SEC_PKCS7AddSigningTime.argtypes = [c_void_p]
smime.SEC_PKCS7AddSigningTime.restype = SECStatus
def SEC_PKCS7AddSigningTime(p7):
  raise_if_not_SECSuccess(smime.SEC_PKCS7AddSigningTime(p7))

smime.SEC_PKCS7IncludeCertChain.argtypes = [c_void_p, c_void_p]
smime.SEC_PKCS7IncludeCertChain.restype = SECStatus
def SEC_PKCS7IncludeCertChain(p7, wincx):
  raise_if_not_SECSuccess(smime.SEC_PKCS7IncludeCertChain(p7, wincx))

SEC_PKCS7EncoderOutputCallback = CFUNCTYPE(None, c_void_p, c_void_p, c_long)
smime.SEC_PKCS7Encode.argtypes = [c_void_p, SEC_PKCS7EncoderOutputCallback,
                                  c_void_p, c_void_p, c_void_p, c_void_p]
smime.SEC_PKCS7Encode.restype = SECStatus
def SEC_PKCS7Encode(p7, bulkkey, wincx):
  outputChunks = []
  def callback(chunks, data, len):
    outputChunks.append(string_at(data, len))
  callbackWrapper = SEC_PKCS7EncoderOutputCallback(callback)
  raise_if_not_SECSuccess(smime.SEC_PKCS7Encode(p7, callbackWrapper,
                                                None, None, None, wincx))
  return "".join(outputChunks)

smime.SEC_PKCS7DestroyContentInfo.argtypes = [c_void_p]
smime.SEC_PKCS7DestroyContentInfo.restype = None
def SEC_PKCS7DestroyContentInfo(p7):
  smime.SEC_PKCS7DestroyContentInfo(p7)
