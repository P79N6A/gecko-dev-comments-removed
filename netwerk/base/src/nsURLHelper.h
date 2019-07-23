




































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






NS_HIDDEN_(void) net_ShutdownURLHelper();
#ifdef XP_MACOSX
NS_HIDDEN_(void) net_ShutdownURLHelperOSX();
#endif


NS_HIDDEN_(nsIURLParser *) net_GetAuthURLParser();
NS_HIDDEN_(nsIURLParser *) net_GetNoAuthURLParser();
NS_HIDDEN_(nsIURLParser *) net_GetStdURLParser();


NS_HIDDEN_(nsresult) net_GetURLSpecFromFile(nsIFile *, nsACString &);
NS_HIDDEN_(nsresult) net_GetURLSpecFromDir(nsIFile *, nsACString &);
NS_HIDDEN_(nsresult) net_GetURLSpecFromActualFile(nsIFile *, nsACString &);
NS_HIDDEN_(nsresult) net_GetFileFromURLSpec(const nsACString &, nsIFile **);


NS_HIDDEN_(nsresult) net_ParseFileURL(const nsACString &inURL,
                                      nsACString &outDirectory,
                                      nsACString &outFileBaseName,
                                      nsACString &outFileExtension);


NS_HIDDEN_(void) net_CoalesceDirs(netCoalesceFlags flags, char* path);














NS_HIDDEN_(nsresult) net_ResolveRelativePath(const nsACString &relativePath,
                                             const nsACString &basePath,
                                             nsACString &result);









NS_HIDDEN_(nsresult) net_ExtractURLScheme(const nsACString &inURI,
                                          PRUint32 *startPos, 
                                          PRUint32 *endPos,
                                          nsACString *scheme = nsnull);


NS_HIDDEN_(PRBool) net_IsValidScheme(const char *scheme, PRUint32 schemeLen);

inline PRBool net_IsValidScheme(const nsAFlatCString &scheme)
{
    return net_IsValidScheme(scheme.get(), scheme.Length());
}
















NS_HIDDEN_(PRBool) net_FilterURIString(const char *str, nsACString& result);

#if defined(XP_WIN) || defined(XP_OS2)













NS_HIDDEN_(PRBool) net_NormalizeFileURL(const nsACString &aURL,
                                        nsCString &aResultBuf);
#endif






NS_HIDDEN_(void) net_ToLowerCase(char* str, PRUint32 length);
NS_HIDDEN_(void) net_ToLowerCase(char* str);






NS_HIDDEN_(char *) net_FindCharInSet(const char *str, const char *end, const char *set);







NS_HIDDEN_(char *) net_FindCharNotInSet(const char *str, const char *end, const char *set);





NS_HIDDEN_(char *) net_RFindCharNotInSet(const char *str, const char *end, const char *set);









NS_HIDDEN_(void) net_ParseContentType(const nsACString &aHeaderStr,
                                      nsACString       &aContentType,
                                      nsACString       &aContentCharset,
                                      PRBool*          aHadCharset);









NS_HIDDEN_(void) net_ParseContentType(const nsACString &aHeaderStr,
                                      nsACString       &aContentType,
                                      nsACString       &aContentCharset,
                                      PRBool           *aHadCharset,
                                      PRInt32          *aCharsetStart,
                                      PRInt32          *aCharsetEnd);




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





NS_HIDDEN_(PRBool) net_IsValidHostName(const nsCSubstring &host);

#endif 
