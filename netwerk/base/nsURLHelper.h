




#ifndef nsURLHelper_h__
#define nsURLHelper_h__

#include "nsString.h"

class nsIFile;
class nsIURLParser;

enum netCoalesceFlags
{
  NET_COALESCE_NORMAL = 0,

  




  NET_COALESCE_ALLOW_RELATIVE_ROOT = 1<<0,

  



  NET_COALESCE_DOUBLE_SLASH_IS_ROOT = 1<<1
};






void net_ShutdownURLHelper();
#ifdef XP_MACOSX
void net_ShutdownURLHelperOSX();
#endif


nsIURLParser * net_GetAuthURLParser();
nsIURLParser * net_GetNoAuthURLParser();
nsIURLParser * net_GetStdURLParser();





nsresult net_GetURLSpecFromFile(nsIFile *, nsACString &);
nsresult net_GetURLSpecFromDir(nsIFile *, nsACString &);
nsresult net_GetURLSpecFromActualFile(nsIFile *, nsACString &);
nsresult net_GetFileFromURLSpec(const nsACString &, nsIFile **);


nsresult net_ParseFileURL(const nsACString &inURL,
                                      nsACString &outDirectory,
                                      nsACString &outFileBaseName,
                                      nsACString &outFileExtension);


void net_CoalesceDirs(netCoalesceFlags flags, char* path);














nsresult net_ResolveRelativePath(const nsACString &relativePath,
                                             const nsACString &basePath,
                                             nsACString &result);









nsresult net_ExtractURLScheme(const nsACString &inURI,
                                          uint32_t *startPos, 
                                          uint32_t *endPos,
                                          nsACString *scheme = nullptr);


bool net_IsValidScheme(const char *scheme, uint32_t schemeLen);

inline bool net_IsValidScheme(const nsAFlatCString &scheme)
{
    return net_IsValidScheme(scheme.get(), scheme.Length());
}

















bool net_FilterURIString(const char *str, nsACString& result);

#if defined(XP_WIN)













bool net_NormalizeFileURL(const nsACString &aURL,
                                        nsCString &aResultBuf);
#endif






void net_ToLowerCase(char* str, uint32_t length);
void net_ToLowerCase(char* str);






char * net_FindCharInSet(const char *str, const char *end, const char *set);







char * net_FindCharNotInSet(const char *str, const char *end, const char *set);





char * net_RFindCharNotInSet(const char *str, const char *end, const char *set);









void net_ParseContentType(const nsACString &aHeaderStr,
                                      nsACString       &aContentType,
                                      nsACString       &aContentCharset,
                                      bool*          aHadCharset);









void net_ParseContentType(const nsACString &aHeaderStr,
                                      nsACString       &aContentType,
                                      nsACString       &aContentCharset,
                                      bool             *aHadCharset,
                                      int32_t          *aCharsetStart,
                                      int32_t          *aCharsetEnd);




#define NET_MAX_ADDRESS (((char*)0)-1)

inline char *net_FindCharInSet(const char *str, const char *set)
{
    return net_FindCharInSet(str, NET_MAX_ADDRESS, set);
}
inline char *net_FindCharNotInSet(const char *str, const char *set)
{
    return net_FindCharNotInSet(str, NET_MAX_ADDRESS, set);
}
inline char *net_RFindCharNotInSet(const char *str, const char *set)
{
    return net_RFindCharNotInSet(str, str + strlen(str), set);
}





bool net_IsValidHostName(const nsCSubstring &host);




bool net_IsValidIPv4Addr(const char *addr, int32_t addrLen);




bool net_IsValidIPv6Addr(const char *addr, int32_t addrLen);

#endif 
