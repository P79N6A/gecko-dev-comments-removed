









































#ifndef nsMenuItemIcon_h__
#define nsMenuItemIcon_h__


#include "nsCOMPtr.h"
#include "imgIDecoderObserver.h"

class nsIURI;
class nsIContent;
class imgIRequest;
class nsIMenu;

#include <Carbon/Carbon.h>


class nsMenuItemIcon : public imgIDecoderObserver
{
public:
  nsMenuItemIcon(nsISupports* aMenuItem,
                 nsIMenu*     aMenu,
                 nsIContent*  aContent);
private:
  ~nsMenuItemIcon();

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
};

#endif 
