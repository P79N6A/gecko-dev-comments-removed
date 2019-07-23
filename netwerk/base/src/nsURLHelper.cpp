






































#include "nsURLHelper.h"
#include "nsReadableUtils.h"
#include "nsIServiceManager.h"
#include "nsIIOService.h"
#include "nsILocalFile.h"
#include "nsIURLParser.h"
#include "nsIURI.h"
#include "nsMemory.h"
#include "nsEscape.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsNetCID.h"
#include "netCore.h"
#include "prprf.h"
#include "prnetdb.h"





static PRBool gInitialized = PR_FALSE;
static nsIURLParser *gNoAuthURLParser = nsnull;
static nsIURLParser *gAuthURLParser = nsnull;
static nsIURLParser *gStdURLParser = nsnull;

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

    gInitialized = PR_TRUE;
}

void
net_ShutdownURLHelper()
{
    if (gInitialized) {
        NS_IF_RELEASE(gNoAuthURLParser);
        NS_IF_RELEASE(gAuthURLParser);
        NS_IF_RELEASE(gStdURLParser);
        gInitialized = PR_FALSE;
    }
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
    nsCAutoString escPath;
    nsresult rv = net_GetURLSpecFromActualFile(aFile, escPath);
    if (NS_FAILED(rv))
        return rv;

    if (escPath.Last() != '/') {
        escPath += '/';
    }
    
    result = escPath;
    return NS_OK;
}

nsresult
net_GetURLSpecFromFile(nsIFile *aFile, nsACString &result)
{
    nsCAutoString escPath;
    nsresult rv = net_GetURLSpecFromActualFile(aFile, escPath);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    
    if (escPath.Last() != '/') {
        PRBool dir;
        rv = aFile->IsDirectory(&dir);
        if (NS_SUCCEEDED(rv) && dir)
            escPath += '/';
    }
    
    result = escPath;
    return NS_OK;
}





