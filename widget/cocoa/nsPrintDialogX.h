




#ifndef nsPrintDialog_h_
#define nsPrintDialog_h_

#include "nsIPrintDialogService.h"
#include "nsCOMPtr.h"
#include "nsCocoaUtils.h"

#import <Cocoa/Cocoa.h>

class nsIPrintSettings;
class nsIStringBundle;

class nsPrintDialogServiceX : public nsIPrintDialogService
{
public:
  nsPrintDialogServiceX();

  NS_DECL_ISUPPORTS

  NS_IMETHODIMP Init() override;
  NS_IMETHODIMP Show(nsIDOMWindow *aParent, nsIPrintSettings *aSettings,
                     nsIWebBrowserPrint *aWebBrowserPrint) override;
  NS_IMETHODIMP ShowPageSetup(nsIDOMWindow *aParent,
                              nsIPrintSettings *aSettings) override;

protected:
  virtual ~nsPrintDialogServiceX();
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
