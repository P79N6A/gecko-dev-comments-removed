







































#include "nsStandardURL.h"
#include "nsDependentSubstring.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsEscape.h"
#include "nsILocalFile.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsICharsetConverterManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIIDNService.h"
#include "nsNetUtil.h"
#include "prlog.h"
#include "nsAutoPtr.h"
#include "nsIProgrammingLanguage.h"
#include "nsVoidArray.h"

static NS_DEFINE_CID(kThisImplCID, NS_THIS_STANDARDURL_IMPL_CID);
static NS_DEFINE_CID(kStandardURLCID, NS_STANDARDURL_CID);

nsIIDNService *nsStandardURL::gIDN = nsnull;
nsICharsetConverterManager *nsStandardURL::gCharsetMgr = nsnull;
PRBool nsStandardURL::gInitialized = PR_FALSE;
PRBool nsStandardURL::gEscapeUTF8 = PR_TRUE;
PRBool nsStandardURL::gAlwaysEncodeInUTF8 = PR_TRUE;
PRBool nsStandardURL::gEncodeQueryInUTF8 = PR_TRUE;

#if defined(PR_LOGGING)



static PRLogModuleInfo *gStandardURLLog;
#endif
#define LOG(args)     PR_LOG(gStandardURLLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gStandardURLLog, PR_LOG_DEBUG)



#define ENSURE_MUTABLE() \
  PR_BEGIN_MACRO \
    if (!mMutable) { \
        NS_ERROR("attempt to modify an immutable nsStandardURL"); \
        return NS_ERROR_ABORT; \
    } \
  PR_END_MACRO



