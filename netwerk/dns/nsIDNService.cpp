




#include "nsIDNService.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsUnicharUtils.h"
#include "nsUnicodeProperties.h"
#include "nsUnicodeScriptCodes.h"
#include "harfbuzz/hb.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIObserverService.h"
#include "nsISupportsPrimitives.h"
#include "punycode.h"


using namespace mozilla::unicode;



static const uint32_t kMaxDNSNodeLen = 63;



#define NS_NET_PREF_IDNTESTBED      "network.IDN_testbed"
#define NS_NET_PREF_IDNPREFIX       "network.IDN_prefix"
#define NS_NET_PREF_IDNBLACKLIST    "network.IDN.blacklist_chars"
#define NS_NET_PREF_SHOWPUNYCODE    "network.IDN_show_punycode"
#define NS_NET_PREF_IDNWHITELIST    "network.IDN.whitelist."
#define NS_NET_PREF_IDNUSEWHITELIST "network.IDN.use_whitelist"
#define NS_NET_PREF_IDNRESTRICTION  "network.IDN.restriction_profile"

inline bool isOnlySafeChars(const nsAFlatString& in,
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
  nsCOMPtr<nsIPrefService> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefs)
    prefs->GetBranch(NS_NET_PREF_IDNWHITELIST, getter_AddRefs(mIDNWhitelistPrefBranch));

  nsCOMPtr<nsIPrefBranch> prefInternal(do_QueryInterface(prefs));
  if (prefInternal) {
    prefInternal->AddObserver(NS_NET_PREF_IDNTESTBED, this, true); 
    prefInternal->AddObserver(NS_NET_PREF_IDNPREFIX, this, true); 
    prefInternal->AddObserver(NS_NET_PREF_IDNBLACKLIST, this, true);
    prefInternal->AddObserver(NS_NET_PREF_SHOWPUNYCODE, this, true);
    prefInternal->AddObserver(NS_NET_PREF_IDNRESTRICTION, this, true);
    prefInternal->AddObserver(NS_NET_PREF_IDNUSEWHITELIST, this, true);
    prefsChanged(prefInternal, nullptr);
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
    bool val;
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
  if (!pref || NS_LITERAL_STRING(NS_NET_PREF_SHOWPUNYCODE).Equals(pref)) {
    bool val;
    if (NS_SUCCEEDED(prefBranch->GetBoolPref(NS_NET_PREF_SHOWPUNYCODE, &val)))
      mShowPunycode = val;
  }
  if (!pref || NS_LITERAL_STRING(NS_NET_PREF_IDNUSEWHITELIST).Equals(pref)) {
    bool val;
    if (NS_SUCCEEDED(prefBranch->GetBoolPref(NS_NET_PREF_IDNUSEWHITELIST,
                                             &val)))
      mIDNUseWhitelist = val;
  }
  if (!pref || NS_LITERAL_STRING(NS_NET_PREF_IDNRESTRICTION).Equals(pref)) {
    nsXPIDLCString profile;
    if (NS_FAILED(prefBranch->GetCharPref(NS_NET_PREF_IDNRESTRICTION,
                                          getter_Copies(profile)))) {
      profile.Truncate();
    }
    if (profile.Equals(NS_LITERAL_CSTRING("moderate"))) {
      mRestrictionProfile = eModeratelyRestrictiveProfile;
    } else if (profile.Equals(NS_LITERAL_CSTRING("high"))) {
      mRestrictionProfile = eHighlyRestrictiveProfile;
    } else {
      mRestrictionProfile = eASCIIOnlyProfile;
    }
  }
}

nsIDNService::nsIDNService()
{
  
  const char kIDNSPrefix[] = "xn--";
  strcpy(mACEPrefix, kIDNSPrefix);

  mMultilingualTestBed = false;

  if (idn_success != idn_nameprep_create(NULL, &mNamePrepHandle))
    mNamePrepHandle = nullptr;

  mNormalizer = do_GetService(NS_UNICODE_NORMALIZER_CONTRACTID);
  
}

nsIDNService::~nsIDNService()
{
  idn_nameprep_destroy(mNamePrepHandle);
}


NS_IMETHODIMP nsIDNService::ConvertUTF8toACE(const nsACString & input, nsACString & ace)
{
  return UTF8toACE(input, ace, true, true);
}

nsresult nsIDNService::SelectiveUTF8toACE(const nsACString& input, nsACString& ace)
{
  return UTF8toACE(input, ace, true, false);
}

