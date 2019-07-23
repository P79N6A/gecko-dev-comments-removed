





































#ifndef __nsAccessibilityService_h__
#define __nsAccessibilityService_h__

#include "nsIAccessibilityService.h"
#include "nsIObserver.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"

class nsIFrame;
class nsIWeakReference;
class nsIDOMNode;
class nsObjectFrame;
class nsIDocShell;
class nsIPresShell;
class nsIContent;

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
  "client",              
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
  "combobox listitem",   
  "image map"            
};

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

  static nsresult GetShellFromNode(nsIDOMNode *aNode, nsIWeakReference **weakShell);
  static nsresult GetAccessibilityService(nsIAccessibilityService** aResult);

private:
  nsresult GetInfo(nsISupports* aFrame, nsIFrame** aRealFrame, nsIWeakReference** aShell, nsIDOMNode** aContent);
  void GetOwnerFor(nsIPresShell *aPresShell, nsIPresShell **aOwnerShell, nsIContent **aOwnerContent);
  nsIContent* FindContentForDocShell(nsIPresShell* aPresShell, nsIContent* aContent, nsIDocShell*  aDocShell);
  static nsAccessibilityService *gAccessibilityService;
  nsresult InitAccessible(nsIAccessible *aAccessibleIn, nsIAccessible **aAccessibleOut);

  



  nsresult GetAccessibleByType(nsIDOMNode *aNode, nsIAccessible **aAccessible);
  PRBool HasListener(nsIContent *aContent, nsAString& aEventType);

  


  nsresult GetAccessibleForDeckChildren(nsIDOMNode *aNode, nsIAccessible **aAccessible);
};

#endif 
