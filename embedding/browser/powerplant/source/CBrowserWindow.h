






































#include <LWindow.h>
#include <LListener.h>
#include <LBroadcaster.h>

class CBrowserShell;
class LEditText;
class LStaticText;
class CThrobber;
class LProgressBar;
class LIconControl;







class CBrowserWindow :  public LWindow,
                        public LListener,
                        public LBroadcaster
{
private:
    typedef LWindow Inherited;

public:
    enum { class_ID = FOUR_CHAR_CODE('BroW') };

                                CBrowserWindow();
                                CBrowserWindow(LCommander*      inSuperCommander,
                                               const Rect&      inGlobalBounds,
                                               ConstStringPtr   inTitle,
                                               SInt16           inProcID,
                                               UInt32           inAttributes,
                                               WindowPtr        inBehind);
                                CBrowserWindow(LStream* inStream);

    virtual                     ~CBrowserWindow();

    virtual void                FinishCreateSelf();

    virtual void                ListenToMessage(MessageT        inMessage,
                                                 void*          ioParam);

    virtual Boolean             ObeyCommand(CommandT            inCommand,
                                            void                *ioParam);
                                    
protected:
    CBrowserShell*              mBrowserShell;
    
    LEditText*                  mURLField;
    LStaticText*                mStatusBar;
    CThrobber*                  mThrobber;
    LControl                    *mBackButton, *mForwardButton, *mReloadButton, *mStopButton;
    LProgressBar*               mProgressBar;
    LIconControl*               mLockIcon;
};






nsresult InitializeWindowCreator();

