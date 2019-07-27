





#ifndef mozilla_dom_IdType_h
#define mozilla_dom_IdType_h

namespace mozilla {
namespace dom {
class ContentParent;


template<typename T>
class IdType
{

  friend struct IPC::ParamTraits<IdType<T>>;

public:
  IdType() : mId(0) {}
  explicit IdType(uint64_t aId) : mId(aId) {}

  operator uint64_t() const { return mId; }

  IdType& operator=(uint64_t aId)
  {
    mId = aId;
    return *this;
  }

  bool operator<(const IdType& rhs)
  {
    return mId < rhs.mId;
  }
private:
  uint64_t mId;
};

typedef IdType<ContentParent> ContentParentId;

} 
} 

namespace IPC {

template<typename T>
struct ParamTraits<mozilla::dom::IdType<T>>
{
  typedef mozilla::dom::IdType<T> paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mId);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return ReadParam(aMsg, aIter, &aResult->mId);
  }
};

}

#endif