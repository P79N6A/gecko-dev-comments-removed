
















































































#ifndef mozilla_JSONWriter_h
#define mozilla_JSONWriter_h

#include "mozilla/double-conversion.h"
#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/PodOperations.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/Vector.h"

#include <stdio.h>

namespace mozilla {




class JSONWriteFunc
{
public:
  virtual void Write(const char* aStr) = 0;
  virtual ~JSONWriteFunc() {}
};



namespace detail {
extern MFBT_DATA const char gTwoCharEscapes[256];
}

class JSONWriter
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  class EscapedString
  {
    
    
    
    bool mIsOwned;
    const char* mUnownedStr;
    UniquePtr<char[]> mOwnedStr;

    void SanityCheck() const
    {
      MOZ_ASSERT_IF( mIsOwned,  mOwnedStr.get() && !mUnownedStr);
      MOZ_ASSERT_IF(!mIsOwned, !mOwnedStr.get() &&  mUnownedStr);
    }

    static char hexDigitToAsciiChar(uint8_t u)
    {
      u = u & 0xf;
      return u < 10 ? '0' + u : 'a' + (u - 10);
    }

  public:
    explicit EscapedString(const char* aStr)
      : mUnownedStr(nullptr)
      , mOwnedStr(nullptr)
    {
      const char* p;

      
      size_t nExtra = 0;
      p = aStr;
      while (true) {
        uint8_t u = *p;   
        if (u == 0) {
          break;
        }
        if (detail::gTwoCharEscapes[u]) {
          nExtra += 1;
        } else if (u <= 0x1f) {
          nExtra += 5;
        }
        p++;
      }

      if (nExtra == 0) {
        
        mIsOwned = false;
        mUnownedStr = aStr;
        return;
      }

      
      mIsOwned = true;
      size_t len = (p - aStr) + nExtra;
      mOwnedStr = MakeUnique<char[]>(len + 1);

      p = aStr;
      size_t i = 0;

      while (true) {
        uint8_t u = *p;   
        if (u == 0) {
          mOwnedStr[i] = 0;
          break;
        }
        if (detail::gTwoCharEscapes[u]) {
          mOwnedStr[i++] = '\\';
          mOwnedStr[i++] = detail::gTwoCharEscapes[u];
        } else if (u <= 0x1f) {
          mOwnedStr[i++] = '\\';
          mOwnedStr[i++] = 'u';
          mOwnedStr[i++] = '0';
          mOwnedStr[i++] = '0';
          mOwnedStr[i++] = hexDigitToAsciiChar((u & 0x00f0) >> 4);
          mOwnedStr[i++] = hexDigitToAsciiChar(u & 0x000f);
        } else {
          mOwnedStr[i++] = u;
        }
        p++;
      }
    }

    ~EscapedString()
    {
      SanityCheck();
    }

    const char* get() const
    {
      SanityCheck();
      return mIsOwned ? mOwnedStr.get() : mUnownedStr;
    }
  };

  const UniquePtr<JSONWriteFunc> mWriter;
  Vector<bool, 8> mNeedComma;     
  size_t mDepth;                  

  void Indent()
  {
    for (size_t i = 0; i < mDepth; i++) {
      mWriter->Write(" ");
    }
  }

  
  
  
  void Separator()
  {
    if (mNeedComma[mDepth]) {
      mWriter->Write(",");
    }
    if (mDepth > 0) {
      mWriter->Write("\n");
    }
    Indent();
  }

  void PropertyNameAndColon(const char* aName)
  {
    EscapedString escapedName(aName);
    mWriter->Write("\"");
    mWriter->Write(escapedName.get());
    mWriter->Write("\": ");
  }

  void Scalar(const char* aMaybePropertyName, const char* aStringValue)
  {
    Separator();
    if (aMaybePropertyName) {
      PropertyNameAndColon(aMaybePropertyName);
    }
    mWriter->Write(aStringValue);
    mNeedComma[mDepth] = true;
  }

