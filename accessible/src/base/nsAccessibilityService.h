




#ifndef __nsAccessibilityService_h__
#define __nsAccessibilityService_h__

#include "nsIAccessibilityService.h"

#include "a11yGeneric.h"
#include "nsAccDocManager.h"

#include "mozilla/a11y/FocusManager.h"

#include "nsIObserver.h"

class nsImageFrame;
class nsITreeView;

namespace mozilla {
namespace a11y {




FocusManager* FocusMgr();

#ifdef MOZ_ACCESSIBILITY_ATK





void PreInit();





bool ShouldA11yBeEnabled();
#endif

} 
} 

class nsAccessibilityService : public nsAccDocManager,
                               public mozilla::a11y::FocusManager,
                               public nsIAccessibilityService,
                               public nsIObserver
{
public:
  virtual ~nsAccessibilityService();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLERETRIEVAL
  NS_DECL_NSIOBSERVER

  
  virtual nsAccessible* GetRootDocumentAccessible(nsIPresShell* aPresShell,
                                                  bool aCanCreate);
  already_AddRefed<nsAccessible>
    CreateHTMLButtonAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLBRAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLCanvasAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLCaptionAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLCheckboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLComboboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLFileInputAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLGroupboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLHRAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLImageAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLImageMapAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLLabelAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLLIAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLListboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLMediaAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLObjectFrameAccessible(nsObjectFrame* aFrame, nsIContent* aContent,
                                    nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLRadioButtonAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLTableAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLTableCellAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLTableRowAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateTextLeafAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHTMLTextFieldAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateHyperTextAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<nsAccessible>
    CreateOuterDocAccessible(nsIContent* aContent, nsIPresShell* aPresShell);

  virtual nsAccessible* AddNativeRootAccessible(void* aAtkAccessible);
  virtual void RemoveNativeRootAccessible(nsAccessible* aRootAccessible);

  virtual void ContentRangeInserted(nsIPresShell* aPresShell,
                                    nsIContent* aContainer,
                                    nsIContent* aStartChild,
                                    nsIContent* aEndChild);

  virtual void ContentRemoved(nsIPresShell* aPresShell, nsIContent* aContainer,
                              nsIContent* aChild);

  virtual void UpdateText(nsIPresShell* aPresShell, nsIContent* aContent);

  


  void TreeViewChanged(nsIPresShell* aPresShell, nsIContent* aContent,
                       nsITreeView* aView);

  


  virtual void UpdateListBullet(nsIPresShell* aPresShell,
                                nsIContent* aHTMLListItemContent,
                                bool aHasBullet);

  


  void UpdateImageMap(nsImageFrame* aImageFrame);

  virtual void NotifyOfAnchorJumpTo(nsIContent *aTarget);

  virtual void PresShellDestroyed(nsIPresShell* aPresShell);

  


  virtual void PresShellActivated(nsIPresShell* aPresShell);

  


  void RecreateAccessible(nsIPresShell* aPresShell, nsIContent* aContent);

  virtual void FireAccessibleEvent(PRUint32 aEvent, nsAccessible* aTarget);

  

  


  static bool IsShutdown() { return gIsShutdown; }

  








  nsAccessible* GetOrCreateAccessible(nsINode* aNode, DocAccessible* aDoc,
                                      bool* aIsSubtreeHidden = nsnull);

  



  nsAccessible* GetAccessible(nsINode* aNode, nsIPresShell* aPresShell);

private:
  
  
  nsAccessibilityService();
  nsAccessibilityService(const nsAccessibilityService&);
  nsAccessibilityService& operator =(const nsAccessibilityService&);

private:
  


  bool Init();

  


  void Shutdown();

  



  already_AddRefed<nsAccessible>
    CreateAccessibleByType(nsIContent* aContent, DocAccessible* aDoc);

  


  already_AddRefed<nsAccessible>
    CreateHTMLAccessibleByMarkup(nsIFrame* aFrame, nsIContent* aContent,
                                 DocAccessible* aDoc);

  


  already_AddRefed<nsAccessible>
    CreateAccessibleForDeckChild(nsIFrame* aFrame, nsIContent* aContent,
                                 DocAccessible* aDoc);

#ifdef MOZ_XUL
  


  already_AddRefed<nsAccessible>
    CreateAccessibleForXULTree(nsIContent* aContent, DocAccessible* aDoc);
#endif

  


  static nsAccessibilityService* gAccessibilityService;

  


  static bool gIsShutdown;

  






  bool HasUniversalAriaProperty(nsIContent *aContent);

  friend nsAccessibilityService* GetAccService();
  friend mozilla::a11y::FocusManager* mozilla::a11y::FocusMgr();

  friend nsresult NS_GetAccessibilityService(nsIAccessibilityService** aResult);
};




inline nsAccessibilityService*
GetAccService()
{
  return nsAccessibilityService::gAccessibilityService;
}





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
  "document load complete",                  
  "document reload",                         
  "document load stopped",                   
  "document attributes changed",             
  "document content changed",                
  "property changed",                        
  "page changed",                           
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
  "virtual cursor changed"                   
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