nsresult
net_ParseFileURL(const nsACString &inURL,
                 nsACString &outDirectory,
                 nsACString &outFileBaseName,
                 nsACString &outFileExtension)
{
    nsresult rv;

    outDirectory.Truncate();
    outFileBaseName.Truncate();
    outFileExtension.Truncate();

    const nsPromiseFlatCString &flatURL = PromiseFlatCString(inURL);
    const char *url = flatURL.get();
    
    PRUint32 schemeBeg, schemeEnd;
    rv = net_ExtractURLScheme(flatURL, &schemeBeg, &schemeEnd, nsnull);
    if (NS_FAILED(rv)) return rv;

    if (strncmp(url + schemeBeg, "file", schemeEnd - schemeBeg) != 0) {
        NS_ERROR("must be a file:// url");
        return NS_ERROR_UNEXPECTED;
    }

    nsIURLParser *parser = net_GetNoAuthURLParser();
    NS_ENSURE_TRUE(parser, NS_ERROR_UNEXPECTED);

    PRUint32 pathPos, filepathPos, directoryPos, basenamePos, extensionPos;
    PRInt32 pathLen, filepathLen, directoryLen, basenameLen, extensionLen;

    
    rv = parser->ParseURL(url, flatURL.Length(),
                          nsnull, nsnull, 
                          nsnull, nsnull, 
                          &pathPos, &pathLen);
    if (NS_FAILED(rv)) return rv;

    
    rv = parser->ParsePath(url + pathPos, pathLen,
                           &filepathPos, &filepathLen,
                           nsnull, nsnull,  
                           nsnull, nsnull,  
                           nsnull, nsnull); 
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
    PRUint32 traversal = 0;
    PRUint32 special_ftp_len = 0;

    
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
    nsCAutoString name;
    nsCAutoString path(basePath);
    PRBool needsDelim = PR_FALSE;

    if ( !path.IsEmpty() ) {
        PRUnichar last = path.Last();
        needsDelim = !(last == '/');
    }

    nsACString::const_iterator beg, end;
    relativePath.BeginReading(beg);
    relativePath.EndReading(end);

    PRBool stop = PR_FALSE;
    char c;
    for (; !stop; ++beg) {
        c = (beg == end) ? '\0' : *beg;
        
        switch (c) {
          case '\0':
          case '#':
          case ';':
          case '?':
            stop = PR_TRUE;
            
          case '/':
            
            if (name.EqualsLiteral("..")) {
                
                
                
                PRInt32 offset = path.Length() - (needsDelim ? 1 : 2);
                
                if (offset < 0 ) 
                    return NS_ERROR_MALFORMED_URI;
                PRInt32 pos = path.RFind("/", PR_FALSE, offset);
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
                needsDelim = PR_TRUE;
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
                     PRUint32 *startPos, 
                     PRUint32 *endPos,
                     nsACString *scheme)
{
    
    const nsPromiseFlatCString &flatURI = PromiseFlatCString(inURI);
    const char* uri_start = flatURI.get();
    const char* uri = uri_start;

    if (!uri)
        return NS_ERROR_MALFORMED_URI;

    
    while (nsCRT::IsAsciiSpace(*uri))
        uri++;

    PRUint32 start = uri - uri_start;
    if (startPos) {
        *startPos = start;
    }

    PRUint32 length = 0;
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

PRBool
net_IsValidScheme(const char *scheme, PRUint32 schemeLen)
{
    
    if (!nsCRT::IsAsciiAlpha(*scheme))
        return PR_FALSE;

    
    for (; schemeLen; ++scheme, --schemeLen) {
        if (!(nsCRT::IsAsciiAlpha(*scheme) ||
              nsCRT::IsAsciiDigit(*scheme) ||
              *scheme == '+' ||
              *scheme == '.' ||
              *scheme == '-'))
            return PR_FALSE;
    }

    return PR_TRUE;
}

PRBool
net_FilterURIString(const char *str, nsACString& result)
{
    NS_PRECONDITION(str, "Must have a non-null string!");
    PRBool writing = PR_FALSE;
    result.Truncate();
    const char *p = str;

    
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
        writing = PR_TRUE;
        str = p + 1;
        p++;
    }

    while (*p) {
        if (*p == '\t' || *p == '\r' || *p == '\n') {
            writing = PR_TRUE;
            
            if (p > str)
                result.Append(str, p - str);
            str = p + 1;
        }
        p++;
    }

    
    while (((p-1) >= str) && (*(p-1) == ' ')) {
        writing = PR_TRUE;
        p--;
    }

    if (writing && p > str)
        result.Append(str, p - str);

    return writing;
}

#if defined(XP_WIN) || defined(XP_OS2)
PRBool
net_NormalizeFileURL(const nsACString &aURL, nsCString &aResultBuf)
{
    PRBool writing = PR_FALSE;

    nsACString::const_iterator beginIter, endIter;
    aURL.BeginReading(beginIter);
    aURL.EndReading(endIter);

    const char *s, *begin = beginIter.get();

    for (s = begin; s != endIter.get(); ++s)
    {
        if (*s == '\\')
        {
            writing = PR_TRUE;
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
net_ToLowerCase(char *str, PRUint32 length)
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


static PRUint32
net_FindStringEnd(const nsCString& flatStr,
                  PRUint32 stringStart,
                  char stringDelim)
{
    NS_ASSERTION(stringStart < flatStr.Length() &&
                 flatStr.CharAt(stringStart) == stringDelim &&
                 (stringDelim == '"' || stringDelim == '\''),
                 "Invalid stringStart");

    const char set[] = { stringDelim, '\\', '\0' };
    do {
        
        
                
        
        
        
        PRUint32 stringEnd = flatStr.FindCharInSet(set, stringStart + 1);
        if (stringEnd == PRUint32(kNotFound))
            return flatStr.Length();

        if (flatStr.CharAt(stringEnd) == '\\') {
            
            stringStart = stringEnd + 1;
            if (stringStart == flatStr.Length())
                return stringStart;

            
            continue;
        }

        return stringEnd;

    } while (PR_TRUE);

    NS_NOTREACHED("How did we get here?");
    return flatStr.Length();
}
                  

static PRUint32
net_FindMediaDelimiter(const nsCString& flatStr,
                       PRUint32 searchStart,
                       char delimiter)
{
    do {
        
        
        const char delimStr[] = { delimiter, '"', '\'', '\0' };
        PRUint32 curDelimPos = flatStr.FindCharInSet(delimStr, searchStart);
        if (curDelimPos == PRUint32(kNotFound))
            return flatStr.Length();
            
        char ch = flatStr.CharAt(curDelimPos);
        if (ch == delimiter) {
            
            return curDelimPos;
        }

        
        searchStart = net_FindStringEnd(flatStr, curDelimPos, ch);
        if (searchStart == flatStr.Length())
            return searchStart;

        ++searchStart;

        
        
        
    } while (PR_TRUE);

    NS_NOTREACHED("How did we get here?");
    return flatStr.Length();
}



static void
net_ParseMediaType(const nsACString &aMediaTypeStr,
                   nsACString       &aContentType,
                   nsACString       &aContentCharset,
                   PRInt32          aOffset,
                   PRBool           *aHadCharset,
                   PRInt32          *aCharsetStart,
                   PRInt32          *aCharsetEnd)
{
    const nsCString& flatStr = PromiseFlatCString(aMediaTypeStr);
    const char* start = flatStr.get();
    const char* end = start + flatStr.Length();

    
    
    
    const char* type = net_FindCharNotInSet(start, end, HTTP_LWS);
    const char* typeEnd = net_FindCharInSet(type, end, HTTP_LWS ";(");

    const char* charset = "";
    const char* charsetEnd = charset;
    PRInt32 charsetParamStart;
    PRInt32 charsetParamEnd;

    
    PRBool typeHasCharset = PR_FALSE;
    PRUint32 paramStart = flatStr.FindChar(';', typeEnd - start);
    if (paramStart != PRUint32(kNotFound)) {
        
        PRUint32 curParamStart = paramStart + 1;
        do {
            PRUint32 curParamEnd =
                net_FindMediaDelimiter(flatStr, curParamStart, ';');

            const char* paramName = net_FindCharNotInSet(start + curParamStart,
                                                         start + curParamEnd,
                                                         HTTP_LWS);
            static const char charsetStr[] = "charset=";
            if (PL_strncasecmp(paramName, charsetStr,
                               sizeof(charsetStr) - 1) == 0) {
                charset = paramName + sizeof(charsetStr) - 1;
                charsetEnd = start + curParamEnd;
                typeHasCharset = PR_TRUE;
                charsetParamStart = curParamStart - 1;
                charsetParamEnd = curParamEnd;
            }

            curParamStart = curParamEnd + 1;
        } while (curParamStart < flatStr.Length());
    }

    if (typeHasCharset) {
        
        
        
        charset = net_FindCharNotInSet(charset, charsetEnd, HTTP_LWS);
        if (*charset == '"' || *charset == '\'') {
            charsetEnd =
                start + net_FindStringEnd(flatStr, charset - start, *charset);
            charset++;
            NS_ASSERTION(charsetEnd >= charset, "Bad charset parsing");
        } else {
            charsetEnd = net_FindCharInSet(charset, charsetEnd, HTTP_LWS ";(");
        }
    }

    
    
    
    
    
    
    

    if (type != typeEnd && strncmp(type, "*/*", typeEnd - type) != 0 &&
        memchr(type, '/', typeEnd - type) != NULL) {
        
        PRBool eq = !aContentType.IsEmpty() &&
            aContentType.Equals(Substring(type, typeEnd),
                                nsCaseInsensitiveCStringComparator());
        if (!eq) {
            aContentType.Assign(type, typeEnd - type);
            ToLowerCase(aContentType);
        }

        if ((!eq && *aHadCharset) || typeHasCharset) {
            *aHadCharset = PR_TRUE;
            aContentCharset.Assign(charset, charsetEnd - charset);
            if (typeHasCharset) {
                *aCharsetStart = charsetParamStart + aOffset;
                *aCharsetEnd = charsetParamEnd + aOffset;
            }
        }
        
        
        
        
        if (!eq && !typeHasCharset) {
            PRInt32 charsetStart = PRInt32(paramStart);
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
                     PRBool           *aHadCharset)
{
    PRInt32 dummy1, dummy2;
    net_ParseContentType(aHeaderStr, aContentType, aContentCharset,
                         aHadCharset, &dummy1, &dummy2);
}

void
net_ParseContentType(const nsACString &aHeaderStr,
                     nsACString       &aContentType,
                     nsACString       &aContentCharset,
                     PRBool           *aHadCharset,
                     PRInt32          *aCharsetStart,
                     PRInt32          *aCharsetEnd)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    *aHadCharset = PR_FALSE;
    const nsCString& flatStr = PromiseFlatCString(aHeaderStr);
    
    
    
    PRUint32 curTypeStart = 0;
    do {
        
        
        PRUint32 curTypeEnd =
            net_FindMediaDelimiter(flatStr, curTypeStart, ',');
        
        
        
        net_ParseMediaType(Substring(flatStr, curTypeStart,
                                     curTypeEnd - curTypeStart),
                           aContentType, aContentCharset, curTypeStart,
                           aHadCharset, aCharsetStart, aCharsetEnd);

        
        curTypeStart = curTypeEnd + 1;
    } while (curTypeStart < flatStr.Length());
}

PRBool
net_IsValidHostName(const nsCSubstring &host)
{
    const char *end = host.EndReading();
    
    
    
    
    
    

    
    
    
    if (net_FindCharNotInSet(host.BeginReading(), end,
                             "abcdefghijklmnopqrstuvwxyz"
                             ".-0123456789"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ$+_") == end)
        return PR_TRUE;

    
    nsCAutoString strhost(host);
    PRNetAddr addr;
    return PR_StringToNetAddr(strhost.get(), &addr) == PR_SUCCESS;
}
