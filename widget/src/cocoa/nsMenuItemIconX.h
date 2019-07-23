









































#ifndef nsMenuItemIconX_h_
#define nsMenuItemIconX_h_

#include "nsCOMPtr.h"
#include "imgIDecoderObserver.h"

class nsIURI;
class nsIContent;
class imgIRequest;
class nsMenuObjectX;

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

class nsMenuItemIconX : public imgIDecoderObserver
{
public:
  nsMenuItemIconX(nsMenuObjectX* aMenuItem,
                  nsIContent*    aContent,
                  NSMenuItem*    aNativeMenuItem);
private:
  ~nsMenuItemIconX();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINEROBSERVER
  NS_DECL_IMGIDECODEROBSERVER

  
  
  nsresult SetupIcon();

  
  nsresult GetIconURI(nsIURI** aIconURI);

  
  
  nsresult LoadIcon(nsIURI* aIconURI);

  
  
  
  
  void Destroy();

protected:
  nsCOMPtr<nsIContent>  mContent;
  nsCOMPtr<imgIRequest> mIconRequest;
  nsMenuObjectX*        mMenuObject; 
  nsIntRect             mImageRegionRect;
  PRPackedBool          mLoadedIcon;
  PRPackedBool          mSetIcon;
  NSMenuItem*           mNativeMenuItem; 
};

#endif 
