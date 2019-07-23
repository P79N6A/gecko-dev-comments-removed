





































#ifndef __nsAccessibilityService_h__
#define __nsAccessibilityService_h__

#include "nsIAccessibilityService.h"
#include "nsCOMArray.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"

class nsIFrame;
class nsIWeakReference;
class nsIDOMNode;
class nsObjectFrame;
class nsIDocShell;
class nsIPresShell;
class nsIContent;
struct nsRoleMapEntry;

class nsAccessibilityService : public nsIAccessibilityService,
                               public nsIObserver,
                               public nsIWebProgressListener,
                               public nsSupportsWeakReference
{
public:
  nsAccessibilityService();
  virtual ~nsAccessibilityService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLERETRIEVAL
  NS_DECL_NSIACCESSIBILITYSERVICE
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIWEBPROGRESSLISTENER

  




  static nsresult GetShellFromNode(nsIDOMNode *aNode,
                                   nsIWeakReference **weakShell);

  


  static nsresult GetAccessibilityService(nsIAccessibilityService** aResult);

  


  static nsIAccessibilityService* GetAccessibilityService();

  


  static PRBool gIsShutdown;

private:
  







  nsresult GetInfo(nsIFrame *aFrame,
                   nsIWeakReference **aShell,
                   nsIDOMNode **aContent);

  









  nsresult InitAccessible(nsIAccessible *aAccessibleIn, nsIAccessible **aAccessibleOut,
                          nsRoleMapEntry *aRoleMapEntry = nsnull);

  





  nsresult GetAccessibleByType(nsIDOMNode *aNode, nsIAccessible **aAccessible);

  




  nsresult GetAccessibleForDeckChildren(nsIDOMNode *aNode,
                                        nsIAccessible **aAccessible);

#ifdef MOZ_XUL
  


  nsresult GetAccessibleForXULTree(nsIDOMNode *aNode,
                                   nsIWeakReference *aWeakShell,
                                   nsIAccessible **aAccessible);
#endif
  
  static nsAccessibilityService *gAccessibilityService;

  







  PRBool HasUniversalAriaProperty(nsIContent *aContent, nsIWeakReference *aWeakShell);

  static void StartLoadCallback(nsITimer *aTimer, void *aClosure);
  static void EndLoadCallback(nsITimer *aTimer, void *aClosure);
  static void FailedLoadCallback(nsITimer *aTimer, void *aClosure);
  nsCOMArray<nsITimer> mLoadTimers;
};





static const char kRoleNames[][20] = {
  "nothing",             
  "titlebar",            
  "menubar",             
  "scrollbar",           
  "grip",                
  "sound",               
  "cursor",              
  "caret",               
  "alert",               
  "window",              
  "internal frame",      
  "menupopup",           
  "menuitem",            
  "tooltip",             
  "application",         
  "document",            
  "pane",                
  "chart",               
  "dialog",              
  "border",              
  "grouping",            
  "separator",           
  "toolbar",             
  "statusbar",           
  "table",               
  "columnheader",        
  "rowheader",           
  "column",              
  "row",                 
  "cell",                
  "link",                
  "helpballoon",         
  "character",           
  "list",                
  "listitem",            
  "outline",             
  "outlineitem",         
  "pagetab",             
  "propertypage",        
  "indicator",           
  "graphic",             
  "statictext",          
  "text leaf",           
  "pushbutton",          
  "checkbutton",         
  "radiobutton",         
  "combobox",            
  "droplist",            
  "progressbar",         
  "dial",                
  "hotkeyfield",         
  "slider",              
  "spinbutton",          
  "diagram",             
  "animation",           
  "equation",            
  "buttondropdown",      
  "buttonmenu",          
  "buttondropdowngrid",  
  "whitespace",          
  "pagetablist",         
  "clock",               
  "splitbutton",         
  "ipaddress",           
  "accel label",         
  "arrow",               
  "canvas",              
  "check menu item",     
  "color chooser",       
  "date editor",         
  "desktop icon",        
  "desktop frame",       
  "directory pane",      
  "file chooser",        
  "font chooser",        
  "chrome window",       
  "glass pane",          
  "html container",      
  "icon",                
  "label",               
  "layered pane",        
  "option pane",         
  "password text",       
  "popup menu",          
  "radio menu item",     
  "root pane",           
  "scroll pane",         
  "split pane",          
  "table column header", 
  "table row header",    
  "tear off menu item",  
  "terminal",            
  "text container",      
  "toggle button",       
  "tree table",          
  "viewport",            
  "header",              
  "footer",              
  "paragraph",           
  "ruler",               
  "autocomplete",        
  "editbar",             
  "entry",               
  "caption",             
  "document frame",      
  "heading",             
  "page",                
  "section",             
  "redundant object",    
  "form",                
  "ime",                 
  "app root",            
  "parent menuitem",     
  "calendar",            
  "combobox list",       
  "combobox option",     
  "image map",           
  "listbox option",      
  "listbox rich option", 
  "listbox",             
  "flat equation",       
  "gridcell"             
};





