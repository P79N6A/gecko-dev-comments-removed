











































#include <windows.h>
#include "nsSetupStrings.h"
#include "readstrings.h"
#include "errors.h"

#define STR_KEY(x) #x "\0"
const char *kSetupStrKeys =
#include "nsSetupStrings.inc"
;
#undef STR_KEY

nsSetupStrings::nsSetupStrings()
{
  m_sBuf = NULL;
  memset(m_arrStrings, 0, sizeof(m_arrStrings));
}

nsSetupStrings::~nsSetupStrings()
{
  delete[] m_sBuf;
}

BOOL nsSetupStrings::LoadStrings(WCHAR *sFileName)
{
  if (!sFileName)
    return FALSE;

  BOOL bResult = FALSE;
  char strings[kNumberOfStrings][MAX_TEXT_LEN];
  if (ReadStrings(sFileName, kSetupStrKeys, kNumberOfStrings, strings) == OK)
  {
    int nSize = 0;
    for (int i = 0; i < kNumberOfStrings; i++)
    {
      nSize += strlen(strings[i]) + 1;
    }

    m_sBuf = new WCHAR[nSize];
    if (!m_sBuf)
      return FALSE;

    WCHAR *p = m_sBuf;
    for (int i = 0; i < kNumberOfStrings; i++)
    {
      int len = strlen(strings[i]) + 1;
      MultiByteToWideChar(CP_UTF8, 0, strings[i], len, p, len);
      m_arrStrings[i] = p;
      p += len;
    }

    bResult = TRUE;
  }

  return bResult;
}

const WCHAR* nsSetupStrings::GetString(int nID)
{
  if (nID >= kNumberOfStrings)
    return L"";
  else
    return m_arrStrings[nID];
}
