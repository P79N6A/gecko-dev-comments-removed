





#ifndef mozilla_dom_asmjscache_asmjscache_h
#define mozilla_dom_asmjscache_asmjscache_h

#include "ipc/IPCMessageUtils.h"
#include "js/TypeDecls.h"
#include "js/Vector.h"
#include "jsapi.h"

class nsIPrincipal;

namespace mozilla {
namespace dom {

namespace quota {
class Client;
}

namespace asmjscache {

class PAsmJSCacheEntryChild;
class PAsmJSCacheEntryParent;

enum OpenMode
{
  eOpenForRead,
  eOpenForWrite,
  NUM_OPEN_MODES
};





struct Metadata
{
  static const unsigned kNumEntries = 16;
  static const unsigned kLastEntry = kNumEntries - 1;

  struct Entry
  {
    uint32_t mFastHash;
    uint32_t mNumChars;
    uint32_t mFullHash;
    unsigned mModuleIndex;

    void clear() {
      mFastHash = -1;
      mNumChars = -1;
      mFullHash = -1;
    }
  };

  Entry mEntries[kNumEntries];
};


struct WriteParams
{
  int64_t mSize;
  int64_t mFastHash;
  int64_t mNumChars;
  int64_t mFullHash;
  bool mInstalled;

  WriteParams()
  : mSize(0),
    mFastHash(0),
    mNumChars(0),
    mFullHash(0),
    mInstalled(false)
  { }
};


struct ReadParams
{
  const char16_t* mBegin;
  const char16_t* mLimit;

  ReadParams()
  : mBegin(nullptr),
    mLimit(nullptr)
  { }
};













bool
OpenEntryForRead(nsIPrincipal* aPrincipal,
                 const char16_t* aBegin,
                 const char16_t* aLimit,
                 size_t* aSize,
                 const uint8_t** aMemory,
                 intptr_t *aHandle);
void
CloseEntryForRead(size_t aSize,
                  const uint8_t* aMemory,
                  intptr_t aHandle);
bool
OpenEntryForWrite(nsIPrincipal* aPrincipal,
                  bool aInstalled,
                  const char16_t* aBegin,
                  const char16_t* aEnd,
                  size_t aSize,
                  uint8_t** aMemory,
                  intptr_t* aHandle);
void
CloseEntryForWrite(size_t aSize,
                   uint8_t* aMemory,
                   intptr_t aHandle);

bool
GetBuildId(JS::BuildIdCharVector* aBuildId);



quota::Client*
CreateClient();



PAsmJSCacheEntryParent*
AllocEntryParent(OpenMode aOpenMode, WriteParams aWriteParams,
                 nsIPrincipal* aPrincipal);

void
DeallocEntryParent(PAsmJSCacheEntryParent* aActor);



void
DeallocEntryChild(PAsmJSCacheEntryChild* aActor);

} 
} 
} 

namespace IPC {

template <>
struct ParamTraits<mozilla::dom::asmjscache::OpenMode> :
  public ContiguousEnumSerializer<mozilla::dom::asmjscache::OpenMode,
                                  mozilla::dom::asmjscache::eOpenForRead,
                                  mozilla::dom::asmjscache::NUM_OPEN_MODES>
{ };

template <>
struct ParamTraits<mozilla::dom::asmjscache::Metadata>
{
  typedef mozilla::dom::asmjscache::Metadata paramType;
  static void Write(Message* aMsg, const paramType& aParam);
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult);
  static void Log(const paramType& aParam, std::wstring* aLog);
};

template <>
struct ParamTraits<mozilla::dom::asmjscache::WriteParams>
{
  typedef mozilla::dom::asmjscache::WriteParams paramType;
  static void Write(Message* aMsg, const paramType& aParam);
  static bool Read(const Message* aMsg, void** aIter, paramType* aResult);
  static void Log(const paramType& aParam, std::wstring* aLog);
};

} 

#endif
