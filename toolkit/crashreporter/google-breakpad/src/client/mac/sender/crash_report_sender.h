

































#include <Foundation/Foundation.h>

#include "client/mac/Framework/Breakpad.h"

#define kClientIdPreferenceKey @"clientid"

extern NSString *const kGoogleServerType;
extern NSString *const kSocorroServerType;
extern NSString *const kDefaultServerType;
@interface Reporter : NSObject {
 @public
  IBOutlet NSWindow *alertWindow_;        

  
  IBOutlet NSBox *headerBox_;
  IBOutlet NSBox *preEmailBox_;
  IBOutlet NSBox *emailSectionBox_;
  
  IBOutlet NSTextField *dialogTitle_;
  IBOutlet NSTextField *commentMessage_;
  IBOutlet NSTextField *emailMessage_;
  IBOutlet NSTextField *emailLabel_;
  IBOutlet NSTextField *privacyLinkLabel_;
  IBOutlet NSButton    *sendButton_;
  IBOutlet NSButton    *cancelButton_;
  IBOutlet NSView      *emailEntryField_;
  IBOutlet NSView      *privacyLinkArrow_;

  
  NSString *commentsValue_;                
  NSString *emailValue_;                   

 @private
  int configFile_;                         
  NSMutableDictionary *parameters_;        
  NSData *minidumpContents_;               
  NSData *logFileData_;                    
                                           
  NSMutableDictionary *serverDictionary_;  
                                           
                                           
                                           
  NSMutableDictionary *socorroDictionary_; 
                                           
  NSMutableDictionary *googleDictionary_;  
                                           
  NSMutableDictionary *extraServerVars_;   
                                           
                                           
                                           
                                           
}



- (IBAction)sendReport:(id)sender;


- (IBAction)cancel:(id)sender;

- (IBAction)showPrivacyPolicy:(id)sender;




- (BOOL)control:(NSControl *)control textView:(NSTextView *)textView
                          doCommandBySelector:(SEL)commandSelector;


- (NSString *)commentsValue;
- (void)setCommentsValue:(NSString *)value;

- (NSString *)emailValue;
- (void)setEmailValue:(NSString *)value;

@end
