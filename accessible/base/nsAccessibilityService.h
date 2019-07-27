




#ifndef __nsAccessibilityService_h__
#define __nsAccessibilityService_h__

#include "nsIAccessibilityService.h"

#include "mozilla/a11y/DocManager.h"
#include "mozilla/a11y/FocusManager.h"
#include "mozilla/a11y/Role.h"
#include "mozilla/a11y/SelectionManager.h"
#include "mozilla/Preferences.h"

#include "nsIObserver.h"

class nsImageFrame;
class nsIPersistentProperties;
class nsPluginFrame;
class nsITreeView;

namespace mozilla {
namespace a11y {

class ApplicationAccessible;
class xpcAccessibleApplication;




FocusManager* FocusMgr();




SelectionManager* SelectionMgr();




ApplicationAccessible* ApplicationAcc();
xpcAccessibleApplication* XPCApplicationAcc();

typedef Accessible* (New_Accessible)(nsIContent* aContent, Accessible* aContext);

struct MarkupAttrInfo {
  nsIAtom** name;
  nsIAtom** value;

  nsIAtom** DOMAttrName;
  nsIAtom** DOMAttrValue;
};

struct MarkupMapInfo {
  nsIAtom** tag;
  New_Accessible* new_func;
  a11y::role role;
  MarkupAttrInfo attrs[4];
};

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
                                                bool aCanCreate) MOZ_OVERRIDE;
  already_AddRefed<Accessible>
    CreatePluginAccessible(nsPluginFrame* aFrame, nsIContent* aContent,
                           Accessible* aContext);

  



  virtual Accessible* AddNativeRootAccessible(void* aAtkAccessible) MOZ_OVERRIDE;
  virtual void RemoveNativeRootAccessible(Accessible* aRootAccessible) MOZ_OVERRIDE;

  virtual bool HasAccessible(nsIDOMNode* aDOMNode) MOZ_OVERRIDE;

  
  



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

  virtual void FireAccessibleEvent(uint32_t aEvent, Accessible* aTarget) MOZ_OVERRIDE;

  

  


  static bool IsShutdown() { return gIsShutdown; }

  








  Accessible* GetOrCreateAccessible(nsINode* aNode, Accessible* aContext,
                                    bool* aIsSubtreeHidden = nullptr);

  mozilla::a11y::role MarkupRole(const nsIContent* aContent) const
  {
    const mozilla::a11y::MarkupMapInfo* markupMap =
      mMarkupMaps.Get(aContent->NodeInfo()->NameAtom());
    return markupMap ? markupMap->role : mozilla::a11y::roles::NOTHING;
  }

  


  void MarkupAttributes(const nsIContent* aContent,
                        nsIPersistentProperties* aAttributes) const;

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
    CreateAccessibleByFrameType(nsIFrame* aFrame, nsIContent* aContent,
                                Accessible* aContext);

#ifdef MOZ_XUL
  


  already_AddRefed<Accessible>
    CreateAccessibleForXULTree(nsIContent* aContent, DocAccessible* aDoc);
#endif

  


  static nsAccessibilityService* gAccessibilityService;

  


  static mozilla::a11y::ApplicationAccessible* gApplicationAccessible;
  static mozilla::a11y::xpcAccessibleApplication* gXPCApplicationAccessible;

  


  static bool gIsShutdown;

  nsDataHashtable<nsPtrHashKey<const nsIAtom>, const mozilla::a11y::MarkupMapInfo*> mMarkupMaps;

  friend nsAccessibilityService* GetAccService();
  friend mozilla::a11y::FocusManager* mozilla::a11y::FocusMgr();
  friend mozilla::a11y::SelectionManager* mozilla::a11y::SelectionMgr();
  friend mozilla::a11y::ApplicationAccessible* mozilla::a11y::ApplicationAcc();
  friend mozilla::a11y::xpcAccessibleApplication* mozilla::a11y::XPCApplicationAcc();

  friend nsresult NS_GetAccessibilityService(nsIAccessibilityService** aResult);
};




inline nsAccessibilityService*
GetAccService()
{
  return nsAccessibilityService::gAccessibilityService;
}




inline bool
IPCAccessibilityActive()
{
	
	return false;
#ifdef MOZ_B2G
  return false;
#else
  return XRE_GetProcessType() == GeckoProcessType_Content &&
    mozilla::Preferences::GetBool("accessibility.ipc_architecture.enabled", true);
#endif
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

