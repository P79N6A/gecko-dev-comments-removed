





#ifndef mozilla_dom_cache_StreamList_h
#define mozilla_dom_cache_StreamList_h

#include "mozilla/dom/cache/Types.h"
#include "nsRefPtr.h"
#include "nsTArray.h"

class nsIInputStream;

namespace mozilla {
namespace dom {
namespace cache {

class CacheStreamControlParent;
class Context;
class Manager;

class StreamList
{
public:
  StreamList(Manager* aManager, Context* aContext);

  void SetStreamControl(CacheStreamControlParent* aStreamControl);
  void RemoveStreamControl(CacheStreamControlParent* aStreamControl);

  void Activate(CacheId aCacheId);

  void Add(const nsID& aId, nsIInputStream* aStream);
  already_AddRefed<nsIInputStream> Extract(const nsID& aId);

  void NoteClosed(const nsID& aId);
  void NoteClosedAll();
  void Close(const nsID& aId);
  void CloseAll();

private:
  ~StreamList();
  struct Entry
  {
    nsID mId;
    nsCOMPtr<nsIInputStream> mStream;
  };
  nsRefPtr<Manager> mManager;
  nsRefPtr<Context> mContext;
  CacheId mCacheId;
  CacheStreamControlParent* mStreamControl;
  nsTArray<Entry> mList;
  bool mActivated;

public:
  NS_INLINE_DECL_REFCOUNTING(cache::StreamList)
};

} 
} 
} 

#endif
