





#include "mozilla/RangedPtr.h"

#include "nsURLHelper.h"
#include "nsIFile.h"
#include "nsIURLParser.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsNetCID.h"
#include "prnetdb.h"

using namespace mozilla;





static bool gInitialized = false;
static nsIURLParser *gNoAuthURLParser = nullptr;
static nsIURLParser *gAuthURLParser = nullptr;
static nsIURLParser *gStdURLParser = nullptr;
static int32_t gMaxLength = 1048576; 

static void
InitGlobals()
{
    nsCOMPtr<nsIURLParser> parser;

    parser = do_GetService(NS_NOAUTHURLPARSER_CONTRACTID);
    NS_ASSERTION(parser, "failed getting 'noauth' url parser");
    if (parser) {
        gNoAuthURLParser = parser.get();
        NS_ADDREF(gNoAuthURLParser);
    }

    parser = do_GetService(NS_AUTHURLPARSER_CONTRACTID);
    NS_ASSERTION(parser, "failed getting 'auth' url parser");
    if (parser) {
        gAuthURLParser = parser.get();
        NS_ADDREF(gAuthURLParser);
    }

    parser = do_GetService(NS_STDURLPARSER_CONTRACTID);
    NS_ASSERTION(parser, "failed getting 'std' url parser");
    if (parser) {
        gStdURLParser = parser.get();
        NS_ADDREF(gStdURLParser);
    }

    gInitialized = true;
#if !defined(MOZILLA_XPCOMRT_API)
    Preferences::AddIntVarCache(&gMaxLength,
                                "network.standard-url.max-length", 1048576);
#endif
}

void
net_ShutdownURLHelper()
{
    if (gInitialized) {
        NS_IF_RELEASE(gNoAuthURLParser);
        NS_IF_RELEASE(gAuthURLParser);
        NS_IF_RELEASE(gStdURLParser);
        gInitialized = false;
    }
}

int32_t net_GetURLMaxLength()
{
    return gMaxLength;
}





nsIURLParser *
net_GetAuthURLParser()
{
    if (!gInitialized)
        InitGlobals();
    return gAuthURLParser;
}

nsIURLParser *
net_GetNoAuthURLParser()
{
    if (!gInitialized)
        InitGlobals();
    return gNoAuthURLParser;
}

nsIURLParser *
net_GetStdURLParser()
{
    if (!gInitialized)
        InitGlobals();
    return gStdURLParser;
}




