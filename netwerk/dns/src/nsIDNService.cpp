





































#include "nsIDNService.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsUnicharUtils.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIObserverService.h"
#include "nsISupportsPrimitives.h"
#include "punycode.h"



static const PRUint32 kMaxDNSNodeLen = 63;



#define NS_NET_PREF_IDNTESTBED      "network.IDN_testbed"
#define NS_NET_PREF_IDNPREFIX       "network.IDN_prefix"
#define NS_NET_PREF_IDNBLACKLIST    "network.IDN.blacklist_chars"

inline PRBool isOnlySafeChars(const nsAFlatString& in,
                              const nsAFlatString& blacklist)
{
  return (blacklist.IsEmpty() ||
          in.FindCharInSet(blacklist) == kNotFound);
}






NS_IMPL_THREADSAFE_ISUPPORTS3(nsIDNService,
                              nsIIDNService,
                              nsIObserver,
                              nsISupportsWeakReference)

nsresult nsIDNService::Init()
{
  nsCOMPtr<nsIPrefBranch2> prefInternal(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefInternal) {
    prefInternal->AddObserver(NS_NET_PREF_IDNTESTBED, this, PR_TRUE); 
    prefInternal->AddObserver(NS_NET_PREF_IDNPREFIX, this, PR_TRUE); 
    prefInternal->AddObserver(NS_NET_PREF_IDNBLACKLIST, this, PR_TRUE);
    prefsChanged(prefInternal, nsnull);
  }
  return NS_OK;
}

NS_IMETHODIMP nsIDNService::Observe(nsISupports *aSubject,
                                    const char *aTopic,
                                    const PRUnichar *aData)
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsCOMPtr<nsIPrefBranch> prefBranch( do_QueryInterface(aSubject) );
    if (prefBranch)
      prefsChanged(prefBranch, aData);
  }
  return NS_OK;
}

void nsIDNService::prefsChanged(nsIPrefBranch *prefBranch, const PRUnichar *pref)
{
  if (!pref || NS_LITERAL_STRING(NS_NET_PREF_IDNTESTBED).Equals(pref)) {
    PRBool val;
    if (NS_SUCCEEDED(prefBranch->GetBoolPref(NS_NET_PREF_IDNTESTBED, &val)))
      mMultilingualTestBed = val;
  }
  if (!pref || NS_LITERAL_STRING(NS_NET_PREF_IDNPREFIX).Equals(pref)) {
    nsXPIDLCString prefix;
    nsresult rv = prefBranch->GetCharPref(NS_NET_PREF_IDNPREFIX, getter_Copies(prefix));
    if (NS_SUCCEEDED(rv) && prefix.Length() <= kACEPrefixLen)
      PL_strncpyz(nsIDNService::mACEPrefix, prefix.get(), kACEPrefixLen + 1);
  }
  if (!pref || NS_LITERAL_STRING(NS_NET_PREF_IDNBLACKLIST).Equals(pref)) {
    nsCOMPtr<nsISupportsString> blacklist;
    nsresult rv = prefBranch->GetComplexValue(NS_NET_PREF_IDNBLACKLIST,
                                              NS_GET_IID(nsISupportsString),
                                              getter_AddRefs(blacklist));
    if (NS_SUCCEEDED(rv))
      blacklist->ToString(getter_Copies(mIDNBlacklist));
    else
      mIDNBlacklist.Truncate();
  }
}

