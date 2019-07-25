






































#ifndef __JumpListItem_h__
#define __JumpListItem_h__

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include <windows.h>
#include <shobjidl.h>

#include "nsIJumpListItem.h"  
#include "nsIMIMEInfo.h" 
#include "nsTArray.h"
#include "nsIMutableArray.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIURI.h"
#include "nsICryptoHash.h"
#include "nsString.h"
#include "nsCycleCollectionParticipant.h"

class nsIThread;

namespace mozilla {
namespace widget {

class JumpListItem : public nsIJumpListItem
{
public:
  JumpListItem() :
   mItemType(nsIJumpListItem::JUMPLIST_ITEM_EMPTY)
  {}

  JumpListItem(PRInt32 type) :
   mItemType(type)
  {}

  virtual ~JumpListItem() 
  {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIJUMPLISTITEM

  static const char kJumpListCacheDir[];

protected:
  short Type() { return mItemType; }
  short mItemType;

  static nsresult HashURI(nsCOMPtr<nsICryptoHash> &aCryptoHash,
                          nsIURI *aUri, nsACString& aUriHash);
};

class JumpListSeparator : public JumpListItem, public nsIJumpListSeparator
{
public:
  JumpListSeparator() :
   JumpListItem(nsIJumpListItem::JUMPLIST_ITEM_SEPARATOR)
  {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_IMETHOD GetType(PRInt16 *aType) { return JumpListItem::GetType(aType); }
  NS_IMETHOD Equals(nsIJumpListItem *item, bool *_retval) { return JumpListItem::Equals(item, _retval); }

  static nsresult GetSeparator(nsRefPtr<IShellLinkW>& aShellLink);
};

class JumpListLink : public JumpListItem, public nsIJumpListLink
{
public:
  JumpListLink() :
   JumpListItem(nsIJumpListItem::JUMPLIST_ITEM_LINK)
  {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_IMETHOD GetType(PRInt16 *aType) { return JumpListItem::GetType(aType); }
  NS_IMETHOD Equals(nsIJumpListItem *item, bool *_retval);
  NS_DECL_NSIJUMPLISTLINK

  static nsresult GetShellItem(nsCOMPtr<nsIJumpListItem>& item, nsRefPtr<IShellItem2>& aShellItem);
  static nsresult GetJumpListLink(IShellItem *pItem, nsCOMPtr<nsIJumpListLink>& aLink);

protected:
  typedef HRESULT (WINAPI * SHCreateItemFromParsingNamePtr)(PCWSTR pszPath, IBindCtx *pbc, REFIID riid, void **ppv);

  nsString mUriTitle;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsICryptoHash> mCryptoHash;
  static const PRUnichar kSehllLibraryName[];
  static HMODULE sShellDll;
  static SHCreateItemFromParsingNamePtr createItemFromParsingName;
};

class JumpListShortcut : public JumpListItem, public nsIJumpListShortcut
{
public:
  JumpListShortcut() :
   JumpListItem(nsIJumpListItem::JUMPLIST_ITEM_SHORTCUT)
  {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(JumpListShortcut, JumpListItem);
  NS_IMETHOD GetType(PRInt16 *aType) { return JumpListItem::GetType(aType); }
  NS_IMETHOD Equals(nsIJumpListItem *item, bool *_retval);
  NS_DECL_NSIJUMPLISTSHORTCUT

  static nsresult GetShellLink(nsCOMPtr<nsIJumpListItem>& item, 
                               nsRefPtr<IShellLinkW>& aShellLink, 
                               nsCOMPtr<nsIThread> &aIOThread);
  static nsresult GetJumpListShortcut(IShellLinkW *pLink, nsCOMPtr<nsIJumpListShortcut>& aShortcut);
  static nsresult GetOutputIconPath(nsCOMPtr<nsIURI> aFaviconPageURI, 
                                    nsCOMPtr<nsIFile> &aICOFile);

protected:
  PRInt32 mIconIndex;
  nsCOMPtr<nsIURI> mFaviconPageURI;
  nsCOMPtr<nsILocalHandlerApp> mHandlerApp;

  bool ExecutableExists(nsCOMPtr<nsILocalHandlerApp>& handlerApp);
  static nsresult ObtainCachedIconFile(nsCOMPtr<nsIURI> aFaviconPageURI, 
                                       nsString &aICOFilePath,
                                       nsCOMPtr<nsIThread> &aIOThread);
  static nsresult CacheIconFileFromFaviconURIAsync(nsCOMPtr<nsIURI> aFaviconPageURI, 
                                                   nsCOMPtr<nsIFile> aICOFile,
                                                   nsCOMPtr<nsIThread> &aIOThread);
};

} 
} 

#endif 

#endif 
