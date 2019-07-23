




































































































#import <AppKit/AppKit.h>

#include "nsISupports.h"

class CHDownloader;




@protocol CHDownloadProgressDisplay

- (void)onStartDownload:(BOOL)isFileSave;
- (void)onEndDownload;

- (void)setProgressTo:(long)aCurProgress ofMax:(long)aMaxProgress;

- (void)setDownloadListener:(CHDownloader*)aDownloader;
- (void)setSourceURL:(NSString*)aSourceURL;
- (void)setDestinationPath:(NSString*)aDestPath;

@end



@interface DownloadControllerFactory : NSObject
{
}

- (NSWindowController<CHDownloadProgressDisplay> *)createDownloadController;

@end







class CHDownloader : public nsISupports
{
public:
                  CHDownloader(DownloadControllerFactory* inControllerFactory);
    virtual       ~CHDownloader();

    NS_DECL_ISUPPORTS

    virtual void PauseDownload() = 0;
    virtual void ResumeDownload() = 0;
    virtual void CancelDownload() = 0;
    virtual void DownloadDone() = 0;
    virtual void DetachDownloadDisplay() = 0;		

    virtual void CreateDownloadDisplay();
    
protected:
  
    PRBool      IsFileSave() { return mIsFileSave; }
    void        SetIsFileSave(PRBool inIsFileSave) { mIsFileSave = inIsFileSave; }

protected:

    DownloadControllerFactory*    mControllerFactory;
    id <CHDownloadProgressDisplay>  mDownloadDisplay;   
    PRBool                        mIsFileSave;        
};

