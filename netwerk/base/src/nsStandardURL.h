





































#ifndef nsStandardURL_h__
#define nsStandardURL_h__

#include "nsString.h"
#include "nsDependentString.h"
#include "nsDependentSubstring.h"
#include "nsISerializable.h"
#include "nsIIPCSerializable.h"
#include "nsIFileURL.h"
#include "nsIStandardURL.h"
#include "nsIFile.h"
#include "nsIURLParser.h"
#include "nsIUnicodeEncoder.h"
#include "nsIObserver.h"
#include "nsIIOService.h"
#include "nsCOMPtr.h"
#include "nsURLHelper.h"
#include "nsIClassInfo.h"
#include "prclist.h"

#ifdef NS_BUILD_REFCNT_LOGGING
#define DEBUG_DUMP_URLS_AT_SHUTDOWN
#endif

class nsIBinaryInputStream;
class nsIBinaryOutputStream;
class nsIIDNService;
class nsICharsetConverterManager;
class nsIPrefBranch;





class nsStandardURL : public nsIFileURL
                    , public nsIStandardURL
                    , public nsISerializable
                    , public nsIIPCSerializable
                    , public nsIClassInfo
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIURI
    NS_DECL_NSIURL
    NS_DECL_NSIFILEURL
    NS_DECL_NSISTANDARDURL
    NS_DECL_NSISERIALIZABLE
    NS_DECL_NSIIPCSERIALIZABLE
    NS_DECL_NSICLASSINFO
    NS_DECL_NSIMUTABLE

    nsStandardURL(bool aSupportsFileURL = false);
    virtual ~nsStandardURL();

    static void InitGlobalObjects();
    static void ShutdownGlobalObjects();

public: 
    
    
    
    struct URLSegment
    {
        PRUint32 mPos;
        PRInt32  mLen;

        URLSegment() : mPos(0), mLen(-1) {}
        URLSegment(PRUint32 pos, PRInt32 len) : mPos(pos), mLen(len) {}
        void Reset() { mPos = 0; mLen = -1; }
        
        
        
        void Merge(const nsCString &spec, const char separator, const URLSegment &right) {
            if (mLen >= 0 && 
                *(spec.get() + mPos + mLen) == separator &&
                mPos + mLen + 1 == right.mPos) {
                mLen += 1 + right.mLen;
            }
        }
            
    };

    
    
    
    class nsPrefObserver : public nsIObserver
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIOBSERVER

        nsPrefObserver() { }
    };
    friend class nsPrefObserver;

    
    
    
    class nsSegmentEncoder
    {
    public:
        nsSegmentEncoder(const char *charset);

        
        
        
        PRInt32 EncodeSegmentCount(const char *str,
                                   const URLSegment &segment,
                                   PRInt16 mask,
                                   nsAFlatCString &buf,
                                   bool& appended,
                                   PRUint32 extraLen = 0);
         
        
        
        
        const nsACString &EncodeSegment(const nsASingleFragmentCString &str,
                                        PRInt16 mask,
                                        nsAFlatCString &buf);
    private:
        bool InitUnicodeEncoder();
        
        const char* mCharset;  
                               
        nsCOMPtr<nsIUnicodeEncoder> mEncoder;
    };
    friend class nsSegmentEncoder;

protected:
    
    enum RefHandlingEnum {
        eIgnoreRef,
        eHonorRef
    };

    
    
    nsresult EqualsInternal(nsIURI* unknownOther,
                            RefHandlingEnum refHandlingMode,
                            bool* result);

    virtual nsStandardURL* StartClone();

    
    nsresult CloneInternal(RefHandlingEnum aRefHandlingMode,
                           nsIURI** aClone);

    
    
    
    
    virtual nsresult EnsureFile();