nsIDNService::nsIDNService()
{
  nsresult rv;

  
  const char kIDNSPrefix[] = "xn--";
  strcpy(mACEPrefix, kIDNSPrefix);

  mMultilingualTestBed = PR_FALSE;

  if (idn_success != idn_nameprep_create(NULL, &mNamePrepHandle))
    mNamePrepHandle = nsnull;

  mNormalizer = do_GetService(NS_UNICODE_NORMALIZER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    mNormalizer = nsnull;
  
}

nsIDNService::~nsIDNService()
{
  idn_nameprep_destroy(mNamePrepHandle);
}


NS_IMETHODIMP nsIDNService::ConvertUTF8toACE(const nsACString & input, nsACString & ace)
{
  nsresult rv;
  NS_ConvertUTF8toUTF16 ustr(input);

  
  normalizeFullStops(ustr);


  PRUint32 len, offset;
  len = 0;
  offset = 0;
  nsCAutoString encodedBuf;

  nsAString::const_iterator start, end;
  ustr.BeginReading(start); 
  ustr.EndReading(end); 
  ace.Truncate();

  
  while (start != end) {
    len++;
    if (*start++ == (PRUnichar)'.') {
      rv = stringPrepAndACE(Substring(ustr, offset, len - 1), encodedBuf);
      NS_ENSURE_SUCCESS(rv, rv);

      ace.Append(encodedBuf);
      ace.Append('.');
      offset += len;
      len = 0;
    }
  }

  
  if (mMultilingualTestBed)
    ace.AppendLiteral("mltbd.");
  
  if (len) {
    rv = stringPrepAndACE(Substring(ustr, offset, len), encodedBuf);
    NS_ENSURE_SUCCESS(rv, rv);

    ace.Append(encodedBuf);
  }

  return NS_OK;
}


NS_IMETHODIMP nsIDNService::ConvertACEtoUTF8(const nsACString & input, nsACString & _retval)
{
  
  
  

  if (!IsASCII(input)) {
    _retval.Assign(input);
    return NS_OK;
  }
  
  PRUint32 len = 0, offset = 0;
  nsCAutoString decodedBuf;

  nsACString::const_iterator start, end;
  input.BeginReading(start); 
  input.EndReading(end); 
  _retval.Truncate();

  
  while (start != end) {
    len++;
    if (*start++ == '.') {
      if (NS_FAILED(decodeACE(Substring(input, offset, len - 1), decodedBuf))) {
        _retval.Assign(input);
        return NS_OK;
      }

      _retval.Append(decodedBuf);
      _retval.Append('.');
      offset += len;
      len = 0;
    }
  }
  
  if (len) {
    if (NS_FAILED(decodeACE(Substring(input, offset, len), decodedBuf)))
      _retval.Assign(input);
    else
      _retval.Append(decodedBuf);
  }

  return NS_OK;
}


NS_IMETHODIMP nsIDNService::IsACE(const nsACString & input, PRBool *_retval)
{
  nsACString::const_iterator begin;
  input.BeginReading(begin);

  const char *data = begin.get();
  PRUint32 dataLen = begin.size_forward();

  
  
  

  const char *p = PL_strncasestr(data, mACEPrefix, dataLen);

  *_retval = p && (p == data || *(p - 1) == '.');
  return NS_OK;
}


NS_IMETHODIMP nsIDNService::Normalize(const nsACString & input, nsACString & output)
{
  
  NS_ENSURE_TRUE(IsUTF8(input), NS_ERROR_UNEXPECTED);

  NS_ConvertUTF8toUTF16 inUTF16(input);
  normalizeFullStops(inUTF16);

  nsAutoString outUTF16;
  nsresult rv = stringPrep(inUTF16, outUTF16);
  if (NS_FAILED(rv))
    return rv;

  CopyUTF16toUTF8(outUTF16, output);
  if (!isOnlySafeChars(outUTF16, mIDNBlacklist))
    return ConvertUTF8toACE(output, output);

  return NS_OK;
}



static void utf16ToUcs4(const nsAString& in, PRUint32 *out, PRUint32 outBufLen, PRUint32 *outLen)
{
  PRUint32 i = 0;
  nsAString::const_iterator start, end;
  in.BeginReading(start); 
  in.EndReading(end); 

  while (start != end) {
    PRUnichar curChar;

    curChar= *start++;

    if (start != end &&
        NS_IS_HIGH_SURROGATE(curChar) && 
        NS_IS_LOW_SURROGATE(*start)) {
      out[i] = SURROGATE_TO_UCS4(curChar, *start);
      ++start;
    }
    else
      out[i] = curChar;

    i++;
    if (i >= outBufLen) {
      NS_ERROR("input too big, the result truncated");
      out[outBufLen-1] = (PRUint32)'\0';
      *outLen = outBufLen-1;
      return;
    }
  }
  out[i] = (PRUint32)'\0';
  *outLen = i;
}

static void ucs4toUtf16(const PRUint32 *in, nsAString& out)
{
  while (*in) {
    if (!IS_IN_BMP(*in)) {
      out.Append((PRUnichar) H_SURROGATE(*in));
      out.Append((PRUnichar) L_SURROGATE(*in));
    }
    else
      out.Append((PRUnichar) *in);
    in++;
  }
}

static nsresult punycode(const char* prefix, const nsAString& in, nsACString& out)
{
  PRUint32 ucs4Buf[kMaxDNSNodeLen + 1];
  PRUint32 ucs4Len;
  utf16ToUcs4(in, ucs4Buf, kMaxDNSNodeLen, &ucs4Len);

  
  
  const PRUint32 kEncodedBufSize = kMaxDNSNodeLen * 20 / 8 + 1 + 1;  
  char encodedBuf[kEncodedBufSize];
  punycode_uint encodedLength = kEncodedBufSize;

  enum punycode_status status = punycode_encode(ucs4Len,
                                                ucs4Buf,
                                                nsnull,
                                                &encodedLength,
                                                encodedBuf);

  if (punycode_success != status ||
      encodedLength >= kEncodedBufSize)
    return NS_ERROR_FAILURE;

  encodedBuf[encodedLength] = '\0';
  out.Assign(nsDependentCString(prefix) + nsDependentCString(encodedBuf));

  return NS_OK;
}

static nsresult encodeToRACE(const char* prefix, const nsAString& in, nsACString& out)
{
  
  
  const PRUint32 kEncodedBufSize = kMaxDNSNodeLen * 20 / 8 + 1 + 1;  

  
  PRUnichar temp[kMaxDNSNodeLen + 2];
  temp[0] = 0xFFFF;   
  temp[in.Length() + 1] = (PRUnichar)'\0';

  nsAString::const_iterator start, end;
  in.BeginReading(start); 
  in.EndReading(end);
  
  for (PRUint32 i = 1; start != end; i++)
    temp[i] = *start++;

  

  char encodedBuf[kEncodedBufSize];
  idn_result_t result = race_compress_encode((const unsigned short *) temp, 
                                             get_compress_mode((unsigned short *) temp + 1), 
                                             encodedBuf, kEncodedBufSize);
  if (idn_success != result)
    return NS_ERROR_FAILURE;

  out.Assign(prefix);
  out.Append(encodedBuf);

  return NS_OK;
}


















nsresult nsIDNService::stringPrep(const nsAString& in, nsAString& out)
{
  if (!mNamePrepHandle || !mNormalizer)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  PRUint32 ucs4Buf[kMaxDNSNodeLen + 1];
  PRUint32 ucs4Len;
  utf16ToUcs4(in, ucs4Buf, kMaxDNSNodeLen, &ucs4Len);

  
  idn_result_t idn_err;

  PRUint32 namePrepBuf[kMaxDNSNodeLen * 3];   
  idn_err = idn_nameprep_map(mNamePrepHandle, (const PRUint32 *) ucs4Buf,
		                     (PRUint32 *) namePrepBuf, kMaxDNSNodeLen * 3);
  NS_ENSURE_TRUE(idn_err == idn_success, NS_ERROR_FAILURE);

  nsAutoString namePrepStr;
  ucs4toUtf16(namePrepBuf, namePrepStr);
  if (namePrepStr.Length() >= kMaxDNSNodeLen)
    return NS_ERROR_FAILURE;

  
  nsAutoString normlizedStr;
  rv = mNormalizer->NormalizeUnicodeNFKC(namePrepStr, normlizedStr);
  if (normlizedStr.Length() >= kMaxDNSNodeLen)
    return NS_ERROR_FAILURE;

  
  const PRUint32 *found = nsnull;
  idn_err = idn_nameprep_isprohibited(mNamePrepHandle, 
                                      (const PRUint32 *) ucs4Buf, &found);
  if (idn_err != idn_success || found)
    return NS_ERROR_FAILURE;

  
  idn_err = idn_nameprep_isvalidbidi(mNamePrepHandle, 
                                     (const PRUint32 *) ucs4Buf, &found);
  if (idn_err != idn_success || found)
    return NS_ERROR_FAILURE;

  
  out.Assign(normlizedStr);

  return rv;
}

nsresult nsIDNService::encodeToACE(const nsAString& in, nsACString& out)
{
  
  if (!strcmp("bq--", mACEPrefix))
    return encodeToRACE(mACEPrefix, in, out);
  
  
  return punycode(mACEPrefix, in, out);
}

nsresult nsIDNService::stringPrepAndACE(const nsAString& in, nsACString& out)
{
  nsresult rv = NS_OK;

  out.Truncate();

  if (in.Length() > kMaxDNSNodeLen) {
    NS_ERROR("IDN node too large");
    return NS_ERROR_FAILURE;
  }

  if (IsASCII(in))
    LossyCopyUTF16toASCII(in, out);
  else {
    nsAutoString strPrep;
    rv = stringPrep(in, strPrep);
    if (NS_SUCCEEDED(rv)) {
      if (IsASCII(strPrep))
        LossyCopyUTF16toASCII(strPrep, out);
      else
        rv = encodeToACE(strPrep, out);
    }
  }

  if (out.Length() > kMaxDNSNodeLen) {
    NS_ERROR("IDN node too large");
    return NS_ERROR_FAILURE;
  }

  return rv;
}







void nsIDNService::normalizeFullStops(nsAString& s)
{
  nsAString::const_iterator start, end;
  s.BeginReading(start); 
  s.EndReading(end); 
  PRInt32 index = 0;

  while (start != end) {
    switch (*start) {
      case 0x3002:
      case 0xFF0E:
      case 0xFF61:
        s.Replace(index, 1, NS_LITERAL_STRING("."));
        break;
      default:
        break;
    }
    start++;
    index++;
  }
}

nsresult nsIDNService::decodeACE(const nsACString& in, nsACString& out)
{
  PRBool isAce;
  IsACE(in, &isAce);
  if (!isAce) {
    out.Assign(in);
    return NS_OK;
  }

  
  
  punycode_uint output_length = in.Length() - kACEPrefixLen + 1;
  punycode_uint *output = new punycode_uint[output_length];
  NS_ENSURE_TRUE(output, NS_ERROR_OUT_OF_MEMORY);

  enum punycode_status status = punycode_decode(in.Length() - kACEPrefixLen,
                                                PromiseFlatCString(in).get() + kACEPrefixLen,
                                                &output_length,
                                                output,
                                                nsnull);
  if (status != punycode_success) {
    delete [] output;
    return NS_ERROR_FAILURE;
  }

  
  output[output_length] = 0;
  nsAutoString utf16;
  ucs4toUtf16(output, utf16);
  delete [] output;
  if (!isOnlySafeChars(utf16, mIDNBlacklist))
    return NS_ERROR_FAILURE;
  CopyUTF16toUTF8(utf16, out);

  
  nsCAutoString ace;
  nsresult rv = ConvertUTF8toACE(out, ace);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ace.Equals(in, nsCaseInsensitiveCStringComparator()))
    return NS_ERROR_FAILURE;

  return NS_OK;
}
