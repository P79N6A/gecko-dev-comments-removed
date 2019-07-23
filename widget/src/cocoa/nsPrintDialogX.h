





































#ifndef nsPrintDialogX_h__
#define nsPrintDialogX_h__

#include "nsIPrintDialogService.h"
#include "nsCOMPtr.h"

#import <Cocoa/Cocoa.h>

class nsIPrintSettings;
class nsIStringBundle;

class nsPrintDialogServiceX : public nsIPrintDialogService
{
public:
  nsPrintDialogServiceX();
  virtual ~nsPrintDialogServiceX();

  NS_DECL_ISUPPORTS

  NS_IMETHODIMP Init();
  NS_IMETHODIMP Show(nsIDOMWindow *aParent, nsIPrintSettings *aSettings);
  NS_IMETHODIMP ShowPageSetup(nsIDOMWindow *aParent,
                              nsIPrintSettings *aSettings);
};

@interface PrintPanelAccessoryView : NSView
{
  nsIPrintSettings* mSettings;
  nsIStringBundle* mPrintBundle;
  NSButton* mPrintSelectionOnlyCheckbox;
  NSButton* mShrinkToFitCheckbox;
  NSButton* mPrintBGColorsCheckbox;
  NSButton* mPrintBGImagesCheckbox;
  NSButtonCell* mAsLaidOutRadio;
  NSButtonCell* mSelectedFrameRadio;
  NSButtonCell* mSeparateFramesRadio;
  NSPopUpButton* mHeaderLeftList;
  NSPopUpButton* mHeaderCenterList;
  NSPopUpButton* mHeaderRightList;
  NSPopUpButton* mFooterLeftList;
  NSPopUpButton* mFooterCenterList;
  NSPopUpButton* mFooterRightList;
}

- (id)initWithSettings:(nsIPrintSettings*)aSettings;

- (void)exportSettings;

@end

@interface PrintPanelAccessoryController : NSViewController <NSPrintPanelAccessorizing>

- (id)initWithSettings:(nsIPrintSettings*)aSettings;

- (void)exportSettings;

@end

#endif
