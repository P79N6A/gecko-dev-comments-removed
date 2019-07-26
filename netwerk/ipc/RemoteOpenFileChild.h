





#ifndef _RemoteOpenFileChild_h
#define _RemoteOpenFileChild_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/net/PRemoteOpenFileChild.h"
#include "nsICachedFileDescriptorListener.h"
#include "nsILocalFile.h"
#include "nsIRemoteOpenFileListener.h"

namespace mozilla {

namespace ipc {
class FileDescriptor;
}

namespace net {




















class RemoteOpenFileChild MOZ_FINAL
  : public PRemoteOpenFileChild
  , public nsIFile
  , public nsIHashable
  , public nsICachedFileDescriptorListener
{
  typedef mozilla::dom::TabChild TabChild;
  typedef mozilla::ipc::FileDescriptor FileDescriptor;

public:
  RemoteOpenFileChild()
    : mNSPRFileDesc(nullptr)
    , mAsyncOpenCalled(false)
    , mNSPROpenCalled(false)
  {}

  virtual ~RemoteOpenFileChild();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFILE
  NS_DECL_NSIHASHABLE

  
  nsresult Init(nsIURI* aRemoteOpenUri);

  
  
  
  nsresult AsyncRemoteFileOpen(int32_t aFlags,
                               nsIRemoteOpenFileListener* aListener,
                               nsITabChild* aTabChild);

  void ReleaseIPDLReference()
  {
    Release();
  }

private:
  RemoteOpenFileChild(const RemoteOpenFileChild& other);

protected:
  void AddIPDLReference()
  {
    AddRef();
  }

  virtual bool Recv__delete__(const FileDescriptor&) MOZ_OVERRIDE;

  virtual void OnCachedFileDescriptor(const nsAString& aPath,
                                      const FileDescriptor& aFD) MOZ_OVERRIDE;

  void HandleFileDescriptorAndNotifyListener(const FileDescriptor&,
                                             bool aFromRecvDelete);

  void NotifyListener(nsresult aResult);

  
  nsCOMPtr<nsIFile> mFile;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIRemoteOpenFileListener> mListener;
  nsRefPtr<TabChild> mTabChild;
  PRFileDesc* mNSPRFileDesc;
  bool mAsyncOpenCalled;
  bool mNSPROpenCalled;
};

} 
} 

#endif 