nsresult
net_GetURLSpecFromDir(nsIFile *aFile, nsACString &result)
{
#if defined(MOZILLA_XPCOMRT_API)
    NS_WARNING("net_GetURLSpecFromDir not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
#else
    nsAutoCString escPath;
    nsresult rv = net_GetURLSpecFromActualFile(aFile, escPath);
    if (NS_FAILED(rv))
        return rv;

    if (escPath.Last() != '/') {
        escPath += '/';
    }
    
    result = escPath;
    return NS_OK;
#endif 
}

nsresult
net_GetURLSpecFromFile(nsIFile *aFile, nsACString &result)
{
#if defined(MOZILLA_XPCOMRT_API)
    NS_WARNING("net_GetURLSpecFromFile not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
#else
    nsAutoCString escPath;
    nsresult rv = net_GetURLSpecFromActualFile(aFile, escPath);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    
    if (escPath.Last() != '/') {
        bool dir;
        rv = aFile->IsDirectory(&dir);
        if (NS_SUCCEEDED(rv) && dir)
            escPath += '/';
    }
    
    result = escPath;
    return NS_OK;
#endif 
}





nsresult
net_ParseFileURL(const nsACString &inURL,
                 nsACString &outDirectory,
                 nsACString &outFileBaseName,
                 nsACString &outFileExtension)
{
    nsresult rv;

    if (inURL.Length() > (uint32_t) gMaxLength) {
        return NS_ERROR_MALFORMED_URI;
    }

    outDirectory.Truncate();
    outFileBaseName.Truncate();
    outFileExtension.Truncate();

    const nsPromiseFlatCString &flatURL = PromiseFlatCString(inURL);
    const char *url = flatURL.get();
    
    uint32_t schemeBeg, schemeEnd;
    rv = net_ExtractURLScheme(flatURL, &schemeBeg, &schemeEnd, nullptr);
    if (NS_FAILED(rv)) return rv;

    if (strncmp(url + schemeBeg, "file", schemeEnd - schemeBeg) != 0) {
        NS_ERROR("must be a file:// url");
        return NS_ERROR_UNEXPECTED;
    }

    nsIURLParser *parser = net_GetNoAuthURLParser();
    NS_ENSURE_TRUE(parser, NS_ERROR_UNEXPECTED);

    uint32_t pathPos, filepathPos, directoryPos, basenamePos, extensionPos;
    int32_t pathLen, filepathLen, directoryLen, basenameLen, extensionLen;

    
    rv = parser->ParseURL(url, flatURL.Length(),
                          nullptr, nullptr, 
                          nullptr, nullptr, 
                          &pathPos, &pathLen);
    if (NS_FAILED(rv)) return rv;

    
    rv = parser->ParsePath(url + pathPos, pathLen,
                           &filepathPos, &filepathLen,
                           nullptr, nullptr,  
                           nullptr, nullptr); 
    if (NS_FAILED(rv)) return rv;

    filepathPos += pathPos;

    
    rv = parser->ParseFilePath(url + filepathPos, filepathLen,
                               &directoryPos, &directoryLen,
                               &basenamePos, &basenameLen,
                               &extensionPos, &extensionLen);
    if (NS_FAILED(rv)) return rv;

    if (directoryLen > 0)
        outDirectory = Substring(inURL, filepathPos + directoryPos, directoryLen);
    if (basenameLen > 0)
        outFileBaseName = Substring(inURL, filepathPos + basenamePos, basenameLen);
    if (extensionLen > 0)
        outFileExtension = Substring(inURL, filepathPos + extensionPos, extensionLen);
    
    

    return NS_OK;
}







void 
net_CoalesceDirs(netCoalesceFlags flags, char* path)
{
    





    char *fwdPtr = path;
    char *urlPtr = path;
    char *lastslash = path;
    uint32_t traversal = 0;
    uint32_t special_ftp_len = 0;

    
    if (flags & NET_COALESCE_DOUBLE_SLASH_IS_ROOT) 
    {
       



        if (nsCRT::strncasecmp(path,"/%2F",4) == 0)
            special_ftp_len = 4;
        else if (nsCRT::strncmp(path,"//",2) == 0 )
            special_ftp_len = 2; 
    }

    
    for(; (*fwdPtr != '\0') && 
            (*fwdPtr != '?') && 
            (*fwdPtr != '#'); ++fwdPtr)
    {
    }

    
    
    if (fwdPtr != path && *fwdPtr == '\0')
    {
        --fwdPtr;
    }

    
    for(; (fwdPtr != path) && 
            (*fwdPtr != '/'); --fwdPtr)
    {
    }
    lastslash = fwdPtr;
    fwdPtr = path;

    
    
    for(; (*fwdPtr != '\0') && 
            (*fwdPtr != '?') && 
            (*fwdPtr != '#') &&
            (*lastslash == '\0' || fwdPtr != lastslash); ++fwdPtr)
    {
        if (*fwdPtr == '%' && *(fwdPtr+1) == '2' && 
            (*(fwdPtr+2) == 'E' || *(fwdPtr+2) == 'e'))
        {
            *urlPtr++ = '.';
            ++fwdPtr;
            ++fwdPtr;
        } 
        else 
        {
            *urlPtr++ = *fwdPtr;
        }
    }
    
    for (; *fwdPtr != '\0'; ++fwdPtr)
    {
        *urlPtr++ = *fwdPtr;
    }
    *urlPtr = '\0';  

    
    fwdPtr = path;
    urlPtr = path;

    for(; (*fwdPtr != '\0') && 
            (*fwdPtr != '?') && 
            (*fwdPtr != '#'); ++fwdPtr)
    {
        if (*fwdPtr == '/' && *(fwdPtr+1) == '.' && *(fwdPtr+2) == '/' )
        {
            
            ++fwdPtr;
        }
        else if(*fwdPtr == '/' && *(fwdPtr+1) == '.' && *(fwdPtr+2) == '.' && 
                (*(fwdPtr+3) == '/' || 
                    *(fwdPtr+3) == '\0' || 
                    *(fwdPtr+3) == '?' ||  
                    *(fwdPtr+3) == '#'))
        {
            
            
            
            
            if(traversal > 0 || !(flags & 
                                  NET_COALESCE_ALLOW_RELATIVE_ROOT))
            { 
                if (urlPtr != path)
                    urlPtr--; 
                for(;*urlPtr != '/' && urlPtr != path; urlPtr--)
                    ;  
                --traversal; 
                
                fwdPtr += 2;
                
                
                
                
                
                if (urlPtr == path && special_ftp_len > 3) 
                {
                    ++urlPtr;
                    ++urlPtr;
                    ++urlPtr;
                }
                
                
                if (*fwdPtr == '.' && *(fwdPtr+1) == '\0')
                    ++urlPtr;
            } 
            else 
            {
                
                

                
                
                
                
                if (special_ftp_len > 3 && urlPtr == path+special_ftp_len-1)
                    ++urlPtr;
                else 
                    *urlPtr++ = *fwdPtr;
                ++fwdPtr;
                *urlPtr++ = *fwdPtr;
                ++fwdPtr;
                *urlPtr++ = *fwdPtr;
            }
        }
        else
        {
            
            
            if (*fwdPtr == '/' &&  *(fwdPtr+1) != '.' &&
               (special_ftp_len != 2 || *(fwdPtr+1) != '/'))
                traversal++;
            
            *urlPtr++ = *fwdPtr;
        }
    }

    




    if ((urlPtr > (path+1)) && (*(urlPtr-1) == '.') && (*(urlPtr-2) == '/'))
        urlPtr--;

    
    for (; *fwdPtr != '\0'; ++fwdPtr)
    {
        *urlPtr++ = *fwdPtr;
    }
    *urlPtr = '\0';  
}

nsresult
net_ResolveRelativePath(const nsACString &relativePath,
                        const nsACString &basePath,
                        nsACString &result)
{
    nsAutoCString name;
    nsAutoCString path(basePath);
    bool needsDelim = false;

    if ( !path.IsEmpty() ) {
        char16_t last = path.Last();
        needsDelim = !(last == '/');
    }

    nsACString::const_iterator beg, end;
    relativePath.BeginReading(beg);
    relativePath.EndReading(end);

    bool stop = false;
    char c;
    for (; !stop; ++beg) {
        c = (beg == end) ? '\0' : *beg;
        
        switch (c) {
          case '\0':
          case '#':
          case '?':
            stop = true;
            
          case '/':
            
            if (name.EqualsLiteral("..")) {
                
                
                
                int32_t offset = path.Length() - (needsDelim ? 1 : 2);
                
                if (offset < 0 ) 
                    return NS_ERROR_MALFORMED_URI;
                int32_t pos = path.RFind("/", false, offset);
                if (pos >= 0)
                    path.Truncate(pos + 1);
                else
                    path.Truncate();
            }
            else if (name.IsEmpty() || name.EqualsLiteral(".")) {
                
            }
            else {
                
                if (needsDelim)
                    path += '/';
                path += name;
                needsDelim = true;
            }
            name.Truncate();
            break;

          default:
            
            name += c;
        }
    }
    
    if (c != '\0')
        path += Substring(--beg, end);

    result = path;
    return NS_OK;
}






nsresult
net_ExtractURLScheme(const nsACString &inURI,
                     uint32_t *startPos, 
                     uint32_t *endPos,
                     nsACString *scheme)
{
    
    const nsPromiseFlatCString &flatURI = PromiseFlatCString(inURI);
    const char* uri_start = flatURI.get();
    const char* uri = uri_start;

    if (!uri)
        return NS_ERROR_MALFORMED_URI;

    
    while (nsCRT::IsAsciiSpace(*uri))
        uri++;

    uint32_t start = uri - uri_start;
    if (startPos) {
        *startPos = start;
    }

    uint32_t length = 0;
    char c;
    while ((c = *uri++) != '\0') {
        
        if (length == 0 && nsCRT::IsAsciiAlpha(c)) {
            length++;
        } 
        
        else if (length > 0 && (nsCRT::IsAsciiAlpha(c) || 
                 nsCRT::IsAsciiDigit(c) || c == '+' || 
                 c == '.' || c == '-')) {
            length++;
        }
        
        else if (c == ':' && length > 0) {
            if (endPos) {
                *endPos = start + length;
            }

            if (scheme)
                scheme->Assign(Substring(inURI, start, length));
            return NS_OK;
        }
        else 
            break;
    }
    return NS_ERROR_MALFORMED_URI;
}

bool
net_IsValidScheme(const char *scheme, uint32_t schemeLen)
{
    
    if (!nsCRT::IsAsciiAlpha(*scheme))
        return false;

    
    for (; schemeLen; ++scheme, --schemeLen) {
        if (!(nsCRT::IsAsciiAlpha(*scheme) ||
              nsCRT::IsAsciiDigit(*scheme) ||
              *scheme == '+' ||
              *scheme == '.' ||
              *scheme == '-'))
            return false;
    }

    return true;
}

bool
net_FilterURIString(const char *str, nsACString& result)
{
    NS_PRECONDITION(str, "Must have a non-null string!");
    bool writing = false;
    result.Truncate();
    const char *p = str;

    
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        writing = true;
        str = p + 1;
        p++;
    }

    
    
    
    bool found_colon = false;
    const char *first = nullptr;
    while (*p) {
        switch (*p) {
            case '\t': 
            case '\r': 
            case '\n':
                if (found_colon) {
                    writing = true;
                    
                    if (p > str)
                        result.Append(str, p - str);
                    str = p + 1;
                } else {
                    
                    if (!first)
                        first = p;
                }
                break;

            case ':':
                found_colon = true;
                break;

            case '/':
            case '@':
                if (!found_colon) {
                    
                    found_colon = true; 
                    if (first) {
                        
                        p = first;
                        continue; 
                    }
                }
                break;

            default:
                break;
        }
        p++;

        
        
        if (!*p && first != nullptr && !found_colon) {
            
            
            p = first;
            
            found_colon = true; 
        }
    }

    
    while (((p-1) >= str) && (*(p-1) == ' ')) {
        writing = true;
        p--;
    }

    if (writing && p > str)
        result.Append(str, p - str);

    return writing;
}

#if defined(XP_WIN)
bool
net_NormalizeFileURL(const nsACString &aURL, nsCString &aResultBuf)
{
    bool writing = false;

    nsACString::const_iterator beginIter, endIter;
    aURL.BeginReading(beginIter);
    aURL.EndReading(endIter);

    const char *s, *begin = beginIter.get();

    for (s = begin; s != endIter.get(); ++s)
    {
        if (*s == '\\')
        {
            writing = true;
            if (s > begin)
                aResultBuf.Append(begin, s - begin);
            aResultBuf += '/';
            begin = s + 1;
        }
    }
    if (writing && s > begin)
        aResultBuf.Append(begin, s - begin);

    return writing;
}
#endif





static inline
void ToLower(char &c)
{
    if ((unsigned)(c - 'A') <= (unsigned)('Z' - 'A'))
        c += 'a' - 'A';
}

void
net_ToLowerCase(char *str, uint32_t length)
{
    for (char *end = str + length; str < end; ++str)
        ToLower(*str);
}

void
net_ToLowerCase(char *str)
{
    for (; *str; ++str)
        ToLower(*str);
}

char *
net_FindCharInSet(const char *iter, const char *stop, const char *set)
{
    for (; iter != stop && *iter; ++iter) {
        for (const char *s = set; *s; ++s) {
            if (*iter == *s)
                return (char *) iter;
        }
    }
    return (char *) iter;
}

char *
net_FindCharNotInSet(const char *iter, const char *stop, const char *set)
{
repeat:
    for (const char *s = set; *s; ++s) {
        if (*iter == *s) {
            if (++iter == stop)
                break;
            goto repeat;
        }
    }
    return (char *) iter;
}

char *
net_RFindCharNotInSet(const char *stop, const char *iter, const char *set)
{
    --iter;
    --stop;

    if (iter == stop)
        return (char *) iter;

repeat:
    for (const char *s = set; *s; ++s) {
        if (*iter == *s) {
            if (--iter == stop)
                break;
            goto repeat;
        }
    }
    return (char *) iter;
}

#define HTTP_LWS " \t"


static uint32_t
net_FindStringEnd(const nsCString& flatStr,
                  uint32_t stringStart,
                  char stringDelim)
{
    NS_ASSERTION(stringStart < flatStr.Length() &&
                 flatStr.CharAt(stringStart) == stringDelim &&
                 (stringDelim == '"' || stringDelim == '\''),
                 "Invalid stringStart");

    const char set[] = { stringDelim, '\\', '\0' };
    do {
        
        
                
        
        
        
        uint32_t stringEnd = flatStr.FindCharInSet(set, stringStart + 1);
        if (stringEnd == uint32_t(kNotFound))
            return flatStr.Length();

        if (flatStr.CharAt(stringEnd) == '\\') {
            
            stringStart = stringEnd + 1;
            if (stringStart == flatStr.Length())
                return stringStart;

            
            continue;
        }

        return stringEnd;

    } while (true);

    NS_NOTREACHED("How did we get here?");
    return flatStr.Length();
}
                  

static uint32_t
net_FindMediaDelimiter(const nsCString& flatStr,
                       uint32_t searchStart,
                       char delimiter)
{
    do {
        
        
        const char delimStr[] = { delimiter, '"', '\0' };
        uint32_t curDelimPos = flatStr.FindCharInSet(delimStr, searchStart);
        if (curDelimPos == uint32_t(kNotFound))
            return flatStr.Length();
            
        char ch = flatStr.CharAt(curDelimPos);
        if (ch == delimiter) {
            
            return curDelimPos;
        }

        
        searchStart = net_FindStringEnd(flatStr, curDelimPos, ch);
        if (searchStart == flatStr.Length())
            return searchStart;

        ++searchStart;

        
        
        
    } while (true);

    NS_NOTREACHED("How did we get here?");
    return flatStr.Length();
}



static void
net_ParseMediaType(const nsACString &aMediaTypeStr,
                   nsACString       &aContentType,
                   nsACString       &aContentCharset,
                   int32_t          aOffset,
                   bool             *aHadCharset,
                   int32_t          *aCharsetStart,
                   int32_t          *aCharsetEnd)
{
    const nsCString& flatStr = PromiseFlatCString(aMediaTypeStr);
    const char* start = flatStr.get();
    const char* end = start + flatStr.Length();

    
    
    
    const char* type = net_FindCharNotInSet(start, end, HTTP_LWS);
    const char* typeEnd = net_FindCharInSet(type, end, HTTP_LWS ";(");

    const char* charset = "";
    const char* charsetEnd = charset;
    int32_t charsetParamStart = 0;
    int32_t charsetParamEnd = 0;

    
    bool typeHasCharset = false;
    uint32_t paramStart = flatStr.FindChar(';', typeEnd - start);
    if (paramStart != uint32_t(kNotFound)) {
        
        uint32_t curParamStart = paramStart + 1;
        do {
            uint32_t curParamEnd =
                net_FindMediaDelimiter(flatStr, curParamStart, ';');

            const char* paramName = net_FindCharNotInSet(start + curParamStart,
                                                         start + curParamEnd,
                                                         HTTP_LWS);
            static const char charsetStr[] = "charset=";
            if (PL_strncasecmp(paramName, charsetStr,
                               sizeof(charsetStr) - 1) == 0) {
                charset = paramName + sizeof(charsetStr) - 1;
                charsetEnd = start + curParamEnd;
                typeHasCharset = true;
                charsetParamStart = curParamStart - 1;
                charsetParamEnd = curParamEnd;
            }

            curParamStart = curParamEnd + 1;
        } while (curParamStart < flatStr.Length());
    }

    bool charsetNeedsQuotedStringUnescaping = false;
    if (typeHasCharset) {
        
        
        
        charset = net_FindCharNotInSet(charset, charsetEnd, HTTP_LWS);
        if (*charset == '"') {
            charsetNeedsQuotedStringUnescaping = true;
            charsetEnd =
                start + net_FindStringEnd(flatStr, charset - start, *charset);
            charset++;
            NS_ASSERTION(charsetEnd >= charset, "Bad charset parsing");
        } else {
            charsetEnd = net_FindCharInSet(charset, charsetEnd, HTTP_LWS ";(");
        }
    }

    
    
    
    
    
    
    

    if (type != typeEnd && strncmp(type, "*/*", typeEnd - type) != 0 &&
        memchr(type, '/', typeEnd - type) != nullptr) {
        
        bool eq = !aContentType.IsEmpty() &&
            aContentType.Equals(Substring(type, typeEnd),
                                nsCaseInsensitiveCStringComparator());
        if (!eq) {
            aContentType.Assign(type, typeEnd - type);
            ToLowerCase(aContentType);
        }

        if ((!eq && *aHadCharset) || typeHasCharset) {
            *aHadCharset = true;
            if (charsetNeedsQuotedStringUnescaping) {
                
                
                aContentCharset.Truncate();
                for (const char *c = charset; c != charsetEnd; c++) {
                    if (*c == '\\' && c + 1 != charsetEnd) {
                        
                        c++;  
                    }
                    aContentCharset.Append(*c);
                }
            }
            else {
                aContentCharset.Assign(charset, charsetEnd - charset);
            }
            if (typeHasCharset) {
                *aCharsetStart = charsetParamStart + aOffset;
                *aCharsetEnd = charsetParamEnd + aOffset;
            }
        }
        
        
        
        
        if (!eq && !typeHasCharset) {
            int32_t charsetStart = int32_t(paramStart);
            if (charsetStart == kNotFound)
                charsetStart =  flatStr.Length();

            *aCharsetEnd = *aCharsetStart = charsetStart + aOffset;
        }
    }
}

#undef HTTP_LWS

void
net_ParseContentType(const nsACString &aHeaderStr,
                     nsACString       &aContentType,
                     nsACString       &aContentCharset,
                     bool             *aHadCharset)
{
    int32_t dummy1, dummy2;
    net_ParseContentType(aHeaderStr, aContentType, aContentCharset,
                         aHadCharset, &dummy1, &dummy2);
}

void
net_ParseContentType(const nsACString &aHeaderStr,
                     nsACString       &aContentType,
                     nsACString       &aContentCharset,
                     bool             *aHadCharset,
                     int32_t          *aCharsetStart,
                     int32_t          *aCharsetEnd)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    *aHadCharset = false;
    const nsCString& flatStr = PromiseFlatCString(aHeaderStr);
    
    
    
    uint32_t curTypeStart = 0;
    do {
        
        
        uint32_t curTypeEnd =
            net_FindMediaDelimiter(flatStr, curTypeStart, ',');
        
        
        
        net_ParseMediaType(Substring(flatStr, curTypeStart,
                                     curTypeEnd - curTypeStart),
                           aContentType, aContentCharset, curTypeStart,
                           aHadCharset, aCharsetStart, aCharsetEnd);

        
        curTypeStart = curTypeEnd + 1;
    } while (curTypeStart < flatStr.Length());
}

