





































#include <string.h>
#include "nsURLParsers.h"
#include "nsURLHelper.h"
#include "nsIURI.h"
#include "prtypes.h"
#include "nsString.h"
#include "nsCRT.h"
#include "netCore.h"



static PRUint32
CountConsecutiveSlashes(const char *str, PRInt32 len)
{
    PRUint32 count = 0;
    while (len-- && *str++ == '/') ++count;
    return count;
}








NS_IMPL_THREADSAFE_ISUPPORTS1(nsBaseURLParser, nsIURLParser)

#define SET_RESULT(component, pos, len) \
    PR_BEGIN_MACRO \
        if (component ## Pos) \
           *component ## Pos = PRUint32(pos); \
        if (component ## Len) \
           *component ## Len = PRInt32(len); \
    PR_END_MACRO

#define OFFSET_RESULT(component, offset) \
    PR_BEGIN_MACRO \
        if (component ## Pos) \
           *component ## Pos += offset; \
    PR_END_MACRO

NS_IMETHODIMP
nsBaseURLParser::ParseURL(const char *spec, PRInt32 specLen,
                          PRUint32 *schemePos, PRInt32 *schemeLen,
                          PRUint32 *authorityPos, PRInt32 *authorityLen,
                          PRUint32 *pathPos, PRInt32 *pathLen)
{
    NS_PRECONDITION(spec, "null pointer");

    if (specLen < 0)
        specLen = strlen(spec);

    const char *stop = nsnull;
    const char *colon = nsnull;
    const char *slash = nsnull;
    const char *p;
    PRUint32 offset = 0;
    PRInt32 len = specLen;
    for (p = spec; len && *p && !colon && !slash; ++p, --len) {
        
        if (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') {
            spec++;
            specLen--;
            offset++;
            continue;
        }
        switch (*p) {
            case ':':
                if (!colon)
                    colon = p;
                break;
            case '/': 
            case '?': 
            case '#': 
            case ';': 
                if (!slash)
                    slash = p;
                break;
            case '@': 
            case '[': 
                if (!stop)
                    stop = p;
                break;
        }
    }
    
    if (colon && stop && colon > stop)
        colon = nsnull;

    
    if (specLen == 0) {
        SET_RESULT(scheme, 0, -1);
        SET_RESULT(authority, 0, 0);
        SET_RESULT(path, 0, 0);
        return NS_OK;
    }

    
    for (p = spec + specLen - 1; ((unsigned char) *p <= ' ') && (p != spec); --p)
        ;

    specLen = p - spec + 1;

    if (colon && (colon < slash || !slash)) {
        
        
        
        
        
        
        
        
        if (!net_IsValidScheme(spec, colon - spec) || (*(colon+1) == ':')) {
            NS_WARNING("malformed uri");
            return NS_ERROR_MALFORMED_URI;
        }
        SET_RESULT(scheme, offset, colon - spec);
        if (authorityLen || pathLen) {
            PRUint32 schemeLen = colon + 1 - spec;
            offset += schemeLen;
            ParseAfterScheme(colon + 1, specLen - schemeLen,
                             authorityPos, authorityLen,
                             pathPos, pathLen);
            OFFSET_RESULT(authority, offset);
            OFFSET_RESULT(path, offset);
        }
    }
    else {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        SET_RESULT(scheme, 0, -1);
        if (authorityLen || pathLen)
            ParseAfterScheme(spec, specLen,
                             authorityPos, authorityLen,
                             pathPos, pathLen);
            OFFSET_RESULT(authority, offset);
            OFFSET_RESULT(path, offset);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsBaseURLParser::ParseAuthority(const char *auth, PRInt32 authLen,
                                PRUint32 *usernamePos, PRInt32 *usernameLen,
                                PRUint32 *passwordPos, PRInt32 *passwordLen,
                                PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                                PRInt32 *port)
{
    NS_PRECONDITION(auth, "null pointer");

    if (authLen < 0)
        authLen = strlen(auth);

    SET_RESULT(username, 0, -1);
    SET_RESULT(password, 0, -1);
    SET_RESULT(hostname, 0, authLen);
    if (port)
       *port = -1;
    return NS_OK;
}

NS_IMETHODIMP
nsBaseURLParser::ParseUserInfo(const char *userinfo, PRInt32 userinfoLen,
                               PRUint32 *usernamePos, PRInt32 *usernameLen,
                               PRUint32 *passwordPos, PRInt32 *passwordLen)
{
    SET_RESULT(username, 0, -1);
    SET_RESULT(password, 0, -1);
    return NS_OK;
}

NS_IMETHODIMP
nsBaseURLParser::ParseServerInfo(const char *serverinfo, PRInt32 serverinfoLen,
                                 PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                                 PRInt32 *port)
{
    SET_RESULT(hostname, 0, -1);
    if (port)
       *port = -1;
    return NS_OK;
}

NS_IMETHODIMP
nsBaseURLParser::ParsePath(const char *path, PRInt32 pathLen,
                           PRUint32 *filepathPos, PRInt32 *filepathLen,
                           PRUint32 *paramPos, PRInt32 *paramLen,
                           PRUint32 *queryPos, PRInt32 *queryLen,
                           PRUint32 *refPos, PRInt32 *refLen)
{
    NS_PRECONDITION(path, "null pointer");

    if (pathLen < 0)
        pathLen = strlen(path);

    

    

    
    const char *query_beg = 0, *query_end = 0;
    const char *ref_beg = 0;
    const char *p = 0;
    for (p = path; p < path + pathLen; ++p) {
        
        if (!ref_beg && !query_beg && *p == '?')
            query_beg = p + 1;
        else if (*p == '#') {
            ref_beg = p + 1;
            if (query_beg)
                query_end = p;
            break;
        }
    }

    if (query_beg) {
        if (query_end)
            SET_RESULT(query, query_beg - path, query_end - query_beg);
        else
            SET_RESULT(query, query_beg - path, pathLen - (query_beg - path));
    }
    else
        SET_RESULT(query, 0, -1);

    if (ref_beg)
        SET_RESULT(ref, ref_beg - path, pathLen - (ref_beg - path));
    else
        SET_RESULT(ref, 0, -1);

    
    const char *param_beg = 0;
    const char *end;
    if (query_beg)
        end = query_beg - 1;
    else if (ref_beg)
        end = ref_beg - 1;
    else
        end = path + pathLen;
    for (p = end - 1; p >= path && *p != '/'; --p) {
        if (*p == ';') {
            
            param_beg = p + 1;
        }
    }

    if (param_beg) {
        
        SET_RESULT(param, param_beg - path, end - param_beg);
        end = param_beg - 1;
    }
    else
        SET_RESULT(param, 0, -1);

    
    if (end != path)
        SET_RESULT(filepath, 0, end - path);
    else
        SET_RESULT(filepath, 0, -1);
    return NS_OK;
}

NS_IMETHODIMP
nsBaseURLParser::ParseFilePath(const char *filepath, PRInt32 filepathLen,
                               PRUint32 *directoryPos, PRInt32 *directoryLen,
                               PRUint32 *basenamePos, PRInt32 *basenameLen,
                               PRUint32 *extensionPos, PRInt32 *extensionLen)
{
    NS_PRECONDITION(filepath, "null pointer");

    if (filepathLen < 0)
        filepathLen = strlen(filepath);

    if (filepathLen == 0) {
        SET_RESULT(directory, 0, -1);
        SET_RESULT(basename, 0, 0); 
        SET_RESULT(extension, 0, -1);
        return NS_OK;
    }

    const char *p;
    const char *end = filepath + filepathLen;

    
    for (p = end - 1; *p != '/' && p > filepath; --p)
        ;
    if (*p == '/') {
        
        if ((p+1 < end && *(p+1) == '.') && 
           (p+2 == end || (*(p+2) == '.' && p+3 == end)))
            p = end - 1;
        
        SET_RESULT(directory, 0, p - filepath + 1);
        ParseFileName(p + 1, end - (p + 1),
                      basenamePos, basenameLen,
                      extensionPos, extensionLen);
        OFFSET_RESULT(basename, p + 1 - filepath);
        OFFSET_RESULT(extension, p + 1 - filepath);
    }
    else {
        
        SET_RESULT(directory, 0, -1);
        ParseFileName(filepath, filepathLen,
                      basenamePos, basenameLen,
                      extensionPos, extensionLen);
    }
    return NS_OK;
}

nsresult
nsBaseURLParser::ParseFileName(const char *filename, PRInt32 filenameLen,
                               PRUint32 *basenamePos, PRInt32 *basenameLen,
                               PRUint32 *extensionPos, PRInt32 *extensionLen)
{
    NS_PRECONDITION(filename, "null pointer");

    if (filenameLen < 0)
        filenameLen = strlen(filename);

    
    if (filename[filenameLen-1] != '.') {
        
        for (const char *p = filename + filenameLen - 1; p > filename; --p) {
            if (*p == '.') {
                
                SET_RESULT(basename, 0, p - filename);
                SET_RESULT(extension, p + 1 - filename, filenameLen - (p - filename + 1));
                return NS_OK;
            }
        }
    }
    
    SET_RESULT(basename, 0, filenameLen);
    SET_RESULT(extension, 0, -1);
    return NS_OK;
}





NS_IMETHODIMP
nsNoAuthURLParser::ParseAuthority(const char *auth, PRInt32 authLen,
                                 PRUint32 *usernamePos, PRInt32 *usernameLen,
                                 PRUint32 *passwordPos, PRInt32 *passwordLen,
                                 PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                                 PRInt32 *port)
{
    NS_NOTREACHED("Shouldn't parse auth in a NoAuthURL!");
    return NS_ERROR_UNEXPECTED;
}

void
nsNoAuthURLParser::ParseAfterScheme(const char *spec, PRInt32 specLen,
                                    PRUint32 *authPos, PRInt32 *authLen,
                                    PRUint32 *pathPos, PRInt32 *pathLen)
{
    NS_PRECONDITION(specLen >= 0, "unexpected");

    
    PRUint32 pos = 0;
    switch (CountConsecutiveSlashes(spec, specLen)) {
    case 0:
    case 1:
        break;
    case 2:
        {
            const char *p = nsnull;
            if (specLen > 2) {
                
#if defined(XP_WIN) || defined(XP_OS2)
                
                
                if ((specLen > 3) && (spec[3] == ':' || spec[3] == '|') &&
                    nsCRT::IsAsciiAlpha(spec[2]) &&
                    ((specLen == 4) || (spec[4] == '/') || (spec[4] == '\\'))) {
                    pos = 1;
                    break;  
                } 
#endif
                p = (const char *) memchr(spec + 2, '/', specLen - 2);
            }
            if (p) {
                SET_RESULT(auth, 0, -1);
                SET_RESULT(path, p - spec, specLen - (p - spec));
            }
            else {
                SET_RESULT(auth, 0, -1);
                SET_RESULT(path, 0, -1);
            }
            return;
        }
    default:
        pos = 2;
        break;
    }
    SET_RESULT(auth, pos, 0);
    SET_RESULT(path, pos, specLen - pos);
}

#if defined(XP_WIN) || defined(XP_OS2)
NS_IMETHODIMP
nsNoAuthURLParser::ParseFilePath(const char *filepath, PRInt32 filepathLen,
                                 PRUint32 *directoryPos, PRInt32 *directoryLen,
                                 PRUint32 *basenamePos, PRInt32 *basenameLen,
                                 PRUint32 *extensionPos, PRInt32 *extensionLen)
{
    NS_PRECONDITION(filepath, "null pointer");

    if (filepathLen < 0)
        filepathLen = strlen(filepath);

    
    
    if (filepathLen > 1 && filepathLen < 4) {
        const char *end = filepath + filepathLen;
        const char *p = filepath;
        if (*p == '/')
            p++;
        if ((end-p == 2) && (p[1]==':' || p[1]=='|') && nsCRT::IsAsciiAlpha(*p)) {
            
            SET_RESULT(directory, 0, filepathLen);
            SET_RESULT(basename, 0, -1);
            SET_RESULT(extension, 0, -1);
            return NS_OK;
        }
    }

    
    return nsBaseURLParser::ParseFilePath(filepath, filepathLen,
                                          directoryPos, directoryLen,
                                          basenamePos, basenameLen,
                                          extensionPos, extensionLen);
}
#endif





NS_IMETHODIMP
nsAuthURLParser::ParseAuthority(const char *auth, PRInt32 authLen,
                                PRUint32 *usernamePos, PRInt32 *usernameLen,
                                PRUint32 *passwordPos, PRInt32 *passwordLen,
                                PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                                PRInt32 *port)
{
    nsresult rv;

    NS_PRECONDITION(auth, "null pointer");

    if (authLen < 0)
        authLen = strlen(auth);

    if (authLen == 0) {
        SET_RESULT(username, 0, -1);
        SET_RESULT(password, 0, -1);
        SET_RESULT(hostname, 0, 0);
        if (port)
            *port = -1;
        return NS_OK;
    }

    
    const char *p = auth + authLen - 1;
    for (; (*p != '@') && (p > auth); --p);
    if ( *p == '@' ) {
        
        rv = ParseUserInfo(auth, p - auth,
                           usernamePos, usernameLen,
                           passwordPos, passwordLen);
        if (NS_FAILED(rv)) return rv;
        rv = ParseServerInfo(p + 1, authLen - (p - auth + 1),
                             hostnamePos, hostnameLen,
                             port);
        if (NS_FAILED(rv)) return rv;
        OFFSET_RESULT(hostname, p + 1 - auth);
    }
    else {
        
        SET_RESULT(username, 0, -1);
        SET_RESULT(password, 0, -1);
        rv = ParseServerInfo(auth, authLen,
                             hostnamePos, hostnameLen,
                             port);
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsAuthURLParser::ParseUserInfo(const char *userinfo, PRInt32 userinfoLen,
                               PRUint32 *usernamePos, PRInt32 *usernameLen,
                               PRUint32 *passwordPos, PRInt32 *passwordLen)
{
    NS_PRECONDITION(userinfo, "null pointer");

    if (userinfoLen < 0)
        userinfoLen = strlen(userinfo);

    if (userinfoLen == 0) {
        SET_RESULT(username, 0, -1);
        SET_RESULT(password, 0, -1);
        return NS_OK;
    }

    const char *p = (const char *) memchr(userinfo, ':', userinfoLen);
    if (p) {
        
        if (p == userinfo) {
            
            return NS_ERROR_MALFORMED_URI;
        }
        SET_RESULT(username, 0, p - userinfo);
        SET_RESULT(password, p - userinfo + 1, userinfoLen - (p - userinfo + 1));
    }
    else {
        
        SET_RESULT(username, 0, userinfoLen);
        SET_RESULT(password, 0, -1);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsAuthURLParser::ParseServerInfo(const char *serverinfo, PRInt32 serverinfoLen,
                                 PRUint32 *hostnamePos, PRInt32 *hostnameLen,
                                 PRInt32 *port)
{
    NS_PRECONDITION(serverinfo, "null pointer");

    if (serverinfoLen < 0)
        serverinfoLen = strlen(serverinfo);

    if (serverinfoLen == 0) {
        SET_RESULT(hostname, 0, 0);
        if (port)
            *port = -1;
        return NS_OK;
    }

    
    
    const char *p = serverinfo + serverinfoLen - 1;
    const char *colon = nsnull, *bracket = nsnull;
    for (; p > serverinfo; --p) {
        switch (*p) {
            case ']':
                bracket = p;
                break;
            case ':':
                if (bracket == nsnull)
                    colon = p;
                break;
            case ' ':
                
                NS_WARNING("malformed hostname");
                return NS_ERROR_MALFORMED_URI;
        }
    }

    if (colon) {
        
        SET_RESULT(hostname, 0, colon - serverinfo);
        if (port) {
            
            nsCAutoString buf(colon+1, serverinfoLen - (colon + 1 - serverinfo));
            if (buf.Length() == 0) {
                *port = -1;
            }
            else {
                PRInt32 err;
                *port = buf.ToInteger(&err);
                if (NS_FAILED(err))
                    return NS_ERROR_MALFORMED_URI;
            }
        }
    }
    else {
        
        SET_RESULT(hostname, 0, serverinfoLen);
        if (port)
           *port = -1;
    }
    return NS_OK;
}

void
nsAuthURLParser::ParseAfterScheme(const char *spec, PRInt32 specLen,
                                  PRUint32 *authPos, PRInt32 *authLen,
                                  PRUint32 *pathPos, PRInt32 *pathLen)
{
    NS_PRECONDITION(specLen >= 0, "unexpected");

    PRUint32 nslash = CountConsecutiveSlashes(spec, specLen);

    
    const char *end = spec + specLen;
    const char *p;
    for (p = spec + nslash; p < end; ++p) {
        if (*p == '/' || *p == '?' || *p == '#' || *p == ';')
            break;
    }
    if (p < end) {
        
        SET_RESULT(auth, nslash, p - (spec + nslash));
        SET_RESULT(path, p - spec, specLen - (p - spec));
    }
    else {
        
        SET_RESULT(auth, nslash, specLen - nslash);
        SET_RESULT(path, 0, -1);
    }
}





void
nsStdURLParser::ParseAfterScheme(const char *spec, PRInt32 specLen,
                                 PRUint32 *authPos, PRInt32 *authLen,
                                 PRUint32 *pathPos, PRInt32 *pathLen)
{
    NS_PRECONDITION(specLen >= 0, "unexpected");

    PRUint32 nslash = CountConsecutiveSlashes(spec, specLen);

    
    const char *end = spec + specLen;
    const char *p;
    for (p = spec + nslash; p < end; ++p) {
        if (strchr("/?#;", *p))
            break;
    }
    switch (nslash) {
    case 0:
    case 2:
        if (p < end) {
            
            SET_RESULT(auth, nslash, p - (spec + nslash));
            SET_RESULT(path, p - spec, specLen - (p - spec));
        }
        else {
            
            SET_RESULT(auth, nslash, specLen - nslash);
            SET_RESULT(path, 0, -1);
        }
        break;
    case 1:
        
        SET_RESULT(auth, 0, -1);
        SET_RESULT(path, 0, specLen);
        break;
    default:
        
        SET_RESULT(auth, 2, 0);
        SET_RESULT(path, 2, specLen - 2);
    }
}
