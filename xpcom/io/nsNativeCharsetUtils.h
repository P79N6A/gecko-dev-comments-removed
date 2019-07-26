





#ifndef nsNativeCharsetUtils_h__
#define nsNativeCharsetUtils_h__

















nsresult NS_CopyNativeToUnicode(const nsACString& aInput, nsAString& aOutput);
nsresult NS_CopyUnicodeToNative(const nsAString& aInput, nsACString& aOutput);














#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(ANDROID)
bool NS_IsNativeUTF8();
#else
inline bool
NS_IsNativeUTF8()
{
#if defined(XP_MACOSX) || defined(ANDROID)
  return true;
#else
  return false;
#endif
}
#endif





void NS_StartupNativeCharsetUtils();
void NS_ShutdownNativeCharsetUtils();

#endif 
