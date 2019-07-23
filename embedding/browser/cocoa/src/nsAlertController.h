



































 
#import <Cocoa/Cocoa.h>

@interface nsAlertController : NSObject
{
    IBOutlet id alertCheckPanel;
    IBOutlet id alertCheckPanelCheck;
    IBOutlet id alertCheckPanelText;
    IBOutlet id alertPanel;
    IBOutlet id alertPanelText;
    IBOutlet id confirmCheckPanel;
    IBOutlet id confirmCheckPanelCheck;
    IBOutlet id confirmCheckPanelText;
    IBOutlet id confirmCheckPanelButton1;
    IBOutlet id confirmCheckPanelButton2;
    IBOutlet id confirmCheckPanelButton3;
    IBOutlet id confirmPanel;
    IBOutlet id confirmPanelText;
    IBOutlet id confirmPanelButton1;
    IBOutlet id confirmPanelButton2;
    IBOutlet id confirmPanelButton3;
    IBOutlet id confirmStorePasswordPanel;
    IBOutlet id promptPanel;
    IBOutlet id promptPanelCheck;
    IBOutlet id promptPanelText;
    IBOutlet id promptPanelInput;
    IBOutlet id passwordPanel;
    IBOutlet id passwordPanelCheck;
    IBOutlet id passwordPanelText;
    IBOutlet id passwordPanelInput;
    IBOutlet id postToInsecureFromSecurePanel;
    IBOutlet id securityMismatchPanel;
    IBOutlet id expiredCertPanel;
    IBOutlet id securityUnknownPanel;
    IBOutlet id usernamePanel;
    IBOutlet id usernamePanelCheck;
    IBOutlet id usernamePanelText;
    IBOutlet id usernamePanelPassword;    
    IBOutlet id usernamePanelUserName;    
    IBOutlet id owner;
}
- (IBAction)hitButton1:(id)sender;
- (IBAction)hitButton2:(id)sender;
- (IBAction)hitButton3:(id)sender;

- (void)awakeFromNib;
- (void)alert:(NSWindow*)parent title:(NSString*)title text:(NSString*)text;
- (void)alertCheck:(NSWindow*)parent title:(NSString*)title text:(NSString*)text checkMsg:(NSString*)checkMsg checkValue:(BOOL*)checkValue;

- (BOOL)confirm:(NSWindow*)parent title:(NSString*)title text:(NSString*)text;
- (BOOL)confirmCheck:(NSWindow*)parent title:(NSString*)title text:(NSString*)text checkMsg:(NSString*)checkMsg checkValue:(BOOL*)checkValue;

- (int)confirmEx:(NSWindow*)parent title:(NSString*)title text:(NSString*)text
   button1:(NSString*)btn1 button2:(NSString*)btn2 button3:(NSString*)btn3;
- (int)confirmCheckEx:(NSWindow*)parent title:(NSString*)title text:(NSString*)text 
  button1:(NSString*)btn1 button2:(NSString*)btn2 button3:(NSString*)btn3
  checkMsg:(NSString*)checkMsg checkValue:(BOOL*)checkValue;
- (BOOL)confirmStorePassword:(NSWindow*)parent;

- (BOOL)prompt:(NSWindow*)parent title:(NSString*)title text:(NSString*)text promptText:(NSMutableString*)promptText checkMsg:(NSString*)checkMsg checkValue:(BOOL*)checkValue doCheck:(BOOL)doCheck;
- (BOOL)promptUserNameAndPassword:(NSWindow*)parent title:(NSString*)title text:(NSString*)text userNameText:(NSMutableString*)userNameText passwordText:(NSMutableString*)passwordText checkMsg:(NSString*)checkMsg checkValue:(BOOL*)checkValue doCheck:(BOOL)doCheck;
- (BOOL)promptPassword:(NSWindow*)parent title:(NSString*)title text:(NSString*)text passwordText:(NSMutableString*)passwordText checkMsg:(NSString*)checkMsg checkValue:(BOOL*)checkValue doCheck:(BOOL)doCheck;

#if PROBABLY_DONT_WANT_THIS
- (BOOL)postToInsecureFromSecure:(NSWindow*)parent;

- (BOOL)badCert:(NSWindow*)parent;
- (BOOL)expiredCert:(NSWindow*)parent;
- (int)unknownCert:(NSWindow*)parent;
#endif

@end
