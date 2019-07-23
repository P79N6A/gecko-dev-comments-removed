















#ifndef _CONVFUNC_
#define _CONVFUNC_ 

int xatoi(char *str);
int xatoiW(wchar_t *wstr);
char* xitoa(int number, char *str, int width);
wchar_t* xitoaW(int number, wchar_t *wstr, int width);
unsigned int xatoui(char *str);
unsigned int xatouiW(wchar_t *wstr);
char* xuitoa(unsigned int number, char *str, int width);
wchar_t* xuitoaW(unsigned int number, wchar_t *wstr, int width);
__int64 xatoi64(char *str);
__int64 xatoi64W(wchar_t *wstr);
char* xi64toa(__int64 number, char *str, int width);
wchar_t* xi64toaW(__int64 number, wchar_t *wstr, int width);
int hex2dec(char *hex);
int hex2decW(wchar_t *whex);
void dec2hex(unsigned int dec, char *hex, BOOL lowercase, unsigned int width);
void dec2hexW(unsigned int dec, wchar_t *whex, BOOL lowercase, unsigned int width);

void str2hex(unsigned char *str, char *hex, BOOL lowercase, unsigned int bytes);
void hex2str(char *hex, char *str);

#endif















#if defined xatoi || defined ALLCONVFUNC
#define xatoi_INCLUDED
#undef xatoi
int xatoi(char *str)
{
  int nNumber=0;
  BOOL bMinus=FALSE;

  while (*str == ' ')
    ++str;
  if (*str == '+')
    ++str;
  else if (*str == '-')
  {
    bMinus=TRUE;
    ++str;
  }
  for (; *str != '\0' && *str >= '0' && *str <= '9'; ++str)
    nNumber=(nNumber * 10) + (*str - '0');
  if (bMinus == TRUE)
    nNumber=0 - nNumber;
  return nNumber;
}
#endif















#if defined xatoiW || defined ALLCONVFUNC
#define xatoiW_INCLUDED
#undef xatoiW
int xatoiW(wchar_t *wstr)
{
  int nNumber=0;
  BOOL bMinus=FALSE;

  while (*wstr == ' ')
    ++wstr;
  if (*wstr == '+')
    ++wstr;
  else if (*wstr == '-')
  {
    bMinus=TRUE;
    ++wstr;
  }
  for (; *wstr != '\0' && *wstr >= '0' && *wstr <= '9'; ++wstr)
    nNumber=(nNumber * 10) + (*wstr - '0');
  if (bMinus == TRUE)
    nNumber=0 - nNumber;
  return nNumber;
}
#endif


















#if defined xitoa || defined ALLCONVFUNC
#define xitoa_INCLUDED
#undef xitoa
char* xitoa(int number, char *str, int width)
{
  char tmp[128]="";
  int a=0;
  int b=0;

  if (number == 0)
  {
    str[0]='0';
    --width;
    b=1;
  }
  else if (number < 0)
  {
    str[0]='-';
    number=0 - number;
    --width;
    b=1;
  }
  for (tmp[a]='\0'; number != 0; ++a)
  {
    tmp[a]=(number % 10) + '0';
    number=number / 10;
  }
  for (; width > a; ++a) tmp[a]='0';
  for (--a; a >= 0; --a, ++b) str[b]=tmp[a];

  str[b]='\0';
  return str;
}
#endif


















#if defined xitoaW || defined ALLCONVFUNC
#define xitoaW_INCLUDED
#undef xitoaW
wchar_t* xitoaW(int number, wchar_t *wstr, int width)
{
  wchar_t wtmp[128]=L"";
  int a=0;
  int b=0;

  if (number == 0)
  {
    wstr[0]='0';
    --width;
    b=1;
  }
  else if (number < 0)
  {
    wstr[0]='-';
    number=0 - number;
    --width;
    b=1;
  }
  for (wtmp[a]='\0'; number != 0; ++a)
  {
    wtmp[a]=(number % 10) + '0';
    number=number / 10;
  }
  for (; width > a; ++a) wtmp[a]='0';
  for (--a; a >= 0; --a, ++b) wstr[b]=wtmp[a];

  wstr[b]='\0';
  return wstr;
}
#endif















#if defined xatoui || defined ALLCONVFUNC
#define xatoui_INCLUDED
#undef xatoui
unsigned int xatoui(char *str)
{
  unsigned int nNumber=0;

  while (*str == ' ')
    ++str;
  if (*str == '+')
    ++str;
  else if (*str == '-')
    return 0;
  for (; *str != '\0' && *str >= '0' && *str <= '9'; ++str)
    nNumber=(nNumber * 10) + (*str - '0');
  return nNumber;
}
#endif















