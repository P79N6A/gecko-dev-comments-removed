





































#ifndef __nsAccessibilityService_h__
#define __nsAccessibilityService_h__

#include "nsIAccessibilityService.h"

#include "a11yGeneric.h"
#include "nsCoreUtils.h"

#include "nsCOMArray.h"
#include "nsIObserver.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"

class nsAccessNode;
class nsAccessible;
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
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIWEBPROGRESSLISTENER

  
  virtual nsAccessible* GetAccessibleInShell(nsIDOMNode *aNode,
                                             nsIPresShell *aPresShell);

  virtual nsresult CreateOuterDocAccessible(nsIDOMNode *aNode,
                                            nsIAccessible **aAccessible);
  virtual nsresult CreateHTML4ButtonAccessible(nsIFrame *aFrame,
                                               nsIAccessible **aAccessible);
  virtual nsresult CreateHyperTextAccessible(nsIFrame *aFrame,
                                             nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLBRAccessible(nsIFrame *aFrame,
                                          nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLButtonAccessible(nsIFrame *aFrame,
                                              nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLLIAccessible(nsIFrame *aFrame,
                                          nsIFrame *aBulletFrame,
                                          const nsAString& aBulletText,
                                          nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLCheckboxAccessible(nsIFrame *aFrame,
                                                nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLComboboxAccessible(nsIDOMNode *aNode,
                                                nsIWeakReference *aPresShell,
                                                nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLGenericAccessible(nsIFrame *aFrame,
                                               nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLGroupboxAccessible(nsIFrame *aFrame,
                                                nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLHRAccessible(nsIFrame *aFrame,
                                          nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLImageAccessible(nsIFrame *aFrame,
                                             nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLLabelAccessible(nsIFrame *aFrame,
                                             nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLListboxAccessible(nsIDOMNode *aNode,
                                               nsIWeakReference *aPresShell,
                                               nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLMediaAccessible(nsIFrame *aFrame,
                                             nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLObjectFrameAccessible(nsObjectFrame *aFrame,
                                                   nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLRadioButtonAccessible(nsIFrame *aFrame,
                                                   nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLSelectOptionAccessible(nsIDOMNode *aNode,
                                                    nsIAccessible *aAccParent,
                                                    nsIWeakReference *aPresShell,
                                                    nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLTableAccessible(nsIFrame *aFrame,
                                             nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLTableCellAccessible(nsIFrame *aFrame,
                                                 nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLTextAccessible(nsIFrame *aFrame,
                                            nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLTextFieldAccessible(nsIFrame *aFrame,
                                                 nsIAccessible **aAccessible);
  virtual nsresult CreateHTMLCaptionAccessible(nsIFrame *aFrame,
                                               nsIAccessible **aAccessible);

  virtual nsresult AddNativeRootAccessible(void *aAtkAccessible,
                                           nsIAccessible **aAccessible);
  virtual nsresult RemoveNativeRootAccessible(nsIAccessible *aRootAccessible);

  virtual nsresult InvalidateSubtreeFor(nsIPresShell *aPresShell,
                                        nsIContent *aContent,
                                        PRUint32 aChangeType);

  virtual void NotifyOfAnchorJumpTo(nsIContent *aTarget);

  virtual nsresult FireAccessibleEvent(PRUint32 aEvent, nsIAccessible *aTarget);

  

  




  static nsresult GetShellFromNode(nsIDOMNode *aNode,
                                   nsIWeakReference **weakShell);

  


  static PRBool gIsShutdown;

  








  already_AddRefed<nsAccessible>
    GetAccessible(nsIDOMNode *aNode, nsIPresShell *aPresShell,
                  nsIWeakReference *aWeakShell, PRBool *aIsHidden = nsnull);

  





  already_AddRefed<nsAccessible>
    GetAccessibleInWeakShell(nsIDOMNode *aNode, nsIWeakReference *aPresShell);

  







  nsAccessNode* GetCachedAccessNode(nsIDOMNode *aNode,
                                    nsIWeakReference *aShell);

private:
  







  nsresult GetInfo(nsIFrame *aFrame,
                   nsIWeakReference **aShell,
                   nsIDOMNode **aContent);

  









  PRBool InitAccessible(nsAccessible *aAccessible,
                        nsRoleMapEntry *aRoleMapEntry);

  


  already_AddRefed<nsAccessible>
    GetAreaAccessible(nsIFrame *aImageFrame, nsIDOMNode *aAreaNode,
                      nsIWeakReference *aWeakShell);

  



  already_AddRefed<nsAccessible>
    CreateAccessibleByType(nsIDOMNode *aNode, nsIWeakReference *aWeakShell);

  


  already_AddRefed<nsAccessible>
    CreateDocOrRootAccessible(nsIPresShell *aShell, nsIDocument *aDocument);

  


  already_AddRefed<nsAccessible>
    CreateHTMLAccessibleByMarkup(nsIFrame *aFrame, nsIWeakReference *aWeakShell,
                                 nsIDOMNode *aNode);

  


  already_AddRefed<nsAccessible>
    CreateAccessibleForDeckChild(nsIFrame *aFrame, nsIDOMNode *aNode,
                                 nsIWeakReference *aWeakShell);

#ifdef MOZ_XUL
  


  already_AddRefed<nsAccessible>
    CreateAccessibleForXULTree(nsIDOMNode *aNode, nsIWeakReference *aWeakShell);
#endif
  
  static nsAccessibilityService *gAccessibilityService;

  






  PRBool HasUniversalAriaProperty(nsIContent *aContent);

  








  void ProcessDocLoadEvent(nsIWebProgress *aWebProgress, PRUint32 aEventType);

  friend nsAccessibilityService* GetAccService();

  friend nsresult  NS_GetAccessibilityService(nsIAccessibilityService** aResult);

  
  NS_DECL_RUNNABLEMETHOD_ARG2(nsAccessibilityService, ProcessDocLoadEvent,
                              nsCOMPtr<nsIWebProgress>, PRUint32)
};




inline nsAccessibilityService*
GetAccService()
{
  return nsAccessibilityService::gAccessibilityService;
}





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
  "gridcell",            
  "embedded object"      
};





static const char kEventTypeNames[][40] = {
  "unknown",                                 
  "show",                                    
  "hide",                                    
  "reorder",                                 
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
  "internal load"                            
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

