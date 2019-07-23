




































#ifndef nsNativeCharsetUtils_h__
#define nsNativeCharsetUtils_h__

















NS_COM nsresult NS_CopyNativeToUnicode(const nsACString &input, nsAString  &output);
NS_COM nsresult NS_CopyUnicodeToNative(const nsAString  &input, nsACString &output);














#if defined(XP_UNIX) && !defined(XP_MACOSX)
NS_COM PRBool NS_IsNativeUTF8();
#else
inline PRBool NS_IsNativeUTF8()
{
#if defined(XP_MACOSX) || defined(XP_BEOS)
    return PR_TRUE;
#else
    return PR_FALSE;
#endif
}
#endif





void NS_StartupNativeCharsetUtils();
void NS_ShutdownNativeCharsetUtils();

#endif 
