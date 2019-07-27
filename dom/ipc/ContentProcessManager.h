





#ifndef mozilla_dom_ContentProcessManager_h
#define mozilla_dom_ContentProcessManager_h

#include <map>
#include <set>
#include "mozilla/StaticPtr.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/dom/ipc/IdType.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {
class ContentParent;

struct RemoteFrameInfo
{
  TabId mOpenerTabId;
  TabContext mContext;
};

struct ContentProcessInfo
{
  ContentParent* mCp;
  ContentParentId mParentCpId;
  std::set<ContentParentId> mChildrenCpId;
  std::map<TabId, RemoteFrameInfo> mRemoteFrames;
};

class ContentProcessManager MOZ_FINAL
{
public:
  static ContentProcessManager* GetSingleton();
  ~ContentProcessManager() {MOZ_COUNT_DTOR(ContentProcessManager);};

  



  void AddContentProcess(ContentParent* aChildCp,
                         const ContentParentId& aParentCpId = ContentParentId(0));
  


  void RemoveContentProcess(const ContentParentId& aChildCpId);
  



  bool AddGrandchildProcess(const ContentParentId& aParentCpId,
                            const ContentParentId& aChildCpId);
  



  bool GetParentProcessId(const ContentParentId& aChildCpId,
                           ContentParentId* aParentCpId);
  


  ContentParent* GetContentProcessById(const ContentParentId& aChildCpId);

  


  nsTArray<ContentParentId>
  GetAllChildProcessById(const ContentParentId& aParentCpId);

  





  TabId AllocateTabId(const TabId& aOpenerTabId,
                      const IPCTabContext& aContext,
                      const ContentParentId& aChildCpId);

  


  void DeallocateTabId(const ContentParentId& aChildCpId,
                       const TabId& aChildTabId);

  



  nsTArray<uint64_t>
  GetAppIdsByContentProcess(const ContentParentId& aChildCpId);

  



  nsTArray<TabContext>
  GetTabContextByContentProcess(const ContentParentId& aChildCpId);

  



  bool GetRemoteFrameOpenerTabId(const ContentParentId& aChildCpId,
                                 const TabId& aChildTabId,
                                  TabId* aOpenerTabId);

private:
  static StaticAutoPtr<ContentProcessManager> sSingleton;
  TabId mUniqueId;
  std::map<ContentParentId, ContentProcessInfo> mContentParentMap;

  ContentProcessManager() {MOZ_COUNT_CTOR(ContentProcessManager);};
};

} 
} 
#endif