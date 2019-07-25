






































#import <Cocoa/Cocoa.h>

#include "nsIAccessible.h"

static const NSString* AXRoles [] = {
  NSAccessibilityUnknownRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityMenuBarRole,                   
  NSAccessibilityScrollBarRole,                 
  NSAccessibilitySplitterRole,                  
  NSAccessibilityUnknownRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityWindowRole,                    
  NSAccessibilityWindowRole,                    
  NSAccessibilityScrollAreaRole,                
  NSAccessibilityMenuRole,                      
  NSAccessibilityMenuItemRole,                  
  @"AXHelpTag",                                 
  NSAccessibilityGroupRole,                     
  @"AXWebArea",                                 
  NSAccessibilityGroupRole,                     
  NSAccessibilityUnknownRole,                   
  NSAccessibilityWindowRole,                    
  NSAccessibilityUnknownRole,                   
  NSAccessibilityGroupRole,                     
  NSAccessibilityUnknownRole,                   
  NSAccessibilityToolbarRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityGroupRole,                     
  NSAccessibilityGroupRole,                     
  NSAccessibilityGroupRole,                     
  NSAccessibilityColumnRole,                    
  NSAccessibilityRowRole,                       
  NSAccessibilityGroupRole,                     
  @"AXLink",                                    
  @"AXHelpTag",                                 
  NSAccessibilityUnknownRole,                   
  NSAccessibilityListRole,                      
  NSAccessibilityRowRole,                       
  NSAccessibilityOutlineRole,                   
  NSAccessibilityRowRole,                       
  NSAccessibilityRadioButtonRole,               
  NSAccessibilityGroupRole,                     
  NSAccessibilityUnknownRole,                   
  NSAccessibilityImageRole,                     
  NSAccessibilityStaticTextRole,                
  NSAccessibilityStaticTextRole,                
  NSAccessibilityButtonRole,                    
  NSAccessibilityCheckBoxRole,                  
  NSAccessibilityRadioButtonRole,               
  NSAccessibilityPopUpButtonRole,               
  NSAccessibilityPopUpButtonRole,               
  NSAccessibilityProgressIndicatorRole,         
  NSAccessibilityUnknownRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilitySliderRole,                    
  NSAccessibilityIncrementorRole,               
  NSAccessibilityUnknownRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityPopUpButtonRole,               
  NSAccessibilityMenuButtonRole,                
  NSAccessibilityGroupRole,                     
  NSAccessibilityUnknownRole,                   
  NSAccessibilityTabGroupRole,                  
  NSAccessibilityUnknownRole,                   
  NSAccessibilityButtonRole,                    
  NSAccessibilityUnknownRole,                   
  NSAccessibilityStaticTextRole,                
  NSAccessibilityUnknownRole,                   
  NSAccessibilityImageRole,                     
  NSAccessibilityMenuItemRole,                  
  NSAccessibilityColorWellRole,                 
  NSAccessibilityUnknownRole,                   
  NSAccessibilityImageRole,                     
  NSAccessibilityUnknownRole,                   
  NSAccessibilityBrowserRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityGroupRole,                     
  NSAccessibilityUnknownRole,                   
  NSAccessibilityImageRole,                     
  NSAccessibilityStaticTextRole,                
  NSAccessibilityGroupRole,                     
  NSAccessibilityGroupRole,                     
  NSAccessibilityTextFieldRole,                 
  NSAccessibilityUnknownRole,                   
  NSAccessibilityMenuItemRole,                  
  NSAccessibilityGroupRole,                     
  NSAccessibilityScrollAreaRole,                
  NSAccessibilitySplitGroupRole,                
  NSAccessibilityUnknownRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityMenuItemRole,                  
  NSAccessibilityUnknownRole,                   
  NSAccessibilityGroupRole,                     
  NSAccessibilityButtonRole,                    
  NSAccessibilityTableRole,                     
  NSAccessibilityUnknownRole,                   
  NSAccessibilityGroupRole,                     
  NSAccessibilityGroupRole,                     
  NSAccessibilityGroupRole,                     
  @"AXRuler",                                   
  NSAccessibilityComboBoxRole,                  
  NSAccessibilityTextFieldRole,                 
  NSAccessibilityTextFieldRole,                 
  NSAccessibilityStaticTextRole,                
  NSAccessibilityScrollAreaRole,                
  @"AXHeading",                                 
  NSAccessibilityGroupRole,                     
  NSAccessibilityGroupRole,                     
  NSAccessibilityUnknownRole,                   
  NSAccessibilityGroupRole,                     
  NSAccessibilityUnknownRole,                   
  NSAccessibilityUnknownRole,                   
  NSAccessibilityMenuItemRole,                  
  NSAccessibilityGroupRole,                     
  NSAccessibilityMenuRole,                      
  NSAccessibilityMenuItemRole,                  
  NSAccessibilityImageRole,                     
  NSAccessibilityRowRole,                       
  NSAccessibilityRowRole,                       
  NSAccessibilityListRole,                      
  NSAccessibilityUnknownRole,                   
  NSAccessibilityGroupRole,                     
  NSAccessibilityGroupRole,                     
  NSAccessibilityGroupRole,                     
  NSAccessibilityGroupRole,                     
  @"ROLE_LAST_ENTRY"                            
};