private:
    PRInt32  Port() { return mPort == -1 ? mDefaultPort : mPort; }

    void     Clear();
    void     InvalidateCache(bool invalidateCachedFile = true);

    bool     EscapeIPv6(const char *host, nsCString &result);
    bool     NormalizeIDN(const nsCSubstring &host, nsCString &result);
    void     CoalescePath(netCoalesceFlags coalesceFlag, char *path);

    PRUint32 AppendSegmentToBuf(char *, PRUint32, const char *, URLSegment &, const nsCString *esc=nsnull, bool useEsc = false);
    PRUint32 AppendToBuf(char *, PRUint32, const char *, PRUint32);

    nsresult BuildNormalizedSpec(const char *spec);

    bool     SegmentIs(const URLSegment &s1, const char *val, bool ignoreCase = false);
    bool     SegmentIs(const char* spec, const URLSegment &s1, const char *val, bool ignoreCase = false);
    bool     SegmentIs(const URLSegment &s1, const char *val, const URLSegment &s2, bool ignoreCase = false);

    PRInt32  ReplaceSegment(PRUint32 pos, PRUint32 len, const char *val, PRUint32 valLen);
    PRInt32  ReplaceSegment(PRUint32 pos, PRUint32 len, const nsACString &val);

    nsresult ParseURL(const char *spec, PRInt32 specLen);
    nsresult ParsePath(const char *spec, PRUint32 pathPos, PRInt32 pathLen = -1);

    char    *AppendToSubstring(PRUint32 pos, PRInt32 len, const char *tail);

    
    const nsDependentCSubstring Segment(PRUint32 pos, PRInt32 len); 
    const nsDependentCSubstring Segment(const URLSegment &s) { return Segment(s.mPos, s.mLen); }

    
    const nsDependentCSubstring Prepath();  
    const nsDependentCSubstring Scheme()    { return Segment(mScheme); }
    const nsDependentCSubstring Userpass(bool includeDelim = false); 
    const nsDependentCSubstring Username()  { return Segment(mUsername); }
    const nsDependentCSubstring Password()  { return Segment(mPassword); }
    const nsDependentCSubstring Hostport(); 
    const nsDependentCSubstring Host();     
    const nsDependentCSubstring Path()      { return Segment(mPath); }
    const nsDependentCSubstring Filepath()  { return Segment(mFilepath); }
    const nsDependentCSubstring Directory() { return Segment(mDirectory); }
    const nsDependentCSubstring Filename(); 
    const nsDependentCSubstring Basename()  { return Segment(mBasename); }
    const nsDependentCSubstring Extension() { return Segment(mExtension); }
    const nsDependentCSubstring Query()     { return Segment(mQuery); }
    const nsDependentCSubstring Ref()       { return Segment(mRef); }

    
    void ShiftFromAuthority(PRInt32 diff) { mAuthority.mPos += diff; ShiftFromUsername(diff); }
    void ShiftFromUsername(PRInt32 diff)  { mUsername.mPos += diff; ShiftFromPassword(diff); }
    void ShiftFromPassword(PRInt32 diff)  { mPassword.mPos += diff; ShiftFromHost(diff); }
    void ShiftFromHost(PRInt32 diff)      { mHost.mPos += diff; ShiftFromPath(diff); }
    void ShiftFromPath(PRInt32 diff)      { mPath.mPos += diff; ShiftFromFilepath(diff); }
    void ShiftFromFilepath(PRInt32 diff)  { mFilepath.mPos += diff; ShiftFromDirectory(diff); }
    void ShiftFromDirectory(PRInt32 diff) { mDirectory.mPos += diff; ShiftFromBasename(diff); }
    void ShiftFromBasename(PRInt32 diff)  { mBasename.mPos += diff; ShiftFromExtension(diff); }
    void ShiftFromExtension(PRInt32 diff) { mExtension.mPos += diff; ShiftFromQuery(diff); }
    void ShiftFromQuery(PRInt32 diff)     { mQuery.mPos += diff; ShiftFromRef(diff); }
    void ShiftFromRef(PRInt32 diff)       { mRef.mPos += diff; }

    
    nsresult ReadSegment(nsIBinaryInputStream *, URLSegment &);
    nsresult WriteSegment(nsIBinaryOutputStream *, const URLSegment &);

    
    bool ReadSegment(const IPC::Message *, void **, URLSegment &);
    void WriteSegment(IPC::Message *, const URLSegment &);

    static void PrefsChanged(nsIPrefBranch *prefs, const char *pref);

    
    nsCString mSpec;
    PRInt32   mDefaultPort;
    PRInt32   mPort;

    
    URLSegment mScheme;
    URLSegment mAuthority;
    URLSegment mUsername;
    URLSegment mPassword;
    URLSegment mHost;
    URLSegment mPath;
    URLSegment mFilepath;
    URLSegment mDirectory;
    URLSegment mBasename;
    URLSegment mExtension;
    URLSegment mQuery;
    URLSegment mRef;

    nsCString              mOriginCharset;
    nsCOMPtr<nsIURLParser> mParser;

    
