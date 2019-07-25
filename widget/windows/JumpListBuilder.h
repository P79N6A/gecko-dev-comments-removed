






































#ifndef __JumpListBuilder_h__
#define __JumpListBuilder_h__

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include <windows.h>

#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN7

#include <shobjidl.h>

#include "nsString.h"
#include "nsIMutableArray.h"

#include "nsIJumpListBuilder.h"
#include "nsIJumpListItem.h"
#include "JumpListItem.h"
#include "nsIObserver.h"
#include "nsIFaviconService.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace widget {

class JumpListBuilder : public nsIJumpListBuilder, 
                        public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIJUMPLISTBUILDER
  NS_DECL_NSIOBSERVER

  JumpListBuilder();
  virtual ~JumpListBuilder();

protected:
  static bool sBuildingList; 

private:
  nsRefPtr<ICustomDestinationList> mJumpListMgr;
  PRUint32 mMaxItems;
  bool mHasCommit;
  nsCOMPtr<nsIThread> mIOThread;

  bool IsSeparator(nsCOMPtr<nsIJumpListItem>& item);
  nsresult TransferIObjectArrayToIMutableArray(IObjectArray *objArray, nsIMutableArray *removedItems);
  nsresult RemoveIconCacheForItems(nsIMutableArray *removedItems);
  nsresult RemoveIconCacheForAllItems();

  friend class WinTaskbar;
};


class AsyncFaviconDataReady : public nsIFaviconDataCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFAVICONDATACALLBACK

  AsyncFaviconDataReady(nsIURI *aNewURI, nsCOMPtr<nsIThread> &aIOThread);
private:
  nsCOMPtr<nsIURI> mNewURI;
  nsCOMPtr<nsIThread> mIOThread;
};




class AsyncWriteIconToDisk : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  
  AsyncWriteIconToDisk(const nsAString &aIconPath,
                       const nsACString &aMimeTypeOfInputData,
                       PRUint8 *aData, 
                       PRUint32 aDataLen);
  virtual ~AsyncWriteIconToDisk();

private:
  nsAutoString mIconPath;
  nsCAutoString mMimeTypeOfInputData;
  nsAutoArrayPtr<PRUint8> mBuffer;
  PRUint32 mBufferLength;
};

class AsyncDeleteIconFromDisk : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  AsyncDeleteIconFromDisk(const nsAString &aIconPath);
  virtual ~AsyncDeleteIconFromDisk();

private:
  nsAutoString mIconPath;
};

class AsyncDeleteAllFaviconsFromDisk : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  AsyncDeleteAllFaviconsFromDisk();
  virtual ~AsyncDeleteAllFaviconsFromDisk();
};

} 
} 

#endif 

#endif 

