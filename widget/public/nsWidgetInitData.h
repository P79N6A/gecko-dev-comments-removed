




































#ifndef nsWidgetInitData_h__
#define nsWidgetInitData_h__

#include "prtypes.h"







enum nsWindowType {
  eWindowType_toplevel,  
  eWindowType_dialog,    
                         
  eWindowType_popup,     
  eWindowType_child,     
                         
  eWindowType_invisible, 
  eWindowType_plugin,    
  eWindowType_sheet      
};






enum nsPopupType {
  ePopupTypePanel,
  ePopupTypeMenu,
  ePopupTypeTooltip,
  ePopupTypeAny      = 0xF000 
                              
};




enum nsPopupLevel {
  
  
  ePopupLevelParent,
  
  
  
  
  ePopupLevelFloating,
  
  ePopupLevelTop
};




enum nsBorderStyle {
  eBorderStyle_none     = 0,      
                                  
  eBorderStyle_all      = 1 << 0, 
  eBorderStyle_border   = 1 << 1, 
                                  
                                  
  eBorderStyle_resizeh  = 1 << 2, 
                                  
                                  
  eBorderStyle_title    = 1 << 3, 
  eBorderStyle_menu     = 1 << 4, 
                                  
                                  
  eBorderStyle_minimize = 1 << 5, 
                                  
                                  
                                  
  eBorderStyle_maximize = 1 << 6, 
                                  
  eBorderStyle_close    = 1 << 7, 
  eBorderStyle_default  = -1      
                                  
};






struct nsWidgetInitData {
  nsWidgetInitData() :
      mWindowType(eWindowType_child),
      mBorderStyle(eBorderStyle_default),
      mPopupHint(ePopupTypePanel),
      mPopupLevel(ePopupLevelTop),
      clipChildren(PR_FALSE), 
      clipSiblings(PR_FALSE), 
      mDropShadow(PR_FALSE),
      mListenForResizes(PR_FALSE),
      mUnicode(PR_TRUE),
      mRTL(PR_FALSE),
      mNoAutoHide(PR_FALSE),
      mIsDragPopup(PR_FALSE)
  {
  }

  nsWindowType  mWindowType;
  nsBorderStyle mBorderStyle;
  nsPopupType   mPopupHint;
  nsPopupLevel  mPopupLevel;
  
  bool          clipChildren, clipSiblings, mDropShadow;
  bool          mListenForResizes;
  bool          mUnicode;
  bool          mRTL;
  bool          mNoAutoHide; 
  bool          mIsDragPopup;  
};

#endif 