protected:
    nsCOMPtr<nsIFile>      mFile;  
    
private:
    char                  *mHostA; 

    enum {
        eEncoding_Unknown,
        eEncoding_ASCII,
        eEncoding_UTF8
    };

    PRUint32 mHostEncoding    : 2; 
    PRUint32 mSpecEncoding    : 2; 
    PRUint32 mURLType         : 2; 
    PRUint32 mMutable         : 1; 
    PRUint32 mSupportsFileURL : 1; 

    
    
    static nsIIDNService               *gIDN;
    static nsICharsetConverterManager  *gCharsetMgr;
    static bool                         gInitialized;
    static bool                         gEscapeUTF8;
    static bool                         gAlwaysEncodeInUTF8;
    static bool                         gEncodeQueryInUTF8;

public:
#ifdef DEBUG_DUMP_URLS_AT_SHUTDOWN
    PRCList mDebugCList;
    void PrintSpec() const { printf("  %s\n", mSpec.get()); }
#endif
};

#define NS_THIS_STANDARDURL_IMPL_CID                 \
{ /* b8e3e97b-1ccd-4b45-af5a-79596770f5d7 */         \
    0xb8e3e97b,                                      \
    0x1ccd,                                          \
    0x4b45,                                          \
    {0xaf, 0x5a, 0x79, 0x59, 0x67, 0x70, 0xf5, 0xd7} \
}





inline const nsDependentCSubstring
nsStandardURL::Segment(PRUint32 pos, PRInt32 len)
{
    if (len < 0) {
        pos = 0;
        len = 0;
    }
    return Substring(mSpec, pos, PRUint32(len));
}

inline const nsDependentCSubstring
nsStandardURL::Prepath()
{
    PRUint32 len = 0;
    if (mAuthority.mLen >= 0)
        len = mAuthority.mPos + mAuthority.mLen;
    return Substring(mSpec, 0, len);
}

inline const nsDependentCSubstring
nsStandardURL::Userpass(bool includeDelim)
{
    PRUint32 pos=0, len=0;
    
    if (mUsername.mLen > 0) {
        pos = mUsername.mPos;
        len = mUsername.mLen;
        if (mPassword.mLen >= 0)
            len += (mPassword.mLen + 1);
        if (includeDelim)
            len++;
    }
    return Substring(mSpec, pos, len);
}

inline const nsDependentCSubstring
nsStandardURL::Hostport()
{
    PRUint32 pos=0, len=0;
    if (mAuthority.mLen > 0) {
        pos = mHost.mPos;
        len = mAuthority.mPos + mAuthority.mLen - pos;
    }
    return Substring(mSpec, pos, len);
}

inline const nsDependentCSubstring
nsStandardURL::Host()
{
    PRUint32 pos=0, len=0;
    if (mHost.mLen > 0) {
        pos = mHost.mPos;
        len = mHost.mLen;
        if (mSpec.CharAt(pos) == '[' && mSpec.CharAt(pos + len - 1) == ']') {
            pos++;
            len -= 2;
        }
    }
    return Substring(mSpec, pos, len);
}

inline const nsDependentCSubstring
nsStandardURL::Filename()
{
    PRUint32 pos=0, len=0;
    
    if (mBasename.mLen > 0) {
        pos = mBasename.mPos;
        len = mBasename.mLen;
        if (mExtension.mLen >= 0)
            len += (mExtension.mLen + 1);
    }
    return Substring(mSpec, pos, len);
}

#endif
