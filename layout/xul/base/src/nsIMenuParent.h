





































#ifndef nsIMenuParent_h___
#define nsIMenuParent_h___

class nsMenuFrame;









class nsIMenuParent {

public:
  
  virtual nsMenuFrame *GetCurrentMenuItem() = 0;
  
  NS_IMETHOD SetCurrentMenuItem(nsMenuFrame* aMenuItem) = 0;
  
  
  virtual void CurrentMenuIsBeingDestroyed() = 0;
  
  
  
  
  
  NS_IMETHOD ChangeMenuItem(nsMenuFrame* aMenuItem, PRBool aSelectFirstItem) = 0;

  
  virtual PRBool IsOpen() = 0;
  
  virtual PRBool IsActive() = 0;
  
  virtual PRBool IsMenuBar() = 0;
  
  
  virtual PRBool IsMenu() = 0;
  
  virtual PRBool IsContextMenu() = 0;

  
  NS_IMETHOD SetActive(PRBool aActiveFlag) = 0;

  
  
  
  virtual PRBool MenuClosed() = 0;
};

#endif

