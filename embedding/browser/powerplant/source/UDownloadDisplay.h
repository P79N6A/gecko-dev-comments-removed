





































#ifndef UDownloadDisplay_h__
#define UDownloadDisplay_h__
#pragma once

#include "UDownload.h"

#include <LWindow.h>

class CMultiDownloadProgressWindow;
class CDownloadProgressView;

class LProgressBar;
class LStaticText;

















class CMultiDownloadProgress : public ADownloadProgressView,
                               public LCommander,
                               public LListener
{
public:
                            CMultiDownloadProgress();
    virtual                 ~CMultiDownloadProgress();

    
    virtual void            AddDownloadItem(CDownload *aDownloadItem);
    
    
    virtual Boolean         AllowSubRemoval(LCommander* inSub);
    virtual Boolean         AttemptQuitSelf(SInt32 inSaveOption);
    
    
    virtual void            ListenToMessage(MessageT inMessage,
                                            void* ioParam);

protected:
    static bool             sRegisteredViewClasses;

    CMultiDownloadProgressWindow *mWindow;
};








class CMultiDownloadProgressWindow : public LWindow,
                                     public LBroadcaster
{
public:
    enum { class_ID = FOUR_CHAR_CODE('MDPW') };

                            CMultiDownloadProgressWindow();
                            CMultiDownloadProgressWindow(LStream* inStream);
                            
    virtual                 ~CMultiDownloadProgressWindow();
                                            
    
    virtual void            AddDownloadView(CDownloadProgressView *aView);
    virtual void            RemoveDownloadView(CDownloadProgressView *aView);
    virtual Boolean         ConfirmClose();    
    
protected:
    SInt32                  mDownloadViewCount;
};








class CDownloadProgressView : public LView,
                              public LCommander,
                              public LListener
{
public:
    enum { class_ID = FOUR_CHAR_CODE('DPrV') };

                            CDownloadProgressView();
                            CDownloadProgressView(LStream* inStream);
                            
    virtual                 ~CDownloadProgressView();

    
    virtual Boolean         ObeyCommand(CommandT inCommand,
                                        void *ioParam);
                                        

    
    virtual void            FinishCreateSelf();

    
    virtual void            ListenToMessage(MessageT inMessage,
                                            void* ioParam);

    
    virtual void            SetDownload(CDownload *aDownload);
    virtual void            CancelDownload();
    virtual Boolean         IsActive();
                                                        
    virtual void            UpdateStatus(CDownload::MsgOnDLProgressChangeInfo *info);
    LStr255&                FormatBytes(float inBytes, LStr255& ioString);
    LStr255&                FormatFuzzyTime(PRInt32 inSecs, LStr255& ioString);

    static OSErr            CreateStyleRecFromThemeFont(ThemeFontID inThemeID,
                                                        ControlFontStyleRec& outStyle);

protected:
    enum { kStatusUpdateIntervalTicks = 60 };

    LProgressBar            *mProgressBar;
    LControl                *mCancelButton, *mOpenButton, *mRevealButton;
    LControl                *mCloseButton;
    LStaticText             *mStatusText, *mTimeRemainingText;
    LStaticText             *mSrcURIText, *mDestFileText;

    nsCOMPtr<nsIDownload>   mDownload;
    Boolean                 mDownloadActive;
    SInt32                  mLastStatusUpdateTicks;
};

#endif