static const char kEventTypeNames[][40] = {
  "unknown",                                 
  "DOM node create",                         
  "DOM node destroy",                        
  "DOM node significant change",             
  "async show",                              
  "async hide",                              
  "async significant change",                
  "active decendent change",                 
  "focus",                                   
  "state change",                            
  "location change",                         
  "name changed",                            
  "description change",                      
  "value change",                            
  "help change",                             
  "default action change",                   
  "action change",                           
  "accelerator change",                      
  "selection",                               
  "selection add",                           
  "selection remove",                        
  "selection within",                        
  "alert",                                   
  "foreground",                              
  "menu start",                              
  "menu end",                                
  "menupopup start",                         
  "menupopup end",                           
  "capture start",                           
  "capture end",                             
  "movesize start",                          
  "movesize end",                            
  "contexthelp start",                       
  "contexthelp end",                         
  "dragdrop start",                          
  "dragdrop end",                            
  "dialog start",                            
  "dialog end",                              
  "scrolling start",                         
  "scrolling end",                           
  "minimize start",                          
  "minimize end",                            
  "document load start",                     
  "document load complete",                  
  "document reload",                         
  "document load stopped",                   
  "document attributes changed",             
  "document content changed",                
  "property changed",                        
  "selection changed",                       
  "text attribute changed",                  
  "text caret moved",                        
  "text changed",                            
  "text inserted",                           
  "text removed",                            
  "text updated",                            
  "text selection changed",                  
  "visible data changed",                    
  "text column changed",                     
  "section changed",                         
  "table caption changed",                   
  "table model changed",                     
  "table summary changed",                   
  "table row description changed",           
  "table row header changed",                
  "table row insert",                        
  "table row delete",                        
  "table row reorder",                       
  "table column description changed",        
  "table column header changed",             
  "table column insert",                     
  "table column delete",                     
  "table column reorder",                    
  "window activate",                         
  "window create",                           
  "window deactivate",                       
  "window destroy",                          
  "window maximize",                         
  "window minimize",                         
  "window resize",                           
  "window restore",                          
  "hyperlink end index changed",             
  "hyperlink number of anchors changed",     
  "hyperlink selected link changed",         
  "hypertext link activated",                
  "hypertext link selected",                 
  "hyperlink start index changed",           
  "hypertext changed",                       
  "hypertext links count changed",           
  "object attribute changed",                
  "page changed",                            
  "internal load",                           
  "reorder"                                  
};





static const char kRelationTypeNames[][20] = {
  "unknown",             
  "controlled by",       
  "controller for",      
  "label for",           
  "labelled by",         
  "member of",           
  "node child of",       
  "flows to",            
  "flows from",          
  "subwindow of",        
  "embeds",              
  "embedded by",         
  "popup for",           
  "parent window of",    
  "described by",        
  "description for",     
  "default button"       
};

#endif 

