








#ifndef nsMenuItemIconX_h_
#define nsMenuItemIconX_h_

#include "nsCOMPtr.h"
#include "imgINotificationObserver.h"

class nsIURI;
class nsIContent;
class imgIRequest;
class nsMenuObjectX;

#import <Cocoa/Cocoa.h>

class nsMenuItemIconX : public imgINotificationObserver
{
public:
  nsMenuItemIconX(nsMenuObjectX* aMenuItem,
                  nsIContent*    aContent,
                  NSMenuItem*    aNativeMenuItem);
private:
  virtual ~nsMenuItemIconX();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER

  
  
  nsresult SetupIcon();

  
  nsresult GetIconURI(nsIURI** aIconURI);

  
  
  nsresult LoadIcon(nsIURI* aIconURI);

  
  
  
  
  void Destroy();

protected:
  nsresult OnStopFrame(imgIRequest* aRequest);

  nsCOMPtr<nsIContent>  mContent;
  nsCOMPtr<imgIRequest> mIconRequest;
  nsMenuObjectX*        mMenuObject; 
  nsIntRect             mImageRegionRect;
  bool                  mLoadedIcon;
  bool                  mSetIcon;
  NSMenuItem*           mNativeMenuItem; 
};

#endif 
