




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

enum EPlatformDisabledState {
  ePlatformIsForceEnabled = -1,
  ePlatformIsEnabled = 0,
  ePlatformIsDisabled = 1
};




EPlatformDisabledState PlatformDisabledState();

#ifdef MOZ_ACCESSIBILITY_ATK





void PreInit();
#endif

#if defined(MOZ_ACCESSIBILITY_ATK) || defined(XP_MACOSX)




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

  
  virtual Accessible* GetRootDocumentAccessible(nsIPresShell* aPresShell,
                                                bool aCanCreate);
  already_AddRefed<Accessible>
    CreateHTMLButtonAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLBRAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLCanvasAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLCaptionAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLCheckboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLComboboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLFileInputAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLGroupboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLHRAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLImageAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLImageMapAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLLabelAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLLIAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLListboxAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLMediaAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLObjectFrameAccessible(nsObjectFrame* aFrame, nsIContent* aContent,
                                    nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLRadioButtonAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLTableAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLTableCellAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLTableRowAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateTextLeafAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHTMLTextFieldAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateHyperTextAccessible(nsIContent* aContent, nsIPresShell* aPresShell);
  already_AddRefed<Accessible>
    CreateOuterDocAccessible(nsIContent* aContent, nsIPresShell* aPresShell);

  virtual Accessible* AddNativeRootAccessible(void* aAtkAccessible);
  virtual void RemoveNativeRootAccessible(Accessible* aRootAccessible);

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

  virtual void FireAccessibleEvent(PRUint32 aEvent, Accessible* aTarget);

  

  


  static bool IsShutdown() { return gIsShutdown; }

  








  Accessible* GetOrCreateAccessible(nsINode* aNode, DocAccessible* aDoc,
                                    bool* aIsSubtreeHidden = nsnull);

private:
  
  
  nsAccessibilityService();
  nsAccessibilityService(const nsAccessibilityService&);
  nsAccessibilityService& operator =(const nsAccessibilityService&);

private:
  


  bool Init();

  


  void Shutdown();

  



  already_AddRefed<Accessible>
    CreateAccessibleByType(nsIContent* aContent, DocAccessible* aDoc);

  


  already_AddRefed<Accessible>
    CreateHTMLAccessibleByMarkup(nsIFrame* aFrame, nsIContent* aContent,
                                 DocAccessible* aDoc);

  


  already_AddRefed<Accessible>
    CreateAccessibleForDeckChild(nsIFrame* aFrame, nsIContent* aContent,
                                 DocAccessible* aDoc);

#ifdef MOZ_XUL
  


  already_AddRefed<Accessible>
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

