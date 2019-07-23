










































#ifndef nsMenuItemIconX_h_
#define nsMenuItemIconX_h_


#include "nsCOMPtr.h"
#include "imgIDecoderObserver.h"

class nsIURI;
class nsIContent;
class imgIRequest;
class nsIMenu;

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>


class nsMenuItemIconX : public imgIDecoderObserver
{
public:
  nsMenuItemIconX(nsISupports* aMenuItem,
                 nsIMenu*     aMenu,
                 nsIContent*  aContent,
                 NSMenuItem* aNativeMenuItem);
private:
  ~nsMenuItemIconX();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINEROBSERVER
  NS_DECL_IMGIDECODEROBSERVER

  
  
  nsresult SetupIcon();

  
  nsresult GetIconURI(nsIURI** aIconURI);

  
  
  nsresult LoadIcon(nsIURI* aIconURI);

  
  
  PRBool ShouldLoadSync(nsIURI* aURI);

protected:
  nsCOMPtr<nsIContent>  mContent;
  nsCOMPtr<imgIRequest> mIconRequest;
  nsISupports*          mMenuItem;
  nsIMenu*              mMenu;
  MenuRef               mMenuRef;
  PRUint16              mMenuItemIndex;
  PRPackedBool          mLoadedIcon;
  PRPackedBool          mSetIcon;
  NSMenuItem*           mNativeMenuItem;
};

#endif 
