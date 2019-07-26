





#ifndef _RemoteOpenFileChild_h
#define _RemoteOpenFileChild_h

#include "mozilla/dom/TabChild.h"
#include "mozilla/net/PRemoteOpenFileChild.h"
#include "nsILocalFile.h"
#include "nsIRemoteOpenFileListener.h"

namespace mozilla {
namespace net {




















class RemoteOpenFileChild MOZ_FINAL
  : public PRemoteOpenFileChild
  , public nsIFile
  , public nsIHashable
{
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

  void AddIPDLReference();
  void ReleaseIPDLReference();

  
  
  
  nsresult AsyncRemoteFileOpen(int32_t aFlags,
                               nsIRemoteOpenFileListener* aListener,
                               nsITabChild* aTabChild);
private:
  RemoteOpenFileChild(const RemoteOpenFileChild& other);

protected:
  virtual bool RecvFileOpened(const FileDescriptor&);
  virtual bool RecvFileDidNotOpen();

  
  nsCOMPtr<nsIFile> mFile;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIRemoteOpenFileListener> mListener;
  PRFileDesc* mNSPRFileDesc;
  bool mAsyncOpenCalled;
  bool mNSPROpenCalled;
};

} 
} 

#endif 

