




#include "JSCustomObjectBuilder.h"

#include "nsDataHashtable.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "nsUTF8Utils.h"

#if _MSC_VER
 #define snprintf _snprintf
#endif


struct PropertyValue {
  virtual ~PropertyValue() {}
  virtual void SendToStream(std::ostream& stream) = 0;
};

template <typename T>
struct finalizer_impl
{
  static void run(T) {}
};

template <typename T>
struct finalizer_impl<T*>
{
  static void run(T* p) {
    delete p;
  }
};

template <>
struct finalizer_impl<char *>
{
  static void run(char* p) {
    free(p);
  }
};

template <class T>
class TemplatePropertyValue : public PropertyValue {
public:
  TemplatePropertyValue(T aValue)
    : mValue(aValue)
  {}

  ~TemplatePropertyValue() {
    finalizer_impl<T>::run(mValue);
  }

  virtual void SendToStream(std::ostream& stream);
private:
  T mValue;
};



void EscapeToStream(std::ostream& stream, const char* str) {
  stream << "\"";

  size_t len = strlen(str);
  const char* end = &str[len];
  while (str < end) {
    bool err;
    const char* utf8CharStart = str;
    uint32_t ucs4Char = UTF8CharEnumerator::NextChar(&str, end, &err);

    if (err) {
      
      stream << "INVALID\"";
      return;
    }

    
    
    
    
    if (ucs4Char == '\"') {
      stream << "\\\"";
    } else if (ucs4Char == '\\') {
      stream << "\\\\";
    } else if (ucs4Char > 0xFF) {
      PRUnichar chr[2];
      ConvertUTF8toUTF16 encoder(chr);
      encoder.write(utf8CharStart, str-utf8CharStart);
      char escChar[13];
      snprintf(escChar, mozilla::ArrayLength(escChar), "\\u%04X\\u%04X", chr[0], chr[1]);
      stream << escChar;
    } else if (ucs4Char < 0x1F || ucs4Char > 0xFF) {
      char escChar[7];
      snprintf(escChar, mozilla::ArrayLength(escChar), "\\u%04X", ucs4Char);
      stream << escChar;
    } else {
      stream << char(ucs4Char);
    }
  }
  stream << "\"";
}

class JSCustomObject {
public:
  JSCustomObject() {
    mProperties.Init();
  }
  ~JSCustomObject();

  friend std::ostream& operator<<(std::ostream& stream, JSCustomObject* entry);

  template<class T>
  void AddProperty(const char* aName, T aValue) {
    mProperties.Put(nsDependentCString(aName), new TemplatePropertyValue<T>(aValue));
  }

  nsDataHashtable<nsCStringHashKey, PropertyValue*> mProperties;
};

class JSCustomArray {
public:
  nsTArray<PropertyValue*> mValues;

  friend std::ostream& operator<<(std::ostream& stream, JSCustomArray* entry);

  template<class T>
  void AppendElement(T aValue) {
    mValues.AppendElement(new TemplatePropertyValue<T>(aValue));
  }
};

template <typename T>
struct SendToStreamImpl
{
  static void run(std::ostream& stream, const T& t) {
    stream << t;
  }
};

template<typename T>
struct SendToStreamImpl<T*>
{
  static void run(std::ostream& stream, T* t) {
    stream << *t;
  }
};

template <>
struct SendToStreamImpl<char *>
{
  static void run(std::ostream& stream, char* p) {
    EscapeToStream(stream, p);
  }
};

template <>
struct SendToStreamImpl<JSCustomObject*>
{
  static void run(std::ostream& stream, JSCustomObject* p) {
    stream << p;
  }
};

template <>
struct SendToStreamImpl<JSCustomArray*>
{
  static void run(std::ostream& stream, JSCustomArray* p) {
    stream << p;
  }
};

template <class T> void
TemplatePropertyValue<T>::SendToStream(std::ostream& stream)
{
  SendToStreamImpl<T>::run(stream, mValue);
}

struct JSONStreamClosure {
  std::ostream& mStream;
  bool mNeedsComma;
};

PLDHashOperator HashTableOutput(const nsACString& aKey, PropertyValue* aValue, void* stream)
{
  JSONStreamClosure& streamClosure = *(JSONStreamClosure*)stream;
  if (streamClosure.mNeedsComma) {
    streamClosure.mStream << ",";
  }
  streamClosure.mNeedsComma = true;
  EscapeToStream(streamClosure.mStream, (const char*)aKey.BeginReading());
  streamClosure.mStream << ":";
  aValue->SendToStream(streamClosure.mStream);
  return PL_DHASH_NEXT;
}

std::ostream&
operator<<(std::ostream& stream, JSCustomObject* entry)
{
  JSONStreamClosure streamClosure = {stream, false};
  stream << "{";
  entry->mProperties.EnumerateRead(HashTableOutput, &streamClosure);
  stream << "}";
  return stream;
}

std::ostream&
operator<<(std::ostream& stream, JSCustomArray* entry)
{
  bool needsComma = false;
  stream << "[";
  for (size_t i = 0; i < entry->mValues.Length(); i++) {
    if (needsComma) {
      stream << ",";
    }
    entry->mValues[i]->SendToStream(stream);
    needsComma = true;
  }
  stream << "]";
  return stream;
}

PLDHashOperator HashTableFree(const nsACString& aKey, PropertyValue* aValue, void* stream)
{
  delete aValue;
  return PL_DHASH_NEXT;
}

JSCustomObject::~JSCustomObject()
{
  mProperties.EnumerateRead(HashTableFree, nullptr);
}

JSAObjectBuilder::~JSAObjectBuilder()
{
}

JSCustomObjectBuilder::JSCustomObjectBuilder()
{}

void
JSCustomObjectBuilder::DeleteObject(JSCustomObject* aObject)
{
  delete aObject;
}

void
JSCustomObjectBuilder::Serialize(JSCustomObject* aObject, std::ostream& stream)
{
  stream << aObject;
}

void
JSCustomObjectBuilder::DefineProperty(JSCustomObject *aObject, const char *name, JSCustomObject *aValue)
{
  aObject->AddProperty(name, aValue);
}

void
JSCustomObjectBuilder::DefineProperty(JSCustomObject *aObject, const char *name, JSCustomArray *aValue)
{
  aObject->AddProperty(name, aValue);
}

void
JSCustomObjectBuilder::DefineProperty(JSCustomObject *aObject, const char *name, int aValue)
{
  aObject->AddProperty(name, aValue);
}

void
JSCustomObjectBuilder::DefineProperty(JSCustomObject *aObject, const char *name, double aValue)
{
  aObject->AddProperty(name, aValue);
}

void
JSCustomObjectBuilder::DefineProperty(JSCustomObject *aObject, const char *name, const char *aValue)
{
  
  aObject->AddProperty(name, strdup(aValue));
}

void
JSCustomObjectBuilder::ArrayPush(JSCustomArray *aArray, int aValue)
{
  aArray->AppendElement(aValue);
}

void
JSCustomObjectBuilder::ArrayPush(JSCustomArray *aArray, const char *aValue)
{
  
  aArray->AppendElement(strdup(aValue));
}

void
JSCustomObjectBuilder::ArrayPush(JSCustomArray *aArray, JSCustomObject *aObject)
{
  aArray->AppendElement(aObject);
}

JSCustomArray*
JSCustomObjectBuilder::CreateArray() {
  return new JSCustomArray();
}

JSCustomObject*
JSCustomObjectBuilder::CreateObject() {
  return new JSCustomObject();
}

