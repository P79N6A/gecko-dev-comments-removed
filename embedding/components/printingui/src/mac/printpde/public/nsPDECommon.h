




































#ifndef nsPDECommon_h___
#define nsPDECommon_h___


#define  kMozPDESignature       "MOZZ"
#define  kMozPDECreatorCode     'MOZZ'






#define kPDEKeyHaveSelection            CFSTR("HaveSelection")          // Value: CFBoolean
#define kPDEKeyHaveFrames               CFSTR("HaveFrames")             // Value: CFBoolean
#define kPDEKeyHaveFrameSelected        CFSTR("HaveFrameSelected")      // Value: CFBoolean


#define kPDEKeyPrintSelection           CFSTR("PrintSelection")         // Value: CFBoolean
#define kPDEKeyPrintFrameType           CFSTR("PrintFrameType")         // Value: CFStringReference - one of the following:
    #define kPDEValueFramesAsIs         CFSTR("FramesAsIs")
    #define kPDEValueSelectedFrame      CFSTR("SelectedFrame")
    #define kPDEValueEachFrameSep       CFSTR("EachFrameSep")
#define kPDEKeyShrinkToFit              CFSTR("ShrinkToFit")            // Value: CFBoolean
#define kPDEKeyPrintBGColors            CFSTR("PrintBGColors")          // Value: CFBoolean
#define kPDEKeyPrintBGImages            CFSTR("PrintBGImages")          // Value: CFBoolean








#define kPDEKeyHeaderLeft               CFSTR("HeaderLeft")             // Value: CFStringReference
#define kPDEKeyHeaderCenter             CFSTR("HeaderCenter")           // Value: CFStringReference
#define kPDEKeyHeaderRight              CFSTR("HeaderRight")            // Value: CFStringReference
#define kPDEKeyFooterLeft               CFSTR("FooterLeft")             // Value: CFStringReference
#define kPDEKeyFooterCenter             CFSTR("FooterCenter")           // Value: CFStringReference
#define kPDEKeyFooterRight              CFSTR("FooterRight")            // Value: CFStringReference





#define kAppPrintDialogPDEOnlyKey       CFSTR("com.apple.print.PrintSettingsTicket." "GEKO")
#define kAppPrintDialogAppOnlyKey       'GEKO'


#endif
