














#ifndef _CONVFUNC_
#define _CONVFUNC_ 
















#ifdef xatoi
#define xatoi_INCLUDED
#undef xatoi
int xatoi(TCHAR *str)
{
	BOOL bMinus=FALSE;
	int nNumber=0;

	while (*str == TEXT(' '))
		++str;
	if (*str == TEXT('-'))
	{
		bMinus=TRUE;
		++str;
	}
	for (; (*str != TEXT('\0')) && (*str >= TEXT('0')) && (*str <= TEXT('9')); ++str)
		nNumber=(nNumber * 10) + (*str - 48);
	if (bMinus == TRUE)
		nNumber=0 - nNumber;
	return nNumber;
}
#endif














#ifdef xitoa
#define xitoa_INCLUDED
#undef xitoa
void xitoa(char *str, int number)
{
	int nCapacity=0;
	int nTmp=number;

	if (number == 0)
	{
		str[0]='0';
		str[1]='\0';
		return;
	}
	if (number < 0)
	{
		str[0]='-';
		number=0 - number;
		++nCapacity;
	}
	for (; (nTmp != 0); ++nCapacity)
		nTmp=nTmp/10;
	for (str[nCapacity--]='\0'; (number != 0); --nCapacity)
	{
		str[nCapacity]=(number % 10) + 48;
		number=number/10;
	}
}
#endif














#if defined hex2dec || defined hex2str
#define hex2dec_INCLUDED
#undef hex2dec
unsigned hex2dec(char *hex)
{
	int a;
	int b=0;

	while (1)
	{
		a=*hex++;
		if (a >= '0' && a <= '9') a-='0';
		else if (a >= 'a' && a <= 'f') a-='a'-10;
		else if (a >= 'A' && a <= 'F') a-='A'-10;
		else return b;

		if (*hex) b=(b + a) * 16;
		else return (b + a);
	}
}
#endif

















#if defined dec2hex || defined str2hex
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
















#if defined str2hex && defined dec2hex_INCLUDED
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













#if defined hex2str && defined hex2dec_INCLUDED
#define hex2str_INCLUDED
#undef hex2str
void hex2str(char *hex, char *str)
{
	char a[4];

	while (*hex)
	{
		a[0]=*hex;
		a[1]=*++hex;
		a[2]='\0';

		if (*hex++) *str++=hex2dec(a);
		else break;
	}
	*str='\0';
}
#endif

#endif 