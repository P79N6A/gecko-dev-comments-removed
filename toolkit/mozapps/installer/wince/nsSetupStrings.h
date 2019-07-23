











































#pragma once

#define STR_KEY(x) StrID_##x,
enum
{
  #include "nsSetupStrings.inc"
  StrID_NumberOfStrings
};
#undef STR_KEY

class nsSetupStrings
{
public:
  nsSetupStrings();
  ~nsSetupStrings();

  BOOL LoadStrings(TCHAR *sFileName);
  const TCHAR* GetString(int nID);

private:

  static const int kNumberOfStrings = StrID_NumberOfStrings;
  TCHAR* m_sBuf;
  TCHAR* m_arrStrings[kNumberOfStrings];
};