nsresult nsIDNService::UTF8toACE(const nsACString & input, nsACString & ace, bool allowUnassigned, bool convertAllLabels)
{
  nsresult rv;
  NS_ConvertUTF8toUTF16 ustr(input);

  
  normalizeFullStops(ustr);


  uint32_t len, offset;
  len = 0;
  offset = 0;
  nsAutoCString encodedBuf;

  nsAString::const_iterator start, end;
  ustr.BeginReading(start); 
  ustr.EndReading(end); 
  ace.Truncate();

  
  while (start != end) {
    len++;
    if (*start++ == (PRUnichar)'.') {
      rv = stringPrepAndACE(Substring(ustr, offset, len - 1), encodedBuf,
                            allowUnassigned, convertAllLabels);
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
    rv = stringPrepAndACE(Substring(ustr, offset, len), encodedBuf,
                          allowUnassigned, convertAllLabels);
    NS_ENSURE_SUCCESS(rv, rv);

    ace.Append(encodedBuf);
  }

  return NS_OK;
}


NS_IMETHODIMP nsIDNService::ConvertACEtoUTF8(const nsACString & input, nsACString & _retval)
{
  return ACEtoUTF8(input, _retval, true, true);
}

nsresult nsIDNService::SelectiveACEtoUTF8(const nsACString& input, nsACString& _retval)
{
  return ACEtoUTF8(input, _retval, false, false);
}

nsresult nsIDNService::ACEtoUTF8(const nsACString & input, nsACString & _retval,
                                 bool allowUnassigned, bool convertAllLabels)
{
  
  
  

  uint32_t len = 0, offset = 0;
  nsAutoCString decodedBuf;

  nsACString::const_iterator start, end;
  input.BeginReading(start); 
  input.EndReading(end); 
  _retval.Truncate();

  
  while (start != end) {
    len++;
    if (*start++ == '.') {
      if (NS_FAILED(decodeACE(Substring(input, offset, len - 1), decodedBuf,
                              allowUnassigned, convertAllLabels))) {
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
    if (NS_FAILED(decodeACE(Substring(input, offset, len), decodedBuf,
                            allowUnassigned, convertAllLabels)))
      _retval.Assign(input);
    else
      _retval.Append(decodedBuf);
  }

  return NS_OK;
}


NS_IMETHODIMP nsIDNService::IsACE(const nsACString & input, bool *_retval)
{
  nsACString::const_iterator begin;
  input.BeginReading(begin);

  const char *data = begin.get();
  uint32_t dataLen = begin.size_forward();

  
  
  

  const char *p = PL_strncasestr(data, mACEPrefix, dataLen);

  *_retval = p && (p == data || *(p - 1) == '.');
  return NS_OK;
}


NS_IMETHODIMP nsIDNService::Normalize(const nsACString & input, nsACString & output)
{
  
  NS_ENSURE_TRUE(IsUTF8(input), NS_ERROR_UNEXPECTED);

  NS_ConvertUTF8toUTF16 inUTF16(input);
  normalizeFullStops(inUTF16);

  
  nsAutoString outUTF16, outLabel;

  uint32_t len = 0, offset = 0;
  nsresult rv;
  nsAString::const_iterator start, end;
  inUTF16.BeginReading(start);
  inUTF16.EndReading(end);

  while (start != end) {
    len++;
    if (*start++ == PRUnichar('.')) {
      rv = stringPrep(Substring(inUTF16, offset, len - 1), outLabel, true);
      NS_ENSURE_SUCCESS(rv, rv);
   
      outUTF16.Append(outLabel);
      outUTF16.Append(PRUnichar('.'));
      offset += len;
      len = 0;
    }
  }
  if (len) {
    rv = stringPrep(Substring(inUTF16, offset, len), outLabel, true);
    NS_ENSURE_SUCCESS(rv, rv);

    outUTF16.Append(outLabel);
  }

  CopyUTF16toUTF8(outUTF16, output);
  if (!isOnlySafeChars(outUTF16, mIDNBlacklist))
    return ConvertUTF8toACE(output, output);

  return NS_OK;
}

NS_IMETHODIMP nsIDNService::ConvertToDisplayIDN(const nsACString & input, bool * _isASCII, nsACString & _retval)
{
  
  

  nsresult rv = NS_OK;

  
  
  bool isACE;
  IsACE(input, &isACE);

  if (IsASCII(input)) {
    
    _retval = input;
    ToLowerCase(_retval);

    if (isACE && !mShowPunycode) {
      
      nsAutoCString temp(_retval);
      if (isInWhitelist(temp)) {
        
        ACEtoUTF8(temp, _retval, false, true);
      } else {
        
        
        SelectiveACEtoUTF8(temp, _retval);
      }
      *_isASCII = IsASCII(_retval);
    } else {
      *_isASCII = true;
    }
  } else {
    
    
    
    
    
    
    if (isACE) {
      nsAutoCString temp;
      ACEtoUTF8(input, temp, false, true);
      rv = Normalize(temp, _retval);
    } else {
      rv = Normalize(input, _retval);
    }
    if (NS_FAILED(rv)) return rv;

    if (mShowPunycode && NS_SUCCEEDED(ConvertUTF8toACE(_retval, _retval))) {
      *_isASCII = true;
      return NS_OK;
    }

    
    
    
    *_isASCII = IsASCII(_retval);
    if (!*_isASCII && !isInWhitelist(_retval)) {
      
      
      
      rv = SelectiveUTF8toACE(_retval, _retval);
      *_isASCII = IsASCII(_retval);
      return rv;
    }
  }

  return NS_OK;
}



static void utf16ToUcs4(const nsAString& in, uint32_t *out, uint32_t outBufLen, uint32_t *outLen)
{
  uint32_t i = 0;
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
      out[outBufLen-1] = (uint32_t)'\0';
      *outLen = outBufLen-1;
      return;
    }
  }
  out[i] = (uint32_t)'\0';
  *outLen = i;
}

static void ucs4toUtf16(const uint32_t *in, nsAString& out)
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
  uint32_t ucs4Buf[kMaxDNSNodeLen + 1];
  uint32_t ucs4Len;
  utf16ToUcs4(in, ucs4Buf, kMaxDNSNodeLen, &ucs4Len);

  
  
  const uint32_t kEncodedBufSize = kMaxDNSNodeLen * 20 / 8 + 1 + 1;  
  char encodedBuf[kEncodedBufSize];
  punycode_uint encodedLength = kEncodedBufSize;

  enum punycode_status status = punycode_encode(ucs4Len,
                                                ucs4Buf,
                                                nullptr,
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
  
  
  const uint32_t kEncodedBufSize = kMaxDNSNodeLen * 20 / 8 + 1 + 1;  

  
  PRUnichar temp[kMaxDNSNodeLen + 2];
  temp[0] = 0xFFFF;   
  temp[in.Length() + 1] = (PRUnichar)'\0';

  nsAString::const_iterator start, end;
  in.BeginReading(start); 
  in.EndReading(end);
  
  for (uint32_t i = 1; start != end; i++)
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






















nsresult nsIDNService::stringPrep(const nsAString& in, nsAString& out,
                                  bool allowUnassigned)
{
  if (!mNamePrepHandle || !mNormalizer)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  uint32_t ucs4Buf[kMaxDNSNodeLen + 1];
  uint32_t ucs4Len;
  utf16ToUcs4(in, ucs4Buf, kMaxDNSNodeLen, &ucs4Len);

  
  idn_result_t idn_err;

  uint32_t namePrepBuf[kMaxDNSNodeLen * 3];   
  idn_err = idn_nameprep_map(mNamePrepHandle, (const uint32_t *) ucs4Buf,
		                     (uint32_t *) namePrepBuf, kMaxDNSNodeLen * 3);
  NS_ENSURE_TRUE(idn_err == idn_success, NS_ERROR_FAILURE);

  nsAutoString namePrepStr;
  ucs4toUtf16(namePrepBuf, namePrepStr);
  if (namePrepStr.Length() >= kMaxDNSNodeLen)
    return NS_ERROR_FAILURE;

  
  nsAutoString normlizedStr;
  rv = mNormalizer->NormalizeUnicodeNFKC(namePrepStr, normlizedStr);
  if (normlizedStr.Length() >= kMaxDNSNodeLen)
    return NS_ERROR_FAILURE;

  
  const uint32_t *found = nullptr;
  idn_err = idn_nameprep_isprohibited(mNamePrepHandle, 
                                      (const uint32_t *) ucs4Buf, &found);
  if (idn_err != idn_success || found)
    return NS_ERROR_FAILURE;

  
  idn_err = idn_nameprep_isvalidbidi(mNamePrepHandle, 
                                     (const uint32_t *) ucs4Buf, &found);
  if (idn_err != idn_success || found)
    return NS_ERROR_FAILURE;

  if (!allowUnassigned) {
    
    idn_err = idn_nameprep_isunassigned(mNamePrepHandle,
                                        (const uint32_t *) ucs4Buf, &found);
    if (idn_err != idn_success || found)
      return NS_ERROR_FAILURE;
  }

  
  out.Assign(normlizedStr);

  return rv;
}

nsresult nsIDNService::encodeToACE(const nsAString& in, nsACString& out)
{
  
  if (!strcmp("bq--", mACEPrefix))
    return encodeToRACE(mACEPrefix, in, out);
  
  
  return punycode(mACEPrefix, in, out);
}

nsresult nsIDNService::stringPrepAndACE(const nsAString& in, nsACString& out,
                                        bool allowUnassigned,
                                        bool convertAllLabels)
{
  nsresult rv = NS_OK;

  out.Truncate();

  if (in.Length() > kMaxDNSNodeLen) {
    NS_WARNING("IDN node too large");
    return NS_ERROR_FAILURE;
  }

  if (IsASCII(in))
    LossyCopyUTF16toASCII(in, out);
  else if (!convertAllLabels && isLabelSafe(in))
    CopyUTF16toUTF8(in, out);
  else {
    nsAutoString strPrep;
    rv = stringPrep(in, strPrep, allowUnassigned);
    if (NS_SUCCEEDED(rv)) {
      if (IsASCII(strPrep))
        LossyCopyUTF16toASCII(strPrep, out);
      else
        rv = encodeToACE(strPrep, out);
    }
  }

  if (out.Length() > kMaxDNSNodeLen) {
    NS_WARNING("IDN node too large");
    return NS_ERROR_FAILURE;
  }

  return rv;
}







void nsIDNService::normalizeFullStops(nsAString& s)
{
  nsAString::const_iterator start, end;
  s.BeginReading(start); 
  s.EndReading(end); 
  int32_t index = 0;

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

nsresult nsIDNService::decodeACE(const nsACString& in, nsACString& out,
                                 bool allowUnassigned, bool convertAllLabels)
{
  bool isAce;
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
                                                nullptr);
  if (status != punycode_success) {
    delete [] output;
    return NS_ERROR_FAILURE;
  }

  
  output[output_length] = 0;
  nsAutoString utf16;
  ucs4toUtf16(output, utf16);
  delete [] output;
  if (!convertAllLabels && !isLabelSafe(utf16)) {
    out.Assign(in);
    return NS_OK;
  }
  if (!isOnlySafeChars(utf16, mIDNBlacklist))
    return NS_ERROR_FAILURE;
  CopyUTF16toUTF8(utf16, out);

  
  nsAutoCString ace;
  nsresult rv = UTF8toACE(out, ace, allowUnassigned, true);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ace.Equals(in, nsCaseInsensitiveCStringComparator()))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

bool nsIDNService::isInWhitelist(const nsACString &host)
{
  if (mIDNUseWhitelist && mIDNWhitelistPrefBranch) {
    nsAutoCString tld(host);
    
    
    if (!IsASCII(tld) && NS_FAILED(UTF8toACE(tld, tld, false, true))) {
      return false;
    }

    
    tld.Trim(".");
    int32_t pos = tld.RFind(".");
    if (pos == kNotFound)
      return false;

    tld.Cut(0, pos + 1);

    bool safe;
    if (NS_SUCCEEDED(mIDNWhitelistPrefBranch->GetBoolPref(tld.get(), &safe)))
      return safe;
  }

  return false;
}

bool nsIDNService::isLabelSafe(const nsAString &label)
{
  
  NS_ASSERTION(!IsASCII(label), "ASCII label in IDN checking");
  if (mRestrictionProfile == eASCIIOnlyProfile) {
    return false;
  }

  nsAString::const_iterator current, end;
  label.BeginReading(current);
  label.EndReading(end);

  int32_t lastScript = MOZ_SCRIPT_INVALID;
  uint32_t previousChar = 0;
  uint32_t savedNumberingSystem = 0;
  HanVariantType savedHanVariant = HVT_NotHan;

  int32_t savedScript = -1;

  while (current != end) {
    uint32_t ch = *current++;

    if (NS_IS_HIGH_SURROGATE(ch) && current != end &&
        NS_IS_LOW_SURROGATE(*current)) {
      ch = SURROGATE_TO_UCS4(ch, *current++);
    }

    
    XidmodType xm = GetIdentifierModification(ch);
    int32_t script = GetScriptCode(ch);
    if (xm > XIDMOD_RECOMMENDED &&
        !(xm == XIDMOD_LIMITED_USE &&
          (script == MOZ_SCRIPT_CANADIAN_ABORIGINAL ||
           script == MOZ_SCRIPT_MIAO ||
           script == MOZ_SCRIPT_MONGOLIAN ||
           script == MOZ_SCRIPT_TIFINAGH ||
           script == MOZ_SCRIPT_YI))) {
      return false;
    }

    
    if (script != MOZ_SCRIPT_COMMON &&
        script != MOZ_SCRIPT_INHERITED &&
        script != lastScript) {
      if (illegalScriptCombo(script, savedScript)) {
        return false;
      }
      lastScript = script;
    }

    
    if (GetGeneralCategory(ch) ==
        HB_UNICODE_GENERAL_CATEGORY_DECIMAL_NUMBER) {
      uint32_t zeroCharacter = ch - GetNumericValue(ch);
      if (savedNumberingSystem == 0) {
        
        
        savedNumberingSystem = zeroCharacter;
      } else if (zeroCharacter != savedNumberingSystem) {
        return false;
      }
    }

    
    if (previousChar != 0 &&
        previousChar == ch &&
        GetGeneralCategory(ch) == HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK) {
      return false;
    }

    
    HanVariantType hanVariant = GetHanVariant(ch);
    if (hanVariant == HVT_SimplifiedOnly || hanVariant == HVT_TraditionalOnly) {
      if (savedHanVariant == HVT_NotHan) {
        savedHanVariant = hanVariant;
      } else if (hanVariant != savedHanVariant)  {
        return false;
      }
    }

    previousChar = ch;
  }
  return true;
}


static const int32_t scriptTable[] = {
  MOZ_SCRIPT_BOPOMOFO, MOZ_SCRIPT_CYRILLIC, MOZ_SCRIPT_GREEK,
  MOZ_SCRIPT_HANGUL,   MOZ_SCRIPT_HAN,      MOZ_SCRIPT_HIRAGANA,
  MOZ_SCRIPT_KATAKANA, MOZ_SCRIPT_LATIN };

#define BOPO 0
#define CYRL 1
#define GREK 2
#define HANG 3
#define HANI 4
#define HIRA 5
#define KATA 6
#define LATN 7
#define OTHR 8
#define JPAN 9    // Latin + Han + Hiragana + Katakana
#define CHNA 10   // Latin + Han + Bopomofo
#define KORE 11   // Latin + Han + Hangul
#define HNLT 12   // Latin + Han (could be any of the above combinations)
#define FAIL 13

static inline int32_t findScriptIndex(int32_t aScript)
{
  int32_t tableLength = sizeof(scriptTable) / sizeof(int32_t);
  for (int32_t index = 0; index < tableLength; ++index) {
    if (aScript == scriptTable[index]) {
      return index;
    }
  }
  return OTHR;
}

static const int32_t scriptComboTable[13][9] = {


   { BOPO, FAIL, FAIL, FAIL, CHNA, FAIL, FAIL, CHNA, FAIL },
   { FAIL, CYRL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL },
   { FAIL, FAIL, GREK, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL },
   { FAIL, FAIL, FAIL, HANG, KORE, FAIL, FAIL, KORE, FAIL },
   { CHNA, FAIL, FAIL, KORE, HANI, JPAN, JPAN, HNLT, FAIL },
   { FAIL, FAIL, FAIL, FAIL, JPAN, HIRA, JPAN, JPAN, FAIL },
   { FAIL, FAIL, FAIL, FAIL, JPAN, JPAN, KATA, JPAN, FAIL },
   { CHNA, FAIL, FAIL, KORE, HNLT, JPAN, JPAN, LATN, OTHR },
   { FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, OTHR, FAIL },
   { FAIL, FAIL, FAIL, FAIL, JPAN, JPAN, JPAN, JPAN, FAIL },
   { CHNA, FAIL, FAIL, FAIL, CHNA, FAIL, FAIL, CHNA, FAIL },
   { FAIL, FAIL, FAIL, KORE, KORE, FAIL, FAIL, KORE, FAIL },
   { CHNA, FAIL, FAIL, KORE, HNLT, JPAN, JPAN, HNLT, FAIL }
};

bool nsIDNService::illegalScriptCombo(int32_t script, int32_t& savedScript)
{
  if (savedScript == -1) {
    savedScript = findScriptIndex(script);
    return false;
  }

  savedScript = scriptComboTable[savedScript] [findScriptIndex(script)];
  







  return ((savedScript == OTHR &&
           mRestrictionProfile == eHighlyRestrictiveProfile) ||
          savedScript == FAIL);
}

#undef BOPO
#undef CYRL
#undef GREK
#undef HANG
#undef HANI
#undef HIRA
#undef KATA
#undef LATN
#undef OTHR
#undef JPAN
#undef CHNA
#undef KORE
#undef HNLT
#undef FAIL