  void QuotedScalar(const char* aMaybePropertyName, const char* aStringValue)
  {
    Separator();
    if (aMaybePropertyName) {
      PropertyNameAndColon(aMaybePropertyName);
    }
    mWriter->Write("\"");
    mWriter->Write(aStringValue);
    mWriter->Write("\"");
    mNeedComma[mDepth] = true;
  }

  void NewCommaEntry()
  {
    
    
    MOZ_RELEASE_ASSERT(mNeedComma.growByUninitialized(1));
    mNeedComma[mDepth] = false;
  }

  void StartCollection(const char* aMaybePropertyName, const char* aStartChar)
  {
    Separator();
    if (aMaybePropertyName) {
      mWriter->Write("\"");
      mWriter->Write(aMaybePropertyName);
      mWriter->Write("\": ");
    }
    mWriter->Write(aStartChar);
    mNeedComma[mDepth] = true;
    mDepth++;
    NewCommaEntry();
  }

  
  void EndCollection(const char* aEndChar)
  {
    mDepth--;
    mWriter->Write("\n");
    Indent();
    mWriter->Write(aEndChar);
  }

public:
  explicit JSONWriter(UniquePtr<JSONWriteFunc> aWriter)
    : mWriter(Move(aWriter))
    , mNeedComma()
    , mDepth(0)
  {
    NewCommaEntry();
  }

  
  
  JSONWriteFunc* WriteFunc() const { return mWriter.get(); }

  
  
  
  
  

  
  void Start() { StartCollection(nullptr, "{"); }

  
  void End() { EndCollection("}\n"); }

  
  void NullProperty(const char* aName)
  {
    Scalar(aName, "null");
  }

  
  void NullElement() { NullProperty(nullptr); }

  
  void BoolProperty(const char* aName, bool aBool)
  {
    Scalar(aName, aBool ? "true" : "false");
  }

  
  void BoolElement(bool aBool) { BoolProperty(nullptr, aBool); }

  
  void IntProperty(const char* aName, int64_t aInt)
  {
    char buf[64];
    sprintf(buf, "%" PRId64, aInt);
    Scalar(aName, buf);
  }

  
  void IntElement(int64_t aInt) { IntProperty(nullptr, aInt); }

  
  void DoubleProperty(const char* aName, double aDouble)
  {
    static const size_t buflen = 64;
    char buf[buflen];
    const double_conversion::DoubleToStringConverter &converter =
      double_conversion::DoubleToStringConverter::EcmaScriptConverter();
    double_conversion::StringBuilder builder(buf, buflen);
    converter.ToShortest(aDouble, &builder);
    Scalar(aName, builder.Finalize());
  }

  
  void DoubleElement(double aDouble) { DoubleProperty(nullptr, aDouble); }

  
  void StringProperty(const char* aName, const char* aStr)
  {
    EscapedString escapedStr(aStr);
    QuotedScalar(aName, escapedStr.get());
  }

  
  void StringElement(const char* aStr) { StringProperty(nullptr, aStr); }

  
  
  void PointerProperty(const char* aName, const void* aPtr)
  {
    char buf[32];
    sprintf(buf, "0x%" PRIxPTR, uintptr_t(aPtr));
    QuotedScalar(aName, buf);
  }

  
  
  void PointerElement(const void* aPtr) { PointerProperty(nullptr, aPtr); }

  
  void StartArrayProperty(const char* aName) { StartCollection(aName, "["); }

  
  void StartArrayElement() { StartArrayProperty(nullptr); }

  
  void EndArray() { EndCollection("]"); }

  
  void StartObjectProperty(const char* aName) { StartCollection(aName, "{"); }

  
  void StartObjectElement() { StartObjectProperty(nullptr); }

  
  void EndObject() { EndCollection("}"); }
};

} 

#endif 

