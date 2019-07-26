





#ifndef nsString_h___
#define nsString_h___

#include "mozilla/Attributes.h"

#include "nsSubstring.h"
#include "nsDependentSubstring.h"
#include "nsReadableUtils.h"

#include <new>


#ifndef MOZ_STRING_WITH_OBSOLETE_API
#define MOZ_STRING_WITH_OBSOLETE_API 1
#endif

#if MOZ_STRING_WITH_OBSOLETE_API

#define kRadix10        (10)
#define kRadix16        (16)
#define kAutoDetect     (100)
#define kRadixUnknown   (kAutoDetect+1)
#define IGNORE_CASE     (true)
#endif



#include "string-template-def-unichar.h"
#include "nsTString.h"
#include "string-template-undef.h"


#include "string-template-def-char.h"
#include "nsTString.h"
#include "string-template-undef.h"

static_assert(sizeof(char16_t) == 2, "size of char16_t must be 2");
static_assert(sizeof(nsString::char_type) == 2,
              "size of nsString::char_type must be 2");
static_assert(nsString::char_type(-1) > nsString::char_type(0),
              "nsString::char_type must be unsigned");
static_assert(sizeof(nsCString::char_type) == 1,
              "size of nsCString::char_type must be 1");





class NS_LossyConvertUTF16toASCII : public nsAutoCString
{
public:
  explicit NS_LossyConvertUTF16toASCII(const char16_t* aString)
  {
    LossyAppendUTF16toASCII(aString, *this);
  }

  NS_LossyConvertUTF16toASCII(const char16_t* aString, uint32_t aLength)
  {
    LossyAppendUTF16toASCII(Substring(aString, aLength), *this);
  }

#ifdef MOZ_USE_CHAR16_WRAPPER
  explicit NS_LossyConvertUTF16toASCII(char16ptr_t aString)
    : NS_LossyConvertUTF16toASCII(static_cast<const char16_t*>(aString))
  {
  }

  NS_LossyConvertUTF16toASCII(char16ptr_t aString, uint32_t aLength)
    : NS_LossyConvertUTF16toASCII(static_cast<const char16_t*>(aString), aLength)
  {
  }
#endif

  explicit NS_LossyConvertUTF16toASCII(const nsAString& aString)
  {
    LossyAppendUTF16toASCII(aString, *this);
  }

private:
  
  NS_LossyConvertUTF16toASCII(char) MOZ_DELETE;
};


class NS_ConvertASCIItoUTF16 : public nsAutoString
{
public:
  explicit NS_ConvertASCIItoUTF16(const char* aCString)
  {
    AppendASCIItoUTF16(aCString, *this);
  }

  NS_ConvertASCIItoUTF16(const char* aCString, uint32_t aLength)
  {
    AppendASCIItoUTF16(Substring(aCString, aLength), *this);
  }

  explicit NS_ConvertASCIItoUTF16(const nsACString& aCString)
  {
    AppendASCIItoUTF16(aCString, *this);
  }

private:
  
  NS_ConvertASCIItoUTF16(char16_t) MOZ_DELETE;
};





class NS_ConvertUTF16toUTF8 : public nsAutoCString
{
public:
  explicit NS_ConvertUTF16toUTF8(const char16_t* aString)
  {
    AppendUTF16toUTF8(aString, *this);
  }

  NS_ConvertUTF16toUTF8(const char16_t* aString, uint32_t aLength)
  {
    AppendUTF16toUTF8(Substring(aString, aLength), *this);
  }

#ifdef MOZ_USE_CHAR16_WRAPPER
  NS_ConvertUTF16toUTF8(char16ptr_t aString)
    : NS_ConvertUTF16toUTF8(static_cast<const char16_t*>(aString))
  {
  }

  NS_ConvertUTF16toUTF8(char16ptr_t aString, uint32_t aLength)
    : NS_ConvertUTF16toUTF8(static_cast<const char16_t*>(aString), aLength)
  {
  }
#endif

  explicit NS_ConvertUTF16toUTF8(const nsAString& aString)
  {
    AppendUTF16toUTF8(aString, *this);
  }

private:
  
  NS_ConvertUTF16toUTF8(char) MOZ_DELETE;
};


class NS_ConvertUTF8toUTF16 : public nsAutoString
{
public:
  explicit NS_ConvertUTF8toUTF16(const char* aCString)
  {
    AppendUTF8toUTF16(aCString, *this);
  }

  NS_ConvertUTF8toUTF16(const char* aCString, uint32_t aLength)
  {
    AppendUTF8toUTF16(Substring(aCString, aLength), *this);
  }

  explicit NS_ConvertUTF8toUTF16(const nsACString& aCString)
  {
    AppendUTF8toUTF16(aCString, *this);
  }

private:
  
  NS_ConvertUTF8toUTF16(char16_t) MOZ_DELETE;
};


#ifdef MOZ_USE_CHAR16_WRAPPER

inline char16_t*
wwc(wchar_t* aStr)
{
  return reinterpret_cast<char16_t*>(aStr);
}

inline wchar_t*
wwc(char16_t* aStr)
{
  return reinterpret_cast<wchar_t*>(aStr);
}

#else

inline char16_t*
wwc(char16_t* aStr)
{
  return aStr;
}

#endif


typedef nsAutoString nsVoidableString;

#include "nsDependentString.h"
#include "nsLiteralString.h"
#include "nsPromiseFlatString.h"


#include "nsMemory.h"
#include <string.h>
#include <stdio.h>
#include "plhash.h"

#endif 