static nsresult
EncodeString(nsIUnicodeEncoder *encoder, const nsAFlatString &str, nsACString &result)
{
    nsresult rv;
    PRInt32 len = str.Length();
    PRInt32 maxlen;

    rv = encoder->GetMaxLength(str.get(), len, &maxlen);
    if (NS_FAILED(rv))
        return rv;

    char buf[256], *p = buf;
    if (PRUint32(maxlen) > sizeof(buf) - 1) {
        p = (char *) malloc(maxlen + 1);
        if (!p)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = encoder->Convert(str.get(), &len, p, &maxlen);
    if (NS_FAILED(rv))
        goto end;
    if (rv == NS_ERROR_UENC_NOMAPPING) {
        NS_WARNING("unicode conversion failed");
        rv = NS_ERROR_UNEXPECTED;
        goto end;
    }
    p[maxlen] = 0;
    result.Assign(p);

    len = sizeof(buf) - 1;
    rv = encoder->Finish(buf, &len);
    if (NS_FAILED(rv))
        goto end;
    buf[len] = 0;
    result.Append(buf);

end:
    encoder->Reset();

    if (p != buf)
        free(p);
    return rv;
}





#define NS_NET_PREF_ESCAPEUTF8         "network.standard-url.escape-utf8"
#define NS_NET_PREF_ENABLEIDN          "network.enableIDN"
#define NS_NET_PREF_ALWAYSENCODEINUTF8 "network.standard-url.encode-utf8"
#define NS_NET_PREF_ENCODEQUERYINUTF8  "network.standard-url.encode-query-utf8"

NS_IMPL_ISUPPORTS1(nsStandardURL::nsPrefObserver, nsIObserver)

NS_IMETHODIMP nsStandardURL::
nsPrefObserver::Observe(nsISupports *subject,
                        const char *topic,
                        const PRUnichar *data)
{
    if (!strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        nsCOMPtr<nsIPrefBranch> prefBranch( do_QueryInterface(subject) );
        if (prefBranch) {
            PrefsChanged(prefBranch, NS_ConvertUTF16toUTF8(data).get());
        } 
    }
    return NS_OK;
}





nsStandardURL::
nsSegmentEncoder::nsSegmentEncoder(const char *charset)
    : mCharset(charset)
{
}

PRInt32 nsStandardURL::
nsSegmentEncoder::EncodeSegmentCount(const char *str,
                                     const URLSegment &seg,
                                     PRInt16 mask,
                                     nsAFlatCString &result,
                                     PRBool &appended)
{
    appended = PR_FALSE;
    if (!str)
        return 0;
    PRInt32 len = 0;
    if (seg.mLen > 0) {
        PRUint32 pos = seg.mPos;
        len = seg.mLen;

        
        
        
        
        nsCAutoString encBuf;
        if (mCharset && *mCharset && !nsCRT::IsAscii(str + pos, len)) {
            
            if (mEncoder || InitUnicodeEncoder()) {
                NS_ConvertUTF8toUTF16 ucsBuf(Substring(str + pos, str + pos + len));
                if (NS_SUCCEEDED(EncodeString(mEncoder, ucsBuf, encBuf))) {
                    str = encBuf.get();
                    pos = 0;
                    len = encBuf.Length();
                }
                
            }
        }

        
        PRInt16 escapeFlags = (gEscapeUTF8 || mEncoder) ? 0 : esc_OnlyASCII;

        PRUint32 initLen = result.Length();

        
        if (NS_EscapeURL(str + pos, len, mask | escapeFlags, result)) {
            len = result.Length() - initLen;
            appended = PR_TRUE;
        }
        else if (str == encBuf.get()) {
            result += encBuf; 
            len = encBuf.Length();
            appended = PR_TRUE;
        }
    }
    return len;
}

const nsACString &nsStandardURL::
nsSegmentEncoder::EncodeSegment(const nsASingleFragmentCString &str,
                                PRInt16 mask,
                                nsAFlatCString &result)
{
    const char *text;
    PRBool encoded;
    EncodeSegmentCount(str.BeginReading(text), URLSegment(0, str.Length()), mask, result, encoded);
    if (encoded)
        return result;
    return str;
}

PRBool nsStandardURL::
nsSegmentEncoder::InitUnicodeEncoder()
{
    NS_ASSERTION(!mEncoder, "Don't call this if we have an encoder already!");
    nsresult rv;
    if (!gCharsetMgr) {
        rv = CallGetService("@mozilla.org/charset-converter-manager;1",
                            &gCharsetMgr);
        if (NS_FAILED(rv)) {
            NS_ERROR("failed to get charset-converter-manager");
            return PR_FALSE;
        }
    }

    rv = gCharsetMgr->GetUnicodeEncoder(mCharset, getter_AddRefs(mEncoder));
    if (NS_FAILED(rv)) {
        NS_ERROR("failed to get unicode encoder");
        mEncoder = 0; 
        return PR_FALSE;
    }

    return PR_TRUE;
}

#define GET_SEGMENT_ENCODER_INTERNAL(name, useUTF8) \
    nsSegmentEncoder name(useUTF8 ? nsnull : mOriginCharset.get())

#define GET_SEGMENT_ENCODER(name) \
    GET_SEGMENT_ENCODER_INTERNAL(name, gAlwaysEncodeInUTF8)

#define GET_QUERY_ENCODER(name) \
    GET_SEGMENT_ENCODER_INTERNAL(name, gAlwaysEncodeInUTF8 && \
                                 gEncodeQueryInUTF8)





#ifdef DEBUG_DUMP_URLS_AT_SHUTDOWN
static PRCList gAllURLs;
#endif

nsStandardURL::nsStandardURL(PRBool aSupportsFileURL)
    : mDefaultPort(-1)
    , mPort(-1)
    , mHostA(nsnull)
    , mHostEncoding(eEncoding_ASCII)
    , mSpecEncoding(eEncoding_Unknown)
    , mURLType(URLTYPE_STANDARD)
    , mMutable(PR_TRUE)
    , mSupportsFileURL(aSupportsFileURL)
{
#if defined(PR_LOGGING)
    if (!gStandardURLLog)
        gStandardURLLog = PR_NewLogModule("nsStandardURL");
#endif

    LOG(("Creating nsStandardURL @%p\n", this));

    if (!gInitialized) {
        gInitialized = PR_TRUE;
        InitGlobalObjects();
    }

    
    mParser = net_GetStdURLParser();

#ifdef DEBUG_DUMP_URLS_AT_SHUTDOWN
    PR_APPEND_LINK(&mDebugCList, &gAllURLs);
#endif
}

nsStandardURL::~nsStandardURL()
{
    LOG(("Destroying nsStandardURL @%p\n", this));

    CRTFREEIF(mHostA);
#ifdef DEBUG_DUMP_URLS_AT_SHUTDOWN
    PR_REMOVE_LINK(&mDebugCList);
#endif
}

#ifdef DEBUG_DUMP_URLS_AT_SHUTDOWN
static void DumpLeakedURLs()
{
    if (!PR_CLIST_IS_EMPTY(&gAllURLs)) {
        printf("Leaked URLs:\n");
        for (PRCList *l = PR_LIST_HEAD(&gAllURLs); l != &gAllURLs; l = PR_NEXT_LINK(l)) {
            nsStandardURL *url = reinterpret_cast<nsStandardURL*>(reinterpret_cast<char*>(l) - offsetof(nsStandardURL, mDebugCList));
            url->PrintSpec();
        }
    }
}
#endif

void
nsStandardURL::InitGlobalObjects()
{
    nsCOMPtr<nsIPrefBranch2> prefBranch( do_GetService(NS_PREFSERVICE_CONTRACTID) );
    if (prefBranch) {
        nsCOMPtr<nsIObserver> obs( new nsPrefObserver() );
        prefBranch->AddObserver(NS_NET_PREF_ESCAPEUTF8, obs.get(), PR_FALSE);
        prefBranch->AddObserver(NS_NET_PREF_ALWAYSENCODEINUTF8, obs.get(), PR_FALSE);
        prefBranch->AddObserver(NS_NET_PREF_ENCODEQUERYINUTF8, obs.get(), PR_FALSE);
        prefBranch->AddObserver(NS_NET_PREF_ENABLEIDN, obs.get(), PR_FALSE);

        PrefsChanged(prefBranch, nsnull);
    }

#ifdef DEBUG_DUMP_URLS_AT_SHUTDOWN
    PR_INIT_CLIST(&gAllURLs);
    atexit(DumpLeakedURLs);
#endif
}

void
nsStandardURL::ShutdownGlobalObjects()
{
    NS_IF_RELEASE(gIDN);
    NS_IF_RELEASE(gCharsetMgr);
}





void
nsStandardURL::Clear()
{
    mSpec.Truncate();

    mPort = -1;

    mAuthority.Reset();
    mUsername.Reset();
    mPassword.Reset();
    mHost.Reset();
    mHostEncoding = eEncoding_ASCII;

    mPath.Reset();
    mFilepath.Reset();
    mDirectory.Reset();
    mBasename.Reset();

    mExtension.Reset();
    mParam.Reset();
    mQuery.Reset();
    mRef.Reset();

    InvalidateCache();
}

void
nsStandardURL::InvalidateCache(PRBool invalidateCachedFile)
{
    if (invalidateCachedFile)
        mFile = 0;
    CRTFREEIF(mHostA);
    mSpecEncoding = eEncoding_Unknown;
}

PRBool
nsStandardURL::EscapeIPv6(const char *host, nsCString &result)
{
    
    if (host && (host[0] != '[') && PL_strchr(host, ':')) {
        result.Assign('[');
        result.Append(host);
        result.Append(']');
        return PR_TRUE;
    }
    return PR_FALSE;
}

PRBool
nsStandardURL::NormalizeIDN(const nsCSubstring &host, nsCString &result)
{
    
    

    

    
    
    
    
    

    NS_ASSERTION(mHostEncoding == eEncoding_ASCII, "unexpected default encoding");

    PRBool isASCII;
    if (gIDN &&
        NS_SUCCEEDED(gIDN->ConvertToDisplayIDN(host, &isASCII, result))) {
        if (!isASCII)
          mHostEncoding = eEncoding_UTF8;

        return PR_TRUE;
    }

    result.Truncate();
    return PR_FALSE;
}

void
nsStandardURL::CoalescePath(netCoalesceFlags coalesceFlag, char *path)
{
    net_CoalesceDirs(coalesceFlag, path);
    PRInt32 newLen = strlen(path);
    if (newLen < mPath.mLen) {
        PRInt32 diff = newLen - mPath.mLen;
        mPath.mLen = newLen;
        mDirectory.mLen += diff;
        mFilepath.mLen += diff;
        ShiftFromBasename(diff);
    }
}

PRUint32
nsStandardURL::AppendSegmentToBuf(char *buf, PRUint32 i, const char *str, URLSegment &seg, const nsCString *escapedStr, PRBool useEscaped)
{
    if (seg.mLen > 0) {
        if (useEscaped) {
            seg.mLen = escapedStr->Length();
            memcpy(buf + i, escapedStr->get(), seg.mLen);
        }
        else
            memcpy(buf + i, str + seg.mPos, seg.mLen);
        seg.mPos = i;
        i += seg.mLen;
    }
    return i;
}

PRUint32
nsStandardURL::AppendToBuf(char *buf, PRUint32 i, const char *str, PRUint32 len)
{
    memcpy(buf + i, str, len);
    return i + len;
}






nsresult
nsStandardURL::BuildNormalizedSpec(const char *spec)
{
    
    

    
    
    nsCAutoString encUsername, encPassword, encHost, encDirectory,
      encBasename, encExtension, encParam, encQuery, encRef;
    PRBool useEncUsername, useEncPassword, useEncHost, useEncDirectory,
      useEncBasename, useEncExtension, useEncParam, useEncQuery, useEncRef;

    
    
    
    
    PRUint32 approxLen = 3; 

    
    if (mScheme.mLen > 0)
        approxLen += mScheme.mLen;

    
    
    
    {
        GET_SEGMENT_ENCODER(encoder);
        GET_QUERY_ENCODER(queryEncoder);
        approxLen += encoder.EncodeSegmentCount(spec, mUsername,  esc_Username,      encUsername,  useEncUsername);
        approxLen += encoder.EncodeSegmentCount(spec, mPassword,  esc_Password,      encPassword,  useEncPassword);
        approxLen += encoder.EncodeSegmentCount(spec, mDirectory, esc_Directory,     encDirectory, useEncDirectory);
        approxLen += encoder.EncodeSegmentCount(spec, mBasename,  esc_FileBaseName,  encBasename,  useEncBasename);
        approxLen += encoder.EncodeSegmentCount(spec, mExtension, esc_FileExtension, encExtension, useEncExtension);
        approxLen += encoder.EncodeSegmentCount(spec, mParam,     esc_Param,         encParam,     useEncParam);
        approxLen += queryEncoder.EncodeSegmentCount(spec, mQuery, esc_Query,        encQuery,     useEncQuery);
        approxLen += encoder.EncodeSegmentCount(spec, mRef,       esc_Ref,           encRef,       useEncRef);
    }

    
    
    
    mHostEncoding = eEncoding_ASCII;
    if (mHost.mLen > 0) {
        const nsCSubstring& tempHost =
            Substring(spec + mHost.mPos, spec + mHost.mPos + mHost.mLen);
        if (tempHost.FindChar('\0') != kNotFound)
            return NS_ERROR_MALFORMED_URI;  
        if (tempHost.FindChar(' ') != kNotFound)
            return NS_ERROR_MALFORMED_URI;  
        if ((useEncHost = NormalizeIDN(tempHost, encHost)))
            approxLen += encHost.Length();
        else
            approxLen += mHost.mLen;
    }

    
    
    
    if (!EnsureStringLength(mSpec, approxLen + 32))
        return NS_ERROR_OUT_OF_MEMORY;
    char *buf;
    mSpec.BeginWriting(buf);
    PRUint32 i = 0;

    if (mScheme.mLen > 0) {
        i = AppendSegmentToBuf(buf, i, spec, mScheme);
        net_ToLowerCase(buf + mScheme.mPos, mScheme.mLen);
        i = AppendToBuf(buf, i, "://", 3);
    }

    
    mAuthority.mPos = i;

    
    if (mUsername.mLen > 0) {
        i = AppendSegmentToBuf(buf, i, spec, mUsername, &encUsername, useEncUsername);
        if (mPassword.mLen >= 0) {
            buf[i++] = ':';
            i = AppendSegmentToBuf(buf, i, spec, mPassword, &encPassword, useEncPassword);
        }
        buf[i++] = '@';
    }
    if (mHost.mLen > 0) {
        i = AppendSegmentToBuf(buf, i, spec, mHost, &encHost, useEncHost);
        net_ToLowerCase(buf + mHost.mPos, mHost.mLen);
        if (mPort != -1 && mPort != mDefaultPort) {
            nsCAutoString portbuf;
            portbuf.AppendInt(mPort);
            buf[i++] = ':';
            i = AppendToBuf(buf, i, portbuf.get(), portbuf.Length());
        }
    }

    
    mAuthority.mLen = i - mAuthority.mPos;

    
    if (mPath.mLen <= 0) {
        LOG(("setting path=/"));
        mDirectory.mPos = mFilepath.mPos = mPath.mPos = i;
        mDirectory.mLen = mFilepath.mLen = mPath.mLen = 1;
        
        mBasename.mPos = i+1;
        mBasename.mLen = 0;
        buf[i++] = '/';
    }
    else {
        PRUint32 leadingSlash = 0;
        if (spec[mPath.mPos] != '/') {
            LOG(("adding leading slash to path\n"));
            leadingSlash = 1;
            buf[i++] = '/';
            
            if (mBasename.mLen == -1) {
                mBasename.mPos = i;
                mBasename.mLen = 0;
            }
        }

        
        mPath.mPos = mFilepath.mPos = i - leadingSlash;

        i = AppendSegmentToBuf(buf, i, spec, mDirectory, &encDirectory, useEncDirectory);

        
        if (buf[i-1] != '/') {
            buf[i++] = '/';
            mDirectory.mLen++;
        }

        i = AppendSegmentToBuf(buf, i, spec, mBasename, &encBasename, useEncBasename);

        
        if (leadingSlash) {
            mDirectory.mPos = mPath.mPos;
            if (mDirectory.mLen >= 0)
                mDirectory.mLen += leadingSlash;
            else
                mDirectory.mLen = 1;
        }

        if (mExtension.mLen >= 0) {
            buf[i++] = '.';
            i = AppendSegmentToBuf(buf, i, spec, mExtension, &encExtension, useEncExtension);
        }
        
        mFilepath.mLen = i - mFilepath.mPos;

        if (mParam.mLen >= 0) {
            buf[i++] = ';';
            i = AppendSegmentToBuf(buf, i, spec, mParam, &encParam, useEncParam);
        }
        if (mQuery.mLen >= 0) {
            buf[i++] = '?';
            i = AppendSegmentToBuf(buf, i, spec, mQuery, &encQuery, useEncQuery);
        }
        if (mRef.mLen >= 0) {
            buf[i++] = '#';
            i = AppendSegmentToBuf(buf, i, spec, mRef, &encRef, useEncRef);
        }
        
        mPath.mLen = i - mPath.mPos;
    }

    buf[i] = '\0';

    if (mDirectory.mLen > 1) {
        netCoalesceFlags coalesceFlag = NET_COALESCE_NORMAL;
        if (SegmentIs(buf,mScheme,"ftp")) {
            coalesceFlag = (netCoalesceFlags) (coalesceFlag 
                                        | NET_COALESCE_ALLOW_RELATIVE_ROOT
                                        | NET_COALESCE_DOUBLE_SLASH_IS_ROOT);
        }
        CoalescePath(coalesceFlag, buf + mDirectory.mPos);
    }
    mSpec.SetLength(strlen(buf));
    NS_ASSERTION(mSpec.Length() <= approxLen+32, "We've overflowed the mSpec buffer!");
    return NS_OK;
}

PRBool
nsStandardURL::SegmentIs(const URLSegment &seg, const char *val, PRBool ignoreCase)
{
    
    if (!val || mSpec.IsEmpty())
        return (!val && (mSpec.IsEmpty() || seg.mLen < 0));
    if (seg.mLen < 0)
        return PR_FALSE;
    
    
    if (ignoreCase)
        return !PL_strncasecmp(mSpec.get() + seg.mPos, val, seg.mLen)
            && (val[seg.mLen] == '\0');
    else
        return !strncmp(mSpec.get() + seg.mPos, val, seg.mLen)
            && (val[seg.mLen] == '\0');
}

PRBool
nsStandardURL::SegmentIs(const char* spec, const URLSegment &seg, const char *val, PRBool ignoreCase)
{
    
    if (!val || !spec)
        return (!val && (!spec || seg.mLen < 0));
    if (seg.mLen < 0)
        return PR_FALSE;
    
    
    if (ignoreCase)
        return !PL_strncasecmp(spec + seg.mPos, val, seg.mLen)
            && (val[seg.mLen] == '\0');
    else
        return !strncmp(spec + seg.mPos, val, seg.mLen)
            && (val[seg.mLen] == '\0');
}

PRBool
nsStandardURL::SegmentIs(const URLSegment &seg1, const char *val, const URLSegment &seg2, PRBool ignoreCase)
{
    if (seg1.mLen != seg2.mLen)
        return PR_FALSE;
    if (seg1.mLen == -1 || (!val && mSpec.IsEmpty()))
        return PR_TRUE; 
    if (ignoreCase)
        return !PL_strncasecmp(mSpec.get() + seg1.mPos, val + seg2.mPos, seg1.mLen); 
    else
        return !strncmp(mSpec.get() + seg1.mPos, val + seg2.mPos, seg1.mLen); 
}

PRInt32
nsStandardURL::ReplaceSegment(PRUint32 pos, PRUint32 len, const char *val, PRUint32 valLen)
{
    if (val && valLen) {
        if (len == 0)
            mSpec.Insert(val, pos, valLen);
        else
            mSpec.Replace(pos, len, nsDependentCString(val, valLen));
        return valLen - len;
    }

    
    mSpec.Cut(pos, len);
    return -PRInt32(len);
}

PRInt32
nsStandardURL::ReplaceSegment(PRUint32 pos, PRUint32 len, const nsACString &val)
{
    if (len == 0)
        mSpec.Insert(val, pos);
    else
        mSpec.Replace(pos, len, val);
    return val.Length() - len;
}

nsresult
nsStandardURL::ParseURL(const char *spec, PRInt32 specLen)
{
    nsresult rv;

    
    
    
    rv = mParser->ParseURL(spec, specLen,
                           &mScheme.mPos, &mScheme.mLen,
                           &mAuthority.mPos, &mAuthority.mLen,
                           &mPath.mPos, &mPath.mLen);
    if (NS_FAILED(rv)) return rv;

#ifdef DEBUG
    if (mScheme.mLen <= 0) {
        printf("spec=%s\n", spec);
        NS_WARNING("malformed url: no scheme");
    }
#endif
     
    if (mAuthority.mLen > 0) {
        rv = mParser->ParseAuthority(spec + mAuthority.mPos, mAuthority.mLen,
                                     &mUsername.mPos, &mUsername.mLen,
                                     &mPassword.mPos, &mPassword.mLen,
                                     &mHost.mPos, &mHost.mLen,
                                     &mPort);
        if (NS_FAILED(rv)) return rv;

        
        if (mPort == mDefaultPort)
            mPort = -1;

        mUsername.mPos += mAuthority.mPos;
        mPassword.mPos += mAuthority.mPos;
        mHost.mPos += mAuthority.mPos;
    }

    if (mPath.mLen > 0)
        rv = ParsePath(spec, mPath.mPos, mPath.mLen);

    return rv;
}

nsresult
nsStandardURL::ParsePath(const char *spec, PRUint32 pathPos, PRInt32 pathLen)
{
    nsresult rv = mParser->ParsePath(spec + pathPos, pathLen,
                                     &mFilepath.mPos, &mFilepath.mLen,
                                     &mParam.mPos, &mParam.mLen,
                                     &mQuery.mPos, &mQuery.mLen,
                                     &mRef.mPos, &mRef.mLen);
    if (NS_FAILED(rv)) return rv;

    mFilepath.mPos += pathPos;
    mParam.mPos += pathPos;
    mQuery.mPos += pathPos;
    mRef.mPos += pathPos;

    if (mFilepath.mLen > 0) {
        rv = mParser->ParseFilePath(spec + mFilepath.mPos, mFilepath.mLen,
                                    &mDirectory.mPos, &mDirectory.mLen,
                                    &mBasename.mPos, &mBasename.mLen,
                                    &mExtension.mPos, &mExtension.mLen);
        if (NS_FAILED(rv)) return rv;

        mDirectory.mPos += mFilepath.mPos;
        mBasename.mPos += mFilepath.mPos;
        mExtension.mPos += mFilepath.mPos;
    }
    return NS_OK;
}

char *
nsStandardURL::AppendToSubstring(PRUint32 pos,
                                 PRInt32 len,
                                 const char *tail,
                                 PRInt32 tailLen)
{
    if (tailLen < 0)
        tailLen = strlen(tail);

    char *result = (char *) NS_Alloc(len + tailLen + 1);
    if (result) {
        memcpy(result, mSpec.get() + pos, len);
        memcpy(result + len, tail, tailLen);
        result[len + tailLen] = '\0';
    }
    return result;
}

nsresult
nsStandardURL::ReadSegment(nsIBinaryInputStream *stream, URLSegment &seg)
{
    nsresult rv;

    rv = stream->Read32(&seg.mPos);
    if (NS_FAILED(rv)) return rv;

    rv = stream->Read32((PRUint32 *) &seg.mLen);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsStandardURL::WriteSegment(nsIBinaryOutputStream *stream, const URLSegment &seg)
{
    nsresult rv;

    rv = stream->Write32(seg.mPos);
    if (NS_FAILED(rv)) return rv;

    rv = stream->Write32(PRUint32(seg.mLen));
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

 void
nsStandardURL::PrefsChanged(nsIPrefBranch *prefs, const char *pref)
{
    PRBool val;

    LOG(("nsStandardURL::PrefsChanged [pref=%s]\n", pref));

#define PREF_CHANGED(p) ((pref == nsnull) || !strcmp(pref, p))
#define GOT_PREF(p, b) (NS_SUCCEEDED(prefs->GetBoolPref(p, &b)))

    if (PREF_CHANGED(NS_NET_PREF_ENABLEIDN)) {
        NS_IF_RELEASE(gIDN);
        if (GOT_PREF(NS_NET_PREF_ENABLEIDN, val) && val) {
            
            nsCOMPtr<nsIIDNService> serv(do_GetService(NS_IDNSERVICE_CONTRACTID));
            if (serv)
                NS_ADDREF(gIDN = serv.get());
        }
        LOG(("IDN support %s\n", gIDN ? "enabled" : "disabled"));
    }
    
    if (PREF_CHANGED(NS_NET_PREF_ESCAPEUTF8)) {
        if (GOT_PREF(NS_NET_PREF_ESCAPEUTF8, val))
            gEscapeUTF8 = val;
        LOG(("escape UTF-8 %s\n", gEscapeUTF8 ? "enabled" : "disabled"));
    }

    if (PREF_CHANGED(NS_NET_PREF_ALWAYSENCODEINUTF8)) {
        if (GOT_PREF(NS_NET_PREF_ALWAYSENCODEINUTF8, val))
            gAlwaysEncodeInUTF8 = val;
        LOG(("encode in UTF-8 %s\n", gAlwaysEncodeInUTF8 ? "enabled" : "disabled"));
    }

    if (PREF_CHANGED(NS_NET_PREF_ENCODEQUERYINUTF8)) {
        if (GOT_PREF(NS_NET_PREF_ENCODEQUERYINUTF8, val))
            gEncodeQueryInUTF8 = val;
        LOG(("encode query in UTF-8 %s\n", gEncodeQueryInUTF8 ? "enabled" : "disabled"));
    }
#undef PREF_CHANGED
#undef GOT_PREF
}





NS_IMPL_ADDREF(nsStandardURL)
NS_IMPL_RELEASE(nsStandardURL)

NS_INTERFACE_MAP_BEGIN(nsStandardURL)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStandardURL)
    NS_INTERFACE_MAP_ENTRY(nsIURI)
    NS_INTERFACE_MAP_ENTRY(nsIURL)
    NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIFileURL, mSupportsFileURL)
    NS_INTERFACE_MAP_ENTRY(nsIStandardURL)
    NS_INTERFACE_MAP_ENTRY(nsISerializable)
    NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
    NS_INTERFACE_MAP_ENTRY(nsIMutable)
    
    if (aIID.Equals(kThisImplCID))
        foundInterface = static_cast<nsIURI *>(this);
    else
NS_INTERFACE_MAP_END






NS_IMETHODIMP
nsStandardURL::GetSpec(nsACString &result)
{
    result = mSpec;
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetPrePath(nsACString &result)
{
    result = Prepath();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetScheme(nsACString &result)
{
    result = Scheme();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetUserPass(nsACString &result)
{
    result = Userpass();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetUsername(nsACString &result)
{
    result = Username();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetPassword(nsACString &result)
{
    result = Password();
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::GetHostPort(nsACString &result)
{
    result = Hostport();
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::GetHost(nsACString &result)
{
    result = Host();
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::GetPort(PRInt32 *result)
{
    *result = mPort;
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetPath(nsACString &result)
{
    result = Path();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetAsciiSpec(nsACString &result)
{
    if (mSpecEncoding == eEncoding_Unknown) {
        if (IsASCII(mSpec))
            mSpecEncoding = eEncoding_ASCII;
        else
            mSpecEncoding = eEncoding_UTF8;
    }

    if (mSpecEncoding == eEncoding_ASCII) {
        result = mSpec;
        return NS_OK;
    }

    
    result.SetCapacity(mSpec.Length() + PR_MIN(32, mSpec.Length()/10));

    result = Substring(mSpec, 0, mScheme.mLen + 3);

    NS_EscapeURL(Userpass(PR_TRUE), esc_OnlyNonASCII | esc_AlwaysCopy, result);

    
    nsCAutoString escHostport;
    if (mHost.mLen > 0) {
        
        (void) GetAsciiHost(escHostport);

        
        PRUint32 pos = mHost.mPos + mHost.mLen;
        if (pos < mPath.mPos)
            escHostport += Substring(mSpec, pos, mPath.mPos - pos);
    }
    result += escHostport;

    NS_EscapeURL(Path(), esc_OnlyNonASCII | esc_AlwaysCopy, result);
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetAsciiHost(nsACString &result)
{
    if (mHostEncoding == eEncoding_ASCII) {
        result = Host();
        return NS_OK;
    }

    
    if (mHostA) {
        result = mHostA;
        return NS_OK;
    }

    if (gIDN) {
        nsresult rv;
        rv = gIDN->ConvertUTF8toACE(Host(), result);
        if (NS_SUCCEEDED(rv)) {
            mHostA = ToNewCString(result);
            return NS_OK;
        }
        NS_WARNING("nsIDNService::ConvertUTF8toACE failed");
    }

    
    NS_EscapeURL(Host(), esc_OnlyNonASCII | esc_AlwaysCopy, result);
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::GetOriginCharset(nsACString &result)
{
    if (mOriginCharset.IsEmpty())
        result.AssignLiteral("UTF-8");
    else
        result = mOriginCharset;
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetSpec(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *spec = flat.get();
    PRInt32 specLength = flat.Length();

    LOG(("nsStandardURL::SetSpec [spec=%s]\n", spec));

    Clear();

    if (!spec || !*spec)
        return NS_OK;

    
    nsCAutoString buf1;
    if (net_FilterURIString(spec, buf1)) {
        spec = buf1.get();
        specLength = buf1.Length();
    }

    
    nsresult rv = ParseURL(spec, specLength);
    if (NS_FAILED(rv)) return rv;

    
    
    rv = BuildNormalizedSpec(spec);

#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        LOG((" spec      = %s\n", mSpec.get()));
        LOG((" port      = %d\n", mPort));
        LOG((" scheme    = (%u,%d)\n", mScheme.mPos,    mScheme.mLen));
        LOG((" authority = (%u,%d)\n", mAuthority.mPos, mAuthority.mLen));
        LOG((" username  = (%u,%d)\n", mUsername.mPos,  mUsername.mLen));
        LOG((" password  = (%u,%d)\n", mPassword.mPos,  mPassword.mLen));
        LOG((" hostname  = (%u,%d)\n", mHost.mPos,      mHost.mLen));
        LOG((" path      = (%u,%d)\n", mPath.mPos,      mPath.mLen));
        LOG((" filepath  = (%u,%d)\n", mFilepath.mPos,  mFilepath.mLen));
        LOG((" directory = (%u,%d)\n", mDirectory.mPos, mDirectory.mLen));
        LOG((" basename  = (%u,%d)\n", mBasename.mPos,  mBasename.mLen));
        LOG((" extension = (%u,%d)\n", mExtension.mPos, mExtension.mLen));
        LOG((" param     = (%u,%d)\n", mParam.mPos,     mParam.mLen));
        LOG((" query     = (%u,%d)\n", mQuery.mPos,     mQuery.mLen));
        LOG((" ref       = (%u,%d)\n", mRef.mPos,       mRef.mLen));
    }
#endif
    return rv;
}

NS_IMETHODIMP
nsStandardURL::SetScheme(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &scheme = PromiseFlatCString(input);

    LOG(("nsStandardURL::SetScheme [scheme=%s]\n", scheme.get()));

    if (scheme.IsEmpty()) {
        NS_ERROR("cannot remove the scheme from an url");
        return NS_ERROR_UNEXPECTED;
    }
    if (mScheme.mLen < 0) {
        NS_ERROR("uninitialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    if (!net_IsValidScheme(scheme)) {
        NS_ERROR("the given url scheme contains invalid characters");
        return NS_ERROR_UNEXPECTED;
    }

    InvalidateCache();

    PRInt32 shift = ReplaceSegment(mScheme.mPos, mScheme.mLen, scheme);

    if (shift) {
        mScheme.mLen = scheme.Length();
        ShiftFromAuthority(shift);
    }

    
    
    
    
    net_ToLowerCase((char *) mSpec.get(), mScheme.mLen);
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetUserPass(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &userpass = PromiseFlatCString(input);

    LOG(("nsStandardURL::SetUserPass [userpass=%s]\n", userpass.get()));

    if (mURLType == URLTYPE_NO_AUTHORITY) {
        if (userpass.IsEmpty())
            return NS_OK;
        NS_ERROR("cannot set user:pass on no-auth url");
        return NS_ERROR_UNEXPECTED;
    }
    if (mAuthority.mLen < 0) {
        NS_ERROR("uninitialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    InvalidateCache();

    if (userpass.IsEmpty()) {
        
        if (mUsername.mLen > 0) {
            if (mPassword.mLen > 0)
                mUsername.mLen += (mPassword.mLen + 1);
            mUsername.mLen++;
            mSpec.Cut(mUsername.mPos, mUsername.mLen);
            mAuthority.mLen -= mUsername.mLen;
            ShiftFromHost(-mUsername.mLen);
            mUsername.mLen = -1;
            mPassword.mLen = -1;
        }
        return NS_OK;
    }

    NS_ASSERTION(mHost.mLen >= 0, "uninitialized");

    nsresult rv;
    PRUint32 usernamePos, passwordPos;
    PRInt32 usernameLen, passwordLen;

    rv = mParser->ParseUserInfo(userpass.get(), userpass.Length(),
                                &usernamePos, &usernameLen,
                                &passwordPos, &passwordLen);
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString buf;
    if (usernameLen > 0) {
        GET_SEGMENT_ENCODER(encoder);
        PRBool ignoredOut;
        usernameLen = encoder.EncodeSegmentCount(userpass.get(),
                                                 URLSegment(usernamePos,
                                                            usernameLen),
                                                 esc_Username | esc_AlwaysCopy,
                                                 buf, ignoredOut);
        if (passwordLen >= 0) {
            buf.Append(':');
            passwordLen = encoder.EncodeSegmentCount(userpass.get(),
                                                     URLSegment(passwordPos,
                                                                passwordLen),
                                                     esc_Password |
                                                     esc_AlwaysCopy, buf,
                                                     ignoredOut);
        }
        if (mUsername.mLen < 0)
            buf.Append('@');
    }

    PRUint32 shift = 0;

    if (mUsername.mLen < 0) {
        
        if (!buf.IsEmpty()) {
            mSpec.Insert(buf, mHost.mPos);
            mUsername.mPos = mHost.mPos;
            shift = buf.Length();
        }
    }
    else {
        
        PRUint32 userpassLen = mUsername.mLen;
        if (mPassword.mLen >= 0)
            userpassLen += (mPassword.mLen + 1);
        mSpec.Replace(mUsername.mPos, userpassLen, buf);
        shift = buf.Length() - userpassLen;
    }
    if (shift) {
        ShiftFromHost(shift);
        mAuthority.mLen += shift;
    }
    
    mUsername.mLen = usernameLen;
    mPassword.mLen = passwordLen;
    if (passwordLen)
        mPassword.mPos = mUsername.mPos + mUsername.mLen + 1;
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetUsername(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &username = PromiseFlatCString(input);

    LOG(("nsStandardURL::SetUsername [username=%s]\n", username.get()));

    if (mURLType == URLTYPE_NO_AUTHORITY) {
        if (username.IsEmpty())
            return NS_OK;
        NS_ERROR("cannot set username on no-auth url");
        return NS_ERROR_UNEXPECTED;
    }

    if (username.IsEmpty())
        return SetUserPass(username);

    InvalidateCache();

    
    nsCAutoString buf;
    GET_SEGMENT_ENCODER(encoder);
    const nsACString &escUsername =
        encoder.EncodeSegment(username, esc_Username, buf);

    PRInt32 shift;

    if (mUsername.mLen < 0) {
        mUsername.mPos = mAuthority.mPos;
        mSpec.Insert(escUsername + NS_LITERAL_CSTRING("@"), mUsername.mPos);
        shift = escUsername.Length() + 1;
    }
    else
        shift = ReplaceSegment(mUsername.mPos, mUsername.mLen, escUsername);

    if (shift) {
        mUsername.mLen = escUsername.Length();
        mAuthority.mLen += shift;
        ShiftFromPassword(shift);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetPassword(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &password = PromiseFlatCString(input);

    LOG(("nsStandardURL::SetPassword [password=%s]\n", password.get()));

    if (mURLType == URLTYPE_NO_AUTHORITY) {
        if (password.IsEmpty())
            return NS_OK;
        NS_ERROR("cannot set password on no-auth url");
        return NS_ERROR_UNEXPECTED;
    }
    if (mUsername.mLen <= 0) {
        NS_ERROR("cannot set password without existing username");
        return NS_ERROR_FAILURE;
    }

    InvalidateCache();

    if (password.IsEmpty()) {
        if (mPassword.mLen >= 0) {
            
            mSpec.Cut(mPassword.mPos - 1, mPassword.mLen + 1);
            ShiftFromHost(-(mPassword.mLen + 1));
            mAuthority.mLen -= (mPassword.mLen + 1);
            mPassword.mLen = -1;
        }
        return NS_OK;
    }

    
    nsCAutoString buf;
    GET_SEGMENT_ENCODER(encoder);
    const nsACString &escPassword =
        encoder.EncodeSegment(password, esc_Password, buf);

    PRInt32 shift;

    if (mPassword.mLen < 0) {
        mPassword.mPos = mUsername.mPos + mUsername.mLen + 1;
        mSpec.Insert(NS_LITERAL_CSTRING(":") + escPassword, mPassword.mPos - 1);
        shift = escPassword.Length() + 1;
    }
    else
        shift = ReplaceSegment(mPassword.mPos, mPassword.mLen, escPassword);

    if (shift) {
        mPassword.mLen = escPassword.Length();
        mAuthority.mLen += shift;
        ShiftFromHost(shift);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetHostPort(const nsACString &value)
{
    ENSURE_MUTABLE();

    
    NS_NOTREACHED("not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsStandardURL::SetHost(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *host = flat.get();

    LOG(("nsStandardURL::SetHost [host=%s]\n", host));

    if (mURLType == URLTYPE_NO_AUTHORITY) {
        if (flat.IsEmpty())
            return NS_OK;
        NS_WARNING("cannot set host on no-auth url");
        return NS_ERROR_UNEXPECTED;
    }

    if (host && strlen(host) < flat.Length())
        return NS_ERROR_MALFORMED_URI; 

    
    
    if (strchr(host, ' '))
        return NS_ERROR_MALFORMED_URI;

    InvalidateCache();
    mHostEncoding = eEncoding_ASCII;

    if (!(host && *host)) {
        
        if (mHost.mLen > 0) {
            
            mSpec.Cut(mAuthority.mPos, mAuthority.mLen);
            ShiftFromPath(-mAuthority.mLen);
            mAuthority.mLen = 0;
            mUsername.mLen = -1;
            mPassword.mLen = -1;
            mHost.mLen = -1;
            mPort = -1;
        }
        return NS_OK;
    }

    
    PRInt32 len;
    nsCAutoString hostBuf;
    if (EscapeIPv6(host, hostBuf)) {
        host = hostBuf.get();
        len = hostBuf.Length();
    }
    else if (NormalizeIDN(flat, hostBuf)) {
        host = hostBuf.get();
        len = hostBuf.Length();
    }
    else
        len = flat.Length();

    if (mHost.mLen < 0) {
        mHost.mPos = mAuthority.mPos;
        mHost.mLen = 0;
    }

    PRInt32 shift = ReplaceSegment(mHost.mPos, mHost.mLen, host, len);

    if (shift) {
        mHost.mLen = len;
        mAuthority.mLen += shift;
        ShiftFromPath(shift);
    }

    
    net_ToLowerCase(mSpec.BeginWriting() + mHost.mPos, mHost.mLen);

    return NS_OK;
}
             
NS_IMETHODIMP
nsStandardURL::SetPort(PRInt32 port)
{
    ENSURE_MUTABLE();

    LOG(("nsStandardURL::SetPort [port=%d]\n", port));

    if ((port == mPort) || (mPort == -1 && port == mDefaultPort))
        return NS_OK;

    if (mURLType == URLTYPE_NO_AUTHORITY) {
        NS_WARNING("cannot set port on no-auth url");
        return NS_ERROR_UNEXPECTED;
    }

    InvalidateCache();

    if (mPort == -1) {
        
        nsCAutoString buf;
        buf.Assign(':');
        buf.AppendInt(port);
        mSpec.Insert(buf, mHost.mPos + mHost.mLen);
        mAuthority.mLen += buf.Length();
        ShiftFromPath(buf.Length());
    }
    else if (port == -1 || port == mDefaultPort) {
        
        port = -1;

        
        PRUint32 start = mHost.mPos + mHost.mLen;
        PRUint32 lengthToCut = mPath.mPos - start;
        mSpec.Cut(start, lengthToCut);
        mAuthority.mLen -= lengthToCut;
        ShiftFromPath(-lengthToCut);
    }
    else {
        
        nsCAutoString buf;
        buf.AppendInt(port);
        PRUint32 start = mHost.mPos + mHost.mLen + 1;
        PRUint32 length = mPath.mPos - start;
        mSpec.Replace(start, length, buf);
        if (buf.Length() != length) {
            mAuthority.mLen += buf.Length() - length;
            ShiftFromPath(buf.Length() - length);
        }
    }

    mPort = port;
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetPath(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &path = PromiseFlatCString(input);

    LOG(("nsStandardURL::SetPath [path=%s]\n", path.get()));

    InvalidateCache();

    if (!path.IsEmpty()) {
        nsCAutoString spec;

        spec.Assign(mSpec.get(), mPath.mPos);
        if (path.First() != '/')
            spec.Append('/');
        spec.Append(path);

        return SetSpec(spec);
    }
    else if (mPath.mLen >= 1) {
        mSpec.Cut(mPath.mPos + 1, mPath.mLen - 1);
        
        mPath.mLen = 1;
        mDirectory.mLen = 1;
        mFilepath.mLen = 1;
        
        mBasename.mLen = -1;
        mExtension.mLen = -1;
        mParam.mLen = -1;
        mQuery.mLen = -1;
        mRef.mLen = -1;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::Equals(nsIURI *unknownOther, PRBool *result)
{
    NS_ENSURE_ARG_POINTER(unknownOther);
    NS_PRECONDITION(result, "null pointer");

    nsRefPtr<nsStandardURL> other;
    nsresult rv = unknownOther->QueryInterface(kThisImplCID,
                                               getter_AddRefs(other));
    if (NS_FAILED(rv)) {
        *result = PR_FALSE;
        return NS_OK;
    }

    
    
    if (mSupportsFileURL != other->mSupportsFileURL) {
        *result = PR_FALSE;
        return NS_OK;
    }

    
    
    if (!SegmentIs(mScheme, other->mSpec.get(), other->mScheme) ||
        
        
        !SegmentIs(mHost, other->mSpec.get(), other->mHost) ||
        !SegmentIs(mQuery, other->mSpec.get(), other->mQuery) ||
        !SegmentIs(mRef, other->mSpec.get(), other->mRef) ||
        !SegmentIs(mUsername, other->mSpec.get(), other->mUsername) ||
        !SegmentIs(mPassword, other->mSpec.get(), other->mPassword) ||
        Port() != other->Port() ||
        !SegmentIs(mParam, other->mSpec.get(), other->mParam)) {
        
        
        *result = PR_FALSE;
        return NS_OK;
    }
    
    
    if (SegmentIs(mDirectory, other->mSpec.get(), other->mDirectory) &&
        SegmentIs(mBasename, other->mSpec.get(), other->mBasename) &&
        SegmentIs(mExtension, other->mSpec.get(), other->mExtension)) {
        *result = PR_TRUE;
        return NS_OK;
    }

    
    
    
    
    if (mSupportsFileURL) {
        
        
        *result = PR_FALSE;

        rv = EnsureFile();
        nsresult rv2 = other->EnsureFile();
        
        if (rv == NS_ERROR_NO_INTERFACE && rv == rv2) 
            return NS_OK;
        
        if (NS_FAILED(rv)) {
            LOG(("nsStandardURL::Equals [this=%p spec=%s] failed to ensure file",
                this, mSpec.get()));
            return rv;
        }
        NS_ASSERTION(mFile, "EnsureFile() lied!");
        rv = rv2;
        if (NS_FAILED(rv)) {
            LOG(("nsStandardURL::Equals [other=%p spec=%s] other failed to ensure file",
                 other.get(), other->mSpec.get()));
            return rv;
        }
        NS_ASSERTION(other->mFile, "EnsureFile() lied!");
        return mFile->Equals(other->mFile, result);
    }

    
    
    *result = PR_FALSE;

    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SchemeIs(const char *scheme, PRBool *result)
{
    NS_PRECONDITION(result, "null pointer");

    *result = SegmentIs(mScheme, scheme);
    return NS_OK;
}

 nsStandardURL*
nsStandardURL::StartClone()
{
    nsStandardURL *clone;
    NS_NEWXPCOM(clone, nsStandardURL);
    return clone;
}

NS_IMETHODIMP
nsStandardURL::Clone(nsIURI **result)
{
    nsStandardURL *clone = StartClone();
    if (!clone)
        return NS_ERROR_OUT_OF_MEMORY;

    clone->mSpec = mSpec;
    clone->mDefaultPort = mDefaultPort;
    clone->mPort = mPort;
    clone->mScheme = mScheme;
    clone->mAuthority = mAuthority;
    clone->mUsername = mUsername;
    clone->mPassword = mPassword;
    clone->mHost = mHost;
    clone->mPath = mPath;
    clone->mFilepath = mFilepath;
    clone->mDirectory = mDirectory;
    clone->mBasename = mBasename;
    clone->mExtension = mExtension;
    clone->mParam = mParam;
    clone->mQuery = mQuery;
    clone->mRef = mRef;
    clone->mOriginCharset = mOriginCharset;
    clone->mURLType = mURLType;
    clone->mParser = mParser;
    clone->mFile = mFile;
    clone->mHostA = mHostA ? nsCRT::strdup(mHostA) : nsnull;
    clone->mMutable = PR_TRUE;
    clone->mSupportsFileURL = mSupportsFileURL;
    clone->mHostEncoding = mHostEncoding;
    clone->mSpecEncoding = mSpecEncoding;

    NS_ADDREF(*result = clone);
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::Resolve(const nsACString &in, nsACString &out)
{
    const nsPromiseFlatCString &flat = PromiseFlatCString(in);
    const char *relpath = flat.get();

    
    nsCAutoString buf;
    PRInt32 relpathLen;
    if (net_FilterURIString(relpath, buf)) {
        relpath = buf.get();
        relpathLen = buf.Length();
    } else
        relpathLen = flat.Length();
    
    char *result = nsnull;

    LOG(("nsStandardURL::Resolve [this=%p spec=%s relpath=%s]\n",
        this, mSpec.get(), relpath));

    NS_ASSERTION(mParser, "no parser: unitialized");

    
    
    

    if (mScheme.mLen < 0) {
        NS_ERROR("unable to Resolve URL: this URL not initialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    nsresult rv;
    URLSegment scheme;
    char *resultPath = nsnull;
    PRBool relative = PR_FALSE;
    PRUint32 offset = 0;
    netCoalesceFlags coalesceFlag = NET_COALESCE_NORMAL;

    
    
    
    rv = mParser->ParseURL(relpath, 
                           relpathLen,
                           &scheme.mPos, &scheme.mLen,
                           nsnull, nsnull,
                           nsnull, nsnull);

    
    
    if (NS_FAILED(rv)) scheme.Reset(); 

    if (scheme.mLen >= 0) {
        
        
        if (SegmentIs(relpath, scheme, "ftp", PR_TRUE)) {
            coalesceFlag = (netCoalesceFlags) (coalesceFlag 
                                        | NET_COALESCE_ALLOW_RELATIVE_ROOT
                                        | NET_COALESCE_DOUBLE_SLASH_IS_ROOT);

        }
        
        
        if (SegmentIs(mScheme, relpath, scheme, PR_TRUE)) {
            
            
            if (nsCRT::strncmp(relpath + scheme.mPos + scheme.mLen,
                               "://",3) == 0) {
                
                
                result = NS_strdup(relpath);
            } else {         
                
                
                
                relative = PR_TRUE;
                offset = scheme.mLen + 1;
            }
        } else {
            
            
            result = NS_strdup(relpath);
        }  
    } else {
        
        
        if (SegmentIs(mScheme,"ftp")) {
            coalesceFlag = (netCoalesceFlags) (coalesceFlag 
                                        | NET_COALESCE_ALLOW_RELATIVE_ROOT
                                        | NET_COALESCE_DOUBLE_SLASH_IS_ROOT);
        }
        if (relpath[0] == '/' && relpath[1] == '/') {
            
            result = AppendToSubstring(mScheme.mPos, mScheme.mLen + 1, relpath);
        } else {
            
            relative = PR_TRUE;
        }
    }
    if (relative) {
        PRUint32 len = 0;
        const char *realrelpath = relpath + offset;
        switch (*realrelpath) {
        case '/':
            
            len = mAuthority.mPos + mAuthority.mLen;
            break;
        case '?':
            
            if (mQuery.mLen >= 0)
                len = mQuery.mPos - 1;
            else if (mRef.mLen >= 0)
                len = mRef.mPos - 1;
            else
                len = mPath.mPos + mPath.mLen;
            break;
        case '#':
        case '\0':
            
            if (mRef.mLen < 0)
                len = mPath.mPos + mPath.mLen;
            else
                len = mRef.mPos - 1;
            break;
        default:
            if (coalesceFlag & NET_COALESCE_DOUBLE_SLASH_IS_ROOT) {
                if (Filename().Equals(NS_LITERAL_CSTRING("%2F"),
                                      nsCaseInsensitiveCStringComparator())) {
                    
                    
                    
                    len = mFilepath.mPos + mFilepath.mLen;
                } else {
                    
                    len = mDirectory.mPos + mDirectory.mLen;
                }
            } else { 
                
                len = mDirectory.mPos + mDirectory.mLen;
            }
        }
        result = AppendToSubstring(0, len, realrelpath);
        
        resultPath = result + mPath.mPos;
    }
    if (!result)
        return NS_ERROR_OUT_OF_MEMORY;

    if (resultPath)
        net_CoalesceDirs(coalesceFlag, resultPath);
    else {
        
        resultPath = PL_strstr(result, "://");
        if (resultPath) {
            resultPath = PL_strchr(resultPath + 3, '/');
            if (resultPath)
                net_CoalesceDirs(coalesceFlag,resultPath);
        }
    }
    out.Adopt(result);
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetCommonBaseSpec(nsIURI *uri2, nsACString &aResult)
{
    NS_ENSURE_ARG_POINTER(uri2);

    
    PRBool isEquals = PR_FALSE;
    if (NS_SUCCEEDED(Equals(uri2, &isEquals)) && isEquals)
        return GetSpec(aResult);

    aResult.Truncate();

    
    nsStandardURL *stdurl2;
    nsresult rv = uri2->QueryInterface(kThisImplCID, (void **) &stdurl2);
    isEquals = NS_SUCCEEDED(rv)
            && SegmentIs(mScheme, stdurl2->mSpec.get(), stdurl2->mScheme)    
            && SegmentIs(mHost, stdurl2->mSpec.get(), stdurl2->mHost)
            && SegmentIs(mUsername, stdurl2->mSpec.get(), stdurl2->mUsername)
            && SegmentIs(mPassword, stdurl2->mSpec.get(), stdurl2->mPassword)
            && (Port() == stdurl2->Port());
    if (!isEquals)
    {
        if (NS_SUCCEEDED(rv))
            NS_RELEASE(stdurl2);
        return NS_OK;
    }

    
    const char *thisIndex, *thatIndex, *startCharPos;
    startCharPos = mSpec.get() + mDirectory.mPos;
    thisIndex = startCharPos;
    thatIndex = stdurl2->mSpec.get() + mDirectory.mPos;
    while ((*thisIndex == *thatIndex) && *thisIndex)
    {
        thisIndex++;
        thatIndex++;
    }

    
    
    
    while ((*(thisIndex-1) != '/') && (thisIndex != startCharPos))
        thisIndex--;

    
    aResult = Substring(mSpec, mScheme.mPos, thisIndex - mSpec.get());

    NS_RELEASE(stdurl2);
    return rv;
}

NS_IMETHODIMP
nsStandardURL::GetRelativeSpec(nsIURI *uri2, nsACString &aResult)
{
    NS_ENSURE_ARG_POINTER(uri2);

    aResult.Truncate();

    
    PRBool isEquals = PR_FALSE;
    if (NS_SUCCEEDED(Equals(uri2, &isEquals)) && isEquals)
        return NS_OK;

    nsStandardURL *stdurl2;
    nsresult rv = uri2->QueryInterface(kThisImplCID, (void **) &stdurl2);
    isEquals = NS_SUCCEEDED(rv)
            && SegmentIs(mScheme, stdurl2->mSpec.get(), stdurl2->mScheme)    
            && SegmentIs(mHost, stdurl2->mSpec.get(), stdurl2->mHost)
            && SegmentIs(mUsername, stdurl2->mSpec.get(), stdurl2->mUsername)
            && SegmentIs(mPassword, stdurl2->mSpec.get(), stdurl2->mPassword)
            && (Port() == stdurl2->Port());
    if (!isEquals)
    {
        if (NS_SUCCEEDED(rv))
            NS_RELEASE(stdurl2);

        return uri2->GetSpec(aResult);
    }

    
    const char *thisIndex, *thatIndex, *startCharPos;
    startCharPos = mSpec.get() + mDirectory.mPos;
    thisIndex = startCharPos;
    thatIndex = stdurl2->mSpec.get() + mDirectory.mPos;

#ifdef XP_WIN
    PRBool isFileScheme = SegmentIs(mScheme, "file");
    if (isFileScheme)
    {
        
        
        
        while ((*thisIndex == *thatIndex) && (*thisIndex == '/'))
        {
            thisIndex++;
            thatIndex++;
        }
        
        while ((*thisIndex == *thatIndex) && *thisIndex && (*thisIndex != '/'))
        {
            thisIndex++;
            thatIndex++;
        }

        
        if ((*thisIndex != '/') || (*thatIndex != '/'))
        {
            NS_RELEASE(stdurl2);
            return uri2->GetSpec(aResult);
        }
    }
#endif

    while ((*thisIndex == *thatIndex) && *thisIndex)
    {
        thisIndex++;
        thatIndex++;
    }

    
    
    
    while ((*(thatIndex-1) != '/') && (thatIndex != startCharPos))
        thatIndex--;

    
    while (*thisIndex)
    {
        if (*thisIndex == '/')
            aResult.AppendLiteral("../");

        thisIndex++;
    }

    
    PRUint32 startPos = stdurl2->mScheme.mPos + thatIndex - stdurl2->mSpec.get();
    aResult.Append(Substring(stdurl2->mSpec, startPos, 
                             stdurl2->mSpec.Length() - startPos));

    NS_RELEASE(stdurl2);
    return rv;
}






NS_IMETHODIMP
nsStandardURL::GetFilePath(nsACString &result)
{
    result = Filepath();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetParam(nsACString &result)
{
    result = Param();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetQuery(nsACString &result)
{
    result = Query();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetRef(nsACString &result)
{
    result = Ref();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetDirectory(nsACString &result)
{
    result = Directory();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetFileName(nsACString &result)
{
    result = Filename();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetFileBaseName(nsACString &result)
{
    result = Basename();
    return NS_OK;
}


NS_IMETHODIMP
nsStandardURL::GetFileExtension(nsACString &result)
{
    result = Extension();
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetFilePath(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *filepath = flat.get();

    LOG(("nsStandardURL::SetFilePath [filepath=%s]\n", filepath));

    
    
    if (mFilepath.mLen < 0)
        return SetPath(flat);

    if (filepath && *filepath) {
        nsCAutoString spec;
        PRUint32 dirPos, basePos, extPos;
        PRInt32 dirLen, baseLen, extLen;
        nsresult rv;

        rv = mParser->ParseFilePath(filepath, -1,
                                    &dirPos, &dirLen,
                                    &basePos, &baseLen,
                                    &extPos, &extLen);
        if (NS_FAILED(rv)) return rv;

        
        spec.Assign(mSpec.get(), mPath.mPos);

        
        if (filepath[dirPos] != '/')
            spec.Append('/');

        GET_SEGMENT_ENCODER(encoder);

        
        if (dirLen > 0)
            encoder.EncodeSegment(Substring(filepath + dirPos,
                                            filepath + dirPos + dirLen),
                                  esc_Directory | esc_AlwaysCopy, spec);
        if (baseLen > 0)
            encoder.EncodeSegment(Substring(filepath + basePos,
                                            filepath + basePos + baseLen),
                                  esc_FileBaseName | esc_AlwaysCopy, spec);
        if (extLen >= 0) {
            spec.Append('.');
            if (extLen > 0)
                encoder.EncodeSegment(Substring(filepath + extPos,
                                                filepath + extPos + extLen),
                                      esc_FileExtension | esc_AlwaysCopy,
                                      spec);
        }

        
        if (mFilepath.mLen >= 0) {
            PRUint32 end = mFilepath.mPos + mFilepath.mLen;
            if (mSpec.Length() > end)
                spec.Append(mSpec.get() + end, mSpec.Length() - end);
        }

        return SetSpec(spec);
    }
    else if (mPath.mLen > 1) {
        mSpec.Cut(mPath.mPos + 1, mFilepath.mLen - 1);
        
        ShiftFromParam(1 - mFilepath.mLen);
        
        mPath.mLen = 1;
        mDirectory.mLen = 1;
        mFilepath.mLen = 1;
        
        mBasename.mLen = -1;
        mExtension.mLen = -1;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetParam(const nsACString &input)
{
    NS_NOTYETIMPLEMENTED("");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsStandardURL::SetQuery(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *query = flat.get();

    LOG(("nsStandardURL::SetQuery [query=%s]\n", query));

    if (mPath.mLen < 0)
        return SetPath(flat);

    InvalidateCache();

    if (!query || !*query) {
        
        if (mQuery.mLen >= 0) {
            
            mSpec.Cut(mQuery.mPos - 1, mQuery.mLen + 1);
            ShiftFromRef(-(mQuery.mLen + 1));
            mPath.mLen -= (mQuery.mLen + 1);
            mQuery.mPos = 0;
            mQuery.mLen = -1;
        }
        return NS_OK;
    }

    PRInt32 queryLen = strlen(query);
    if (query[0] == '?') {
        query++;
        queryLen--;
    }

    if (mQuery.mLen < 0) {
        if (mRef.mLen < 0)
            mQuery.mPos = mSpec.Length();
        else
            mQuery.mPos = mRef.mPos - 1;
        mSpec.Insert('?', mQuery.mPos);
        mQuery.mPos++;
        mQuery.mLen = 0;
        
        mPath.mLen++;
        mRef.mPos++;
    }

    
    nsCAutoString buf;
    PRBool encoded;
    GET_QUERY_ENCODER(encoder);
    encoder.EncodeSegmentCount(query, URLSegment(0, queryLen), esc_Query,
                               buf, encoded);
    if (encoded) {
        query = buf.get();
        queryLen = buf.Length();
    }

    PRInt32 shift = ReplaceSegment(mQuery.mPos, mQuery.mLen, query, queryLen);

    if (shift) {
        mQuery.mLen = queryLen;
        mPath.mLen += shift;
        ShiftFromRef(shift);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetRef(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *ref = flat.get();

    LOG(("nsStandardURL::SetRef [ref=%s]\n", ref));

    if (mPath.mLen < 0)
        return SetPath(flat);

    InvalidateCache();

    if (!ref || !*ref) {
        
        if (mRef.mLen >= 0) {
            
            mSpec.Cut(mRef.mPos - 1, mRef.mLen + 1);
            mPath.mLen -= (mRef.mLen + 1);
            mRef.mPos = 0;
            mRef.mLen = -1;
        }
        return NS_OK;
    }
            
    PRInt32 refLen = strlen(ref);
    if (ref[0] == '#') {
        ref++;
        refLen--;
    }
    
    if (mRef.mLen < 0) {
        mSpec.Append('#');
        ++mPath.mLen;  
        mRef.mPos = mSpec.Length();
        mRef.mLen = 0;
    }

    
    nsCAutoString buf;
    PRBool encoded;
    GET_SEGMENT_ENCODER(encoder);
    encoder.EncodeSegmentCount(ref, URLSegment(0, refLen), esc_Ref,
                               buf, encoded);
    if (encoded) {
        ref = buf.get();
        refLen = buf.Length();
    }

    PRInt32 shift = ReplaceSegment(mRef.mPos, mRef.mLen, ref, refLen);
    mPath.mLen += shift;
    mRef.mLen = refLen;
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetDirectory(const nsACString &input)
{
    NS_NOTYETIMPLEMENTED("");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsStandardURL::SetFileName(const nsACString &input)
{
    ENSURE_MUTABLE();

    const nsPromiseFlatCString &flat = PromiseFlatCString(input);
    const char *filename = flat.get();

    LOG(("nsStandardURL::SetFileName [filename=%s]\n", filename));

    if (mPath.mLen < 0)
        return SetPath(flat);

    PRInt32 shift = 0;

    if (!(filename && *filename)) {
        
        if (mBasename.mLen > 0) {
            if (mExtension.mLen >= 0)
                mBasename.mLen += (mExtension.mLen + 1);
            mSpec.Cut(mBasename.mPos, mBasename.mLen);
            shift = -mBasename.mLen;
            mBasename.mLen = 0;
            mExtension.mLen = -1;
        }
    }
    else {
        nsresult rv;
        URLSegment basename, extension;

        
        rv = mParser->ParseFileName(filename, -1,
                                    &basename.mPos, &basename.mLen,
                                    &extension.mPos, &extension.mLen);
        if (NS_FAILED(rv)) return rv;

        if (basename.mLen < 0) {
            
            if (mBasename.mLen >= 0) {
                PRUint32 len = mBasename.mLen;
                if (mExtension.mLen >= 0)
                    len += (mExtension.mLen + 1);
                mSpec.Cut(mBasename.mPos, len);
                shift = -PRInt32(len);
                mBasename.mLen = 0;
                mExtension.mLen = -1;
            }
        }
        else {
            nsCAutoString newFilename;
            PRBool ignoredOut;
            GET_SEGMENT_ENCODER(encoder);
            basename.mLen = encoder.EncodeSegmentCount(filename, basename,
                                                       esc_FileBaseName |
                                                       esc_AlwaysCopy,
                                                       newFilename,
                                                       ignoredOut);
            if (extension.mLen >= 0) {
                newFilename.Append('.');
                extension.mLen = encoder.EncodeSegmentCount(filename, extension,
                                                            esc_FileExtension |
                                                            esc_AlwaysCopy,
                                                            newFilename,
                                                            ignoredOut);
            }

            if (mBasename.mLen < 0) {
                
                mBasename.mPos = mDirectory.mPos + mDirectory.mLen;
                mSpec.Insert(newFilename, mBasename.mPos);
                shift = newFilename.Length();
            }
            else {
                
                PRUint32 oldLen = PRUint32(mBasename.mLen);
                if (mExtension.mLen >= 0)
                    oldLen += (mExtension.mLen + 1);
                mSpec.Replace(mBasename.mPos, oldLen, newFilename);
                shift = newFilename.Length() - oldLen;
            }
            
            mBasename.mLen = basename.mLen;
            mExtension.mLen = extension.mLen;
            if (mExtension.mLen >= 0)
                mExtension.mPos = mBasename.mPos + mBasename.mLen + 1;
        }
    }
    if (shift) {
        ShiftFromParam(shift);
        mFilepath.mLen += shift;
        mPath.mLen += shift;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetFileBaseName(const nsACString &input)
{
    nsCAutoString extension;
    nsresult rv = GetFileExtension(extension);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString newFileName(input);

    if (!extension.IsEmpty()) {
        newFileName.Append('.');
        newFileName.Append(extension);
    }

    return SetFileName(newFileName);
}

NS_IMETHODIMP
nsStandardURL::SetFileExtension(const nsACString &input)
{
    nsCAutoString newFileName;
    nsresult rv = GetFileBaseName(newFileName);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!input.IsEmpty()) {
        newFileName.Append('.');
        newFileName.Append(input);
    }

    return SetFileName(newFileName);
}





nsresult
nsStandardURL::EnsureFile()
{
    NS_PRECONDITION(mSupportsFileURL,
                    "EnsureFile() called on a URL that doesn't support files!");
    if (mFile) {
        
        return NS_OK;
    }

    
    if (mSpec.IsEmpty()) {
        NS_ERROR("url not initialized");
        return NS_ERROR_NOT_INITIALIZED;
    }

    if (!SegmentIs(mScheme, "file")) {
        NS_ERROR("not a file URL");
        return NS_ERROR_FAILURE;
    }

    return net_GetFileFromURLSpec(mSpec, getter_AddRefs(mFile));
}

NS_IMETHODIMP
nsStandardURL::GetFile(nsIFile **result)
{
    NS_PRECONDITION(mSupportsFileURL,
                    "GetFile() called on a URL that doesn't support files!");
    nsresult rv = EnsureFile();
    if (NS_FAILED(rv))
        return rv;

#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        nsCAutoString path;
        mFile->GetNativePath(path);
        LOG(("nsStandardURL::GetFile [this=%p spec=%s resulting_path=%s]\n",
            this, mSpec.get(), path.get()));
    }
#endif

    
    
    
    
    
    
    return mFile->Clone(result);
}

NS_IMETHODIMP
nsStandardURL::SetFile(nsIFile *file)
{
    ENSURE_MUTABLE();

    NS_ENSURE_ARG_POINTER(file);

    nsresult rv;
    nsCAutoString url;

    rv = net_GetURLSpecFromFile(file, url);
    if (NS_FAILED(rv)) return rv;

    SetSpec(url);

    rv = Init(mURLType, mDefaultPort, url, nsnull, nsnull);

    
    if (NS_SUCCEEDED(rv)) {
        InvalidateCache();
        if (NS_FAILED(file->Clone(getter_AddRefs(mFile)))) {
            NS_WARNING("nsIFile::Clone failed");
            
            mFile = 0;
        }
    }
    return rv;
}





inline PRBool
IsUTFCharset(const char *aCharset)
{
    return ((aCharset[0] == 'U' || aCharset[0] == 'u') &&
            (aCharset[1] == 'T' || aCharset[1] == 't') &&
            (aCharset[2] == 'F' || aCharset[2] == 'f'));
}

NS_IMETHODIMP
nsStandardURL::Init(PRUint32 urlType,
                    PRInt32 defaultPort,
                    const nsACString &spec,
                    const char *charset,
                    nsIURI *baseURI)
{
    ENSURE_MUTABLE();

    InvalidateCache();

    switch (urlType) {
    case URLTYPE_STANDARD:
        mParser = net_GetStdURLParser();
        break;
    case URLTYPE_AUTHORITY:
        mParser = net_GetAuthURLParser();
        break;
    case URLTYPE_NO_AUTHORITY:
        mParser = net_GetNoAuthURLParser();
        break;
    default:
        NS_NOTREACHED("bad urlType");
        return NS_ERROR_INVALID_ARG;
    }
    mDefaultPort = defaultPort;
    mURLType = urlType;

    mOriginCharset.Truncate();

    if (charset == nsnull || *charset == '\0') {
        
        if (baseURI)
            baseURI->GetOriginCharset(mOriginCharset);

        
        
        
        

        if (mOriginCharset.Length() > 3 &&
            IsUTFCharset(mOriginCharset.get())) {
            mOriginCharset.Truncate();
        }
    }
    else if (!IsUTFCharset(charset)) {
        mOriginCharset = charset;
    }

    if (baseURI) {
        PRUint32 start, end;
        
        nsresult rv = net_ExtractURLScheme(spec, &start, &end, nsnull);
        if (NS_SUCCEEDED(rv) && spec.Length() > end+2) {
            nsACString::const_iterator slash;
            spec.BeginReading(slash);
            slash.advance(end+1);
            
            
            
            if (*slash == '/' && *(++slash) == '/')
                baseURI = nsnull;
        }
    }

    if (!baseURI)
        return SetSpec(spec);

    nsCAutoString buf;
    nsresult rv = baseURI->Resolve(spec, buf);
    if (NS_FAILED(rv)) return rv;

    return SetSpec(buf);
}

NS_IMETHODIMP
nsStandardURL::GetMutable(PRBool *value)
{
    *value = mMutable;
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::SetMutable(PRBool value)
{
    NS_ENSURE_ARG(mMutable || !value);

    mMutable = value;
    return NS_OK;
}





NS_IMETHODIMP
nsStandardURL::Read(nsIObjectInputStream *stream)
{
    NS_PRECONDITION(!mHostA, "Shouldn't have cached ASCII host");
    NS_PRECONDITION(mSpecEncoding == eEncoding_Unknown,
                    "Shouldn't have spec encoding here");
    
    nsresult rv;
    
    PRUint32 urlType;
    rv = stream->Read32(&urlType);
    if (NS_FAILED(rv)) return rv;
    mURLType = urlType;
    switch (mURLType) {
      case URLTYPE_STANDARD:
        mParser = net_GetStdURLParser();
        break;
      case URLTYPE_AUTHORITY:
        mParser = net_GetAuthURLParser();
        break;
      case URLTYPE_NO_AUTHORITY:
        mParser = net_GetNoAuthURLParser();
        break;
      default:
        NS_NOTREACHED("bad urlType");
        return NS_ERROR_FAILURE;
    }

    rv = stream->Read32((PRUint32 *) &mPort);
    if (NS_FAILED(rv)) return rv;

    rv = stream->Read32((PRUint32 *) &mDefaultPort);
    if (NS_FAILED(rv)) return rv;

    rv = NS_ReadOptionalCString(stream, mSpec);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mScheme);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mAuthority);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mUsername);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mPassword);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mHost);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mPath);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mFilepath);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mDirectory);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mBasename);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mExtension);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mParam);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mQuery);
    if (NS_FAILED(rv)) return rv;

    rv = ReadSegment(stream, mRef);
    if (NS_FAILED(rv)) return rv;

    rv = NS_ReadOptionalCString(stream, mOriginCharset);
    if (NS_FAILED(rv)) return rv;

    PRBool isMutable;
    rv = stream->ReadBoolean(&isMutable);
    if (NS_FAILED(rv)) return rv;
    if (isMutable != PR_TRUE && isMutable != PR_FALSE) {
        NS_WARNING("Unexpected boolean value");
        return NS_ERROR_UNEXPECTED;
    }
    mMutable = isMutable;

    PRBool supportsFileURL;
    rv = stream->ReadBoolean(&supportsFileURL);
    if (NS_FAILED(rv)) return rv;
    if (supportsFileURL != PR_TRUE && supportsFileURL != PR_FALSE) {
        NS_WARNING("Unexpected boolean value");
        return NS_ERROR_UNEXPECTED;
    }
    mSupportsFileURL = supportsFileURL;

    PRUint32 hostEncoding;
    rv = stream->Read32(&hostEncoding);
    if (NS_FAILED(rv)) return rv;
    if (hostEncoding != eEncoding_ASCII && hostEncoding != eEncoding_UTF8) {
        NS_WARNING("Unexpected host encoding");
        return NS_ERROR_UNEXPECTED;
    }
    mHostEncoding = hostEncoding;
    
    return NS_OK;
}

NS_IMETHODIMP
nsStandardURL::Write(nsIObjectOutputStream *stream)
{
    nsresult rv;

    rv = stream->Write32(mURLType);
    if (NS_FAILED(rv)) return rv;

    rv = stream->Write32(PRUint32(mPort));
    if (NS_FAILED(rv)) return rv;

    rv = stream->Write32(PRUint32(mDefaultPort));
    if (NS_FAILED(rv)) return rv;

    rv = NS_WriteOptionalStringZ(stream, mSpec.get());
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mScheme);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mAuthority);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mUsername);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mPassword);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mHost);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mPath);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mFilepath);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mDirectory);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mBasename);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mExtension);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mParam);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mQuery);
    if (NS_FAILED(rv)) return rv;

    rv = WriteSegment(stream, mRef);
    if (NS_FAILED(rv)) return rv;

    rv = NS_WriteOptionalStringZ(stream, mOriginCharset.get());
    if (NS_FAILED(rv)) return rv;

    rv = stream->WriteBoolean(mMutable);
    if (NS_FAILED(rv)) return rv;

    rv = stream->WriteBoolean(mSupportsFileURL);
    if (NS_FAILED(rv)) return rv;

    rv = stream->Write32(mHostEncoding);
    if (NS_FAILED(rv)) return rv;

    

    return NS_OK;
}





NS_IMETHODIMP 
nsStandardURL::GetInterfaces(PRUint32 *count, nsIID * **array)
{
    *count = 0;
    *array = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsStandardURL::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
    *_retval = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsStandardURL::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsStandardURL::GetClassDescription(char * *aClassDescription)
{
    *aClassDescription = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsStandardURL::GetClassID(nsCID * *aClassID)
{
    *aClassID = (nsCID*) nsMemory::Alloc(sizeof(nsCID));
    if (!*aClassID)
        return NS_ERROR_OUT_OF_MEMORY;
    return GetClassIDNoAlloc(*aClassID);
}

NS_IMETHODIMP 
nsStandardURL::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}

NS_IMETHODIMP 
nsStandardURL::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::MAIN_THREAD_ONLY;
    return NS_OK;
}

NS_IMETHODIMP 
nsStandardURL::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    *aClassIDNoAlloc = kStandardURLCID;
    return NS_OK;
}