bool
net_IsValidHostName(const nsCSubstring &host)
{
    const char *end = host.EndReading();
    
    
    
    
    
    

    
    
    
    if (net_FindCharNotInSet(host.BeginReading(), end,
                             "abcdefghijklmnopqrstuvwxyz"
                             ".-0123456789"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ$+_") == end)
        return true;

    
    nsAutoCString strhost(host);
    PRNetAddr addr;
    return PR_StringToNetAddr(strhost.get(), &addr) == PR_SUCCESS;
}

bool
net_IsValidIPv4Addr(const char *addr, int32_t addrLen)
{
    RangedPtr<const char> p(addr, addrLen);

    int32_t octet = -1;   
    int32_t dotCount = 0; 

    for (; addrLen; ++p, --addrLen) {
        if (*p == '.') {
            dotCount++;
            if (octet == -1) {
                
                return false;
            }
            octet = -1;
        } else if (*p >= '0' && *p <='9') {
            if (octet == 0) {
                
                return false;
            } else if (octet == -1) {
                octet = *p - '0';
            } else {
                octet *= 10;
                octet += *p - '0';
                if (octet > 255)
                    return false;
            }
        } else {
            
            return false;
        }
    }

    return (dotCount == 3 && octet != -1);
}

bool
net_IsValidIPv6Addr(const char *addr, int32_t addrLen)
{
    RangedPtr<const char> p(addr, addrLen);

    int32_t digits = 0; 
    int32_t colons = 0; 
    int32_t blocks = 0; 
    bool haveZeros = false; 

    for (; addrLen; ++p, --addrLen) {
        if (*p == ':') {
            if (colons == 0) {
                if (digits != 0) {
                    digits = 0;
                    blocks++;
                }
            } else if (colons == 1) {
                if (haveZeros)
                    return false; 
                haveZeros = true;
            } else {
                
                return false;
            }
            colons++;
        } else if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
                   (*p >= 'A' && *p <= 'F')) {
            if (colons == 1 && blocks == 0) 
                return false;
            if (digits == 4) 
                return false;
            colons = 0;
            digits++;
        } else if (*p == '.') {
            
            if (!net_IsValidIPv4Addr(p.get() - digits, addrLen + digits))
                return false;
            return (haveZeros && blocks < 6) || (!haveZeros && blocks == 6);
        } else {
            
            return false;
        }
    }

    if (colons == 1) 
        return false;

    if (digits) 
        blocks++;

    return (haveZeros && blocks < 8) || (!haveZeros && blocks == 8);
}
