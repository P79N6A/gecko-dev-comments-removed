





































#ifndef __PDECUSTOM__
#define __PDECUSTOM__

#ifndef nsPDECommon_h___
#include "nsPDECommon.h"
#endif












#define  kMyBundleIdentifier \
            CFSTR("org.mozilla.pde." kMozPDESignature)
















#define  kMyPaneKindID  kMyBundleIdentifier











#define  kMyNibFile  CFSTR("PrintPDE")










#define  kMyNibWindow  CFSTR("PrintPDE")













#define  kMyAppPrintSettingsKey \
            CFSTR("com.apple.print.PrintSettingsTicket." kMozPDESignature)












#define  kMyAppPageFormatKey \
            CFSTR("com.apple.print.PageFormatTicket." kMozPDESignature)




















enum {
    kMyMaxV = 292,
    kMyMaxH = 478
};







enum {
    kFramesAsLaidOutIndex = 1,
    kFramesSelectedIndex,
    kFramesEachSeparatelyIndex,
};








typedef struct {
    ControlRef frameRadioGroup;
    ControlRef printSelCheck;
    ControlRef shrinkToFitCheck;
    ControlRef printBGColorsCheck;
    ControlRef printBGImagesCheck;
    ControlRef headerLeftPopup;
    ControlRef headerCenterPopup;
    ControlRef headerRightPopup;
    ControlRef footerLeftPopup;
    ControlRef footerCenterPopup;
    ControlRef footerRightPopup;
} MyControls;

typedef struct {
  
  Boolean mHaveSelection;
  Boolean mHaveFrames;
  Boolean mHaveFrameSelected;
  
  Boolean mPrintFrameAsIs;
  Boolean mPrintSelectedFrame;
  Boolean mPrintFramesSeparately;
  Boolean mPrintSelection;
  Boolean mShrinkToFit;
  Boolean mPrintBGColors;
  Boolean mPrintBGImages;
  CFStringRef mHeaderLeft, mHeaderCenter, mHeaderRight;
  CFStringRef mFooterLeft, mFooterCenter, mFooterRight;
} MySettings;

typedef struct {
    MyControls controls;
    MySettings settings;
} MyCustomContextBlock;

typedef MyCustomContextBlock *MyCustomContext;













extern CFStringRef  MyGetCustomTitle (Boolean stillNeeded);

extern MyCustomContext  MyCreateCustomContext();
extern void             MyReleaseCustomContext (MyCustomContext);

extern OSStatus  MyEmbedCustomControls  (MyCustomContext, WindowRef, ControlRef);
extern OSStatus  MyGetSummaryText       (MyCustomContext, CFMutableArrayRef, CFMutableArrayRef);
extern OSStatus  MySyncPaneFromTicket   (MyCustomContext, PMPrintSession);
extern OSStatus  MySyncTicketFromPane   (MyCustomContext, PMPrintSession);


#endif