#if defined xatouiW || defined ALLCONVFUNC
#define xatouiW_INCLUDED
#undef xatouiW
unsigned int xatouiW(wchar_t *wstr)
{
  unsigned int nNumber=0;

  while (*wstr == ' ')
    ++wstr;
  if (*wstr == '+')
    ++wstr;
  else if (*wstr == '-')
    return 0;
  for (; *wstr != '\0' && *wstr >= '0' && *wstr <= '9'; ++wstr)
    nNumber=(nNumber * 10) + (*wstr - '0');
  return nNumber;
}
#endif

















#if defined xuitoa || defined ALLCONVFUNC
#define xuitoa_INCLUDED
#undef xuitoa
char* xuitoa(unsigned int number, char *str, int width)
{
  char tmp[128]="";
  int a=0;
  int b=0;

  if (number == 0)
  {
    str[0]='0';
    --width;
    b=1;
  }
  for (tmp[a]='\0'; number != 0; ++a)
  {
    tmp[a]=(number % 10) + '0';
    number=number / 10;
  }
  for (; width > a; ++a) tmp[a]='0';
  for (--a; a >= 0; --a, ++b) str[b]=tmp[a];

  str[b]='\0';
  return str;
}
#endif

















#if defined xuitoaW || defined ALLCONVFUNC
#define xuitoaW_INCLUDED
#undef xuitoaW
wchar_t* xuitoaW(unsigned int number, wchar_t *wstr, int width)
{
  wchar_t wtmp[128]=L"";
  int a=0;
  int b=0;

  if (number == 0)
  {
    wstr[0]='0';
    --width;
    b=1;
  }
  for (wtmp[a]='\0'; number != 0; ++a)
  {
    wtmp[a]=(number % 10) + '0';
    number=number / 10;
  }
  for (; width > a; ++a) wtmp[a]='0';
  for (--a; a >= 0; --a, ++b) wstr[b]=wtmp[a];

  wstr[b]='\0';
  return wstr;
}
#endif















#if defined xatoi64 || defined ALLCONVFUNC
#define xatoi64_INCLUDED
#undef xatoi64
__int64 xatoi64(char *str)
{
  __int64 nNumber=0;
  BOOL bMinus=FALSE;

  while (*str == ' ')
    ++str;
  if (*str == '+')
    ++str;
  else if (*str == '-')
  {
    bMinus=TRUE;
    ++str;
  }
  for (; *str != '\0' && *str >= '0' && *str <= '9'; ++str)
    nNumber=(nNumber * 10) + (*str - '0');
  if (bMinus == TRUE)
    nNumber=0 - nNumber;
  return nNumber;
}
#endif















#if defined xatoi64W || defined ALLCONVFUNC
#define xatoi64W_INCLUDED
#undef xatoi64W
__int64 xatoi64W(wchar_t *wstr)
{
  __int64 nNumber=0;
  BOOL bMinus=FALSE;

  while (*wstr == ' ')
    ++wstr;
  if (*wstr == '+')
    ++wstr;
  else if (*wstr == '-')
  {
    bMinus=TRUE;
    ++wstr;
  }
  for (; *wstr != '\0' && *wstr >= '0' && *wstr <= '9'; ++wstr)
    nNumber=(nNumber * 10) + (*wstr - '0');
  if (bMinus == TRUE)
    nNumber=0 - nNumber;
  return nNumber;
}
#endif


















#if defined xi64toa || defined ALLCONVFUNC
#define xi64toa_INCLUDED
#undef xi64toa
char* xi64toa(__int64 number, char *str, int width)
{
  char tmp[128]="";
  int a=0;
  int b=0;

  if (number == 0)
  {
    str[0]='0';
    --width;
    b=1;
  }
  else if (number < 0)
  {
    str[0]='-';
    number=0 - number;
    --width;
    b=1;
  }
  for (tmp[a]='\0'; number != 0; ++a)
  {
    tmp[a]=(char)((number % 10) + '0');
    number=number / 10;
  }
  for (; width > a; ++a) tmp[a]='0';
  for (--a; a >= 0; --a, ++b) str[b]=tmp[a];

  str[b]='\0';
  return str;
}
#endif


















