




#ifndef nsWidgetInitData_h__
#define nsWidgetInitData_h__







enum nsWindowType {
  eWindowType_toplevel,           
  eWindowType_dialog,             
                                  
  eWindowType_popup,              
  eWindowType_child,              
                                  
  eWindowType_invisible,          
  eWindowType_plugin,             
  eWindowType_plugin_ipc_chrome,  
  eWindowType_plugin_ipc_content, 
  eWindowType_sheet,              
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
      mScreenId(0),
      clipChildren(false),
      clipSiblings(false),
      mDropShadow(false),
      mListenForResizes(false),
      mUnicode(true),
      mRTL(false),
      mNoAutoHide(false),
      mIsDragPopup(false),
      mIsAnimationSuppressed(false),
      mSupportTranslucency(false),
      mMouseTransparent(false),
      mMultiProcessWindow(false)
  {
  }

  nsWindowType  mWindowType;
  nsBorderStyle mBorderStyle;
  nsPopupType   mPopupHint;
  nsPopupLevel  mPopupLevel;
  
  
  
  uint32_t      mScreenId;
  
  bool          clipChildren, clipSiblings, mDropShadow;
  bool          mListenForResizes;
  bool          mUnicode;
  bool          mRTL;
  bool          mNoAutoHide; 
  bool          mIsDragPopup;  
  
  bool          mIsAnimationSuppressed;
  
  bool          mSupportTranslucency;
  
  
  bool          mMouseTransparent;
  
  bool          mMultiProcessWindow;
};

#endif 
