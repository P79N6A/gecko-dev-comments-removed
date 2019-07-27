




#ifndef __nsAccessibilityService_h__
#define __nsAccessibilityService_h__

#include "nsIAccessibilityService.h"

#include "mozilla/a11y/DocManager.h"
#include "mozilla/a11y/FocusManager.h"
#include "mozilla/a11y/SelectionManager.h"

#include "nsIObserver.h"

class nsImageFrame;
class nsObjectFrame;
class nsITreeView;

namespace mozilla {
namespace a11y {

class ApplicationAccessible;




FocusManager* FocusMgr();




SelectionManager* SelectionMgr();




ApplicationAccessible* ApplicationAcc();

} 
} 

class nsAccessibilityService MOZ_FINAL : public mozilla::a11y::DocManager,
                                         public mozilla::a11y::FocusManager,
                                         public mozilla::a11y::SelectionManager,
                                         public nsIAccessibilityService,
                                         public nsIObserver
{
public:
  typedef mozilla::a11y::Accessible Accessible;
  typedef mozilla::a11y::DocAccessible DocAccessible;

protected:
  virtual ~nsAccessibilityService();

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLERETRIEVAL
  NS_DECL_NSIOBSERVER

  
  virtual Accessible* GetRootDocumentAccessible(nsIPresShell* aPresShell,
                                                bool aCanCreate);
  already_AddRefed<Accessible>
    CreatePluginAccessible(nsObjectFrame* aFrame, nsIContent* aContent,
                           Accessible* aContext);

  



  virtual Accessible* AddNativeRootAccessible(void* aAtkAccessible);
  virtual void RemoveNativeRootAccessible(Accessible* aRootAccessible);

  



  void DeckPanelSwitched(nsIPresShell* aPresShell, nsIContent* aDeckNode,
                         nsIFrame* aPrevBoxFrame, nsIFrame* aCurrentBoxFrame);

  



  void ContentRangeInserted(nsIPresShell* aPresShell, nsIContent* aContainer,
                            nsIContent* aStartChild, nsIContent* aEndChild);

  


  void ContentRemoved(nsIPresShell* aPresShell, nsIContent* aChild);

  virtual void UpdateText(nsIPresShell* aPresShell, nsIContent* aContent);

  


  void TreeViewChanged(nsIPresShell* aPresShell, nsIContent* aContent,
                       nsITreeView* aView);

  


  void RangeValueChanged(nsIPresShell* aPresShell, nsIContent* aContent);

  


  virtual void UpdateListBullet(nsIPresShell* aPresShell,
                                nsIContent* aHTMLListItemContent,
                                bool aHasBullet);

  


  void UpdateImageMap(nsImageFrame* aImageFrame);

  


  void UpdateLabelValue(nsIPresShell* aPresShell, nsIContent* aLabelElm,
                        const nsString& aNewValue);

  



  void NotifyOfAnchorJumpTo(nsIContent *aTarget);

  


  virtual void PresShellActivated(nsIPresShell* aPresShell);

  


  void RecreateAccessible(nsIPresShell* aPresShell, nsIContent* aContent);

  virtual void FireAccessibleEvent(uint32_t aEvent, Accessible* aTarget);

  

  


  static bool IsShutdown() { return gIsShutdown; }

  








  Accessible* GetOrCreateAccessible(nsINode* aNode, Accessible* aContext,
                                    bool* aIsSubtreeHidden = nullptr);

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
                                 Accessible* aContext);

  


  already_AddRefed<Accessible>
    CreateAccessibleByFrameType(nsIFrame* aFrame, nsIContent* aContent,
                                Accessible* aContext);

#ifdef MOZ_XUL
  


  already_AddRefed<Accessible>
    CreateAccessibleForXULTree(nsIContent* aContent, DocAccessible* aDoc);
#endif

  


  static nsAccessibilityService* gAccessibilityService;

  


  static mozilla::a11y::ApplicationAccessible* gApplicationAccessible;

  


  static bool gIsShutdown;

  friend nsAccessibilityService* GetAccService();
  friend mozilla::a11y::FocusManager* mozilla::a11y::FocusMgr();
  friend mozilla::a11y::SelectionManager* mozilla::a11y::SelectionMgr();
  friend mozilla::a11y::ApplicationAccessible* mozilla::a11y::ApplicationAcc();

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

#endif 