#if defined xi64toaW || defined ALLCONVFUNC
#define xi64toaW_INCLUDED
#undef xi64toaW
wchar_t* xi64toaW(__int64 number, wchar_t *wstr, int width)
{
  wchar_t wtmp[128]=L"";
  int a=0;
  int b=0;

  if (number == 0)
  {
    wstr[0]='0';
    --width;
    b=1;
  }
  else if (number < 0)
  {
    wstr[0]='-';
    number=0 - number;
    --width;
    b=1;
  }
  for (wtmp[a]='\0'; number != 0; ++a)
  {
    wtmp[a]=(char)((number % 10) + '0');
    number=number / 10;
  }
  for (; width > a; ++a) wtmp[a]='0';
  for (--a; a >= 0; --a, ++b) wstr[b]=wtmp[a];

  wstr[b]='\0';
  return wstr;
}
#endif















#if defined hex2dec || defined ALLCONVFUNC
#define hex2dec_INCLUDED
#undef hex2dec
int hex2dec(char *hex)
{
  int a;
  int b=0;

  while (1)
  {
    a=*hex++;
    if (a >= '0' && a <= '9') a-='0';
    else if (a >= 'a' && a <= 'f') a-='a'-10;
    else if (a >= 'A' && a <= 'F') a-='A'-10;
    else return -1;

    if (*hex) b=(b + a) * 16;
    else return (b + a);
  }
}
#endif















#if defined hex2decW || defined ALLCONVFUNC
#define hex2decW_INCLUDED
#undef hex2decW
int hex2decW(wchar_t *whex)
{
  int a;
  int b=0;

  while (1)
  {
    a=*whex++;
    if (a >= '0' && a <= '9') a-='0';
    else if (a >= 'a' && a <= 'f') a-='a'-10;
    else if (a >= 'A' && a <= 'F') a-='A'-10;
    else return -1;

    if (*whex) b=(b + a) * 16;
    else return (b + a);
  }
}
#endif

















#if defined dec2hex || defined ALLCONVFUNC
#define dec2hex_INCLUDED
#undef dec2hex
void dec2hex(unsigned int dec, char *hex, BOOL lowercase, unsigned int width)
{
  unsigned int a=dec;
  unsigned int b=0;
  unsigned int c=0;
  char d='1';
  if (a == 0) d='0';

  while (a)
  {
    b=a % 16;
    a=a / 16;
    if (b < 10) hex[c++]=b + '0';
    else if (lowercase == TRUE) hex[c++]=b + 'a' - 10;
    else hex[c++]=b + 'A' - 10;
  }
  while (width > c) hex[c++]='0';
  hex[c]='\0';

  if (d == '1')
    for (b=0, --c; b < c; d=hex[b], hex[b++]=hex[c], hex[c--]=d);
}
#endif

















#if defined dec2hexW || defined ALLCONVFUNC
#define dec2hexW_INCLUDED
#undef dec2hexW
void dec2hexW(unsigned int dec, wchar_t *whex, BOOL lowercase, unsigned int width)
{
  unsigned int a=dec;
  unsigned int b=0;
  unsigned int c=0;
  wchar_t d='1';
  if (a == 0) d='0';

  while (a)
  {
    b=a % 16;
    a=a / 16;
    if (b < 10) whex[c++]=b + '0';
    else if (lowercase == TRUE) whex[c++]=b + 'a' - 10;
    else whex[c++]=b + 'A' - 10;
  }
  while (width > c) whex[c++]='0';
  whex[c]='\0';

  if (d == '1')
    for (b=0, --c; b < c; d=whex[b], whex[b++]=whex[c], whex[c--]=d);
}
#endif



















#if defined str2hex || defined ALLCONVFUNCS
#define str2hex_INCLUDED
#undef str2hex
void str2hex(unsigned char *str, char *hex, BOOL lowercase, unsigned int bytes)
{
  char a[16];
  unsigned int b=0;

  for (hex[0]='\0'; b < bytes; ++b)
  {
    
    dec2hex((unsigned int)str[b], a, lowercase, 2);
    lstrcat(hex, a);
  }
}
#endif













#if defined hex2str || defined ALLCONVFUNCS
#define hex2str_INCLUDED
#undef hex2str
void hex2str(char *hex, char *str)
{
  char a[4];
  int b;

  while (*hex)
  {
    a[0]=*hex;
    a[1]=*++hex;
    a[2]='\0';

    if (*hex++)
    {
      if ((b=hex2dec(a)) > 0) *str++=b;
      else break;
            }
    else break;
  }
  *str='\0';
}
#endif































