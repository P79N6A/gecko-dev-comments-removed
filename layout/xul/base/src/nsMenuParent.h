





































#ifndef nsMenuParent_h___
#define nsMenuParent_h___

class nsMenuFrame;









class nsMenuParent {

public:
  
  virtual nsMenuFrame *GetCurrentMenuItem() = 0;
  
  NS_IMETHOD SetCurrentMenuItem(nsMenuFrame* aMenuItem) = 0;
  
  
  virtual void CurrentMenuIsBeingDestroyed() = 0;
  
  
  
  
  
  NS_IMETHOD ChangeMenuItem(nsMenuFrame* aMenuItem, bool aSelectFirstItem) = 0;

  
  virtual bool IsOpen() = 0;
  
  virtual bool IsActive() = 0;
  
  virtual bool IsMenuBar() = 0;
  
  
  virtual bool IsMenu() = 0;
  
  virtual bool IsContextMenu() = 0;

  
  NS_IMETHOD SetActive(bool aActiveFlag) = 0;

  
  
  
  virtual bool MenuClosed() = 0;

  
  
  
  
  
  virtual void LockMenuUntilClosed(bool aLock) = 0;
  virtual bool IsMenuLocked() = 0;
};

#endif

