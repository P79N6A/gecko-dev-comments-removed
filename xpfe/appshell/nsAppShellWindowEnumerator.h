




#ifndef nsAppShellWindowEnumerator_h
#define nsAppShellWindowEnumerator_h

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsISimpleEnumerator.h"
#include "nsIXULWindow.h"

class nsWindowMediator;





struct nsWindowInfo
{
  nsWindowInfo(nsIXULWindow* inWindow, int32_t inTimeStamp);
  ~nsWindowInfo();

  nsCOMPtr<nsIXULWindow>    mWindow;
  int32_t                   mTimeStamp;
  uint32_t                  mZLevel;

  
  nsWindowInfo              *mYounger, 
                            *mOlder;
  nsWindowInfo              *mLower,   
                            *mHigher;
  
  bool TypeEquals(const nsAString &aType);
  void   InsertAfter(nsWindowInfo *inOlder, nsWindowInfo *inHigher);
  void   Unlink(bool inAge, bool inZ);
  void   ReferenceSelf(bool inAge, bool inZ);
};





class nsAppShellWindowEnumerator : public nsISimpleEnumerator {

friend class nsWindowMediator;

public:
  nsAppShellWindowEnumerator(const char16_t* aTypeString,
                             nsWindowMediator& inMediator);
  NS_IMETHOD GetNext(nsISupports **retval) MOZ_OVERRIDE = 0;
  NS_IMETHOD HasMoreElements(bool *retval) MOZ_OVERRIDE;

  NS_DECL_ISUPPORTS

protected:

  virtual ~nsAppShellWindowEnumerator();

  void AdjustInitialPosition();
  virtual nsWindowInfo *FindNext() = 0;

  void WindowRemoved(nsWindowInfo *inInfo);

  nsWindowMediator *mWindowMediator;
  nsString          mType;
  nsWindowInfo     *mCurrentPosition;
};

class nsASDOMWindowEnumerator : public nsAppShellWindowEnumerator {

public:
  nsASDOMWindowEnumerator(const char16_t* aTypeString,
                          nsWindowMediator& inMediator);
  virtual ~nsASDOMWindowEnumerator();
  NS_IMETHOD GetNext(nsISupports **retval);
};

class nsASXULWindowEnumerator : public nsAppShellWindowEnumerator {

public:
  nsASXULWindowEnumerator(const char16_t* aTypeString,
                          nsWindowMediator& inMediator);
  virtual ~nsASXULWindowEnumerator();
  NS_IMETHOD GetNext(nsISupports **retval);
};





class nsASDOMWindowEarlyToLateEnumerator : public nsASDOMWindowEnumerator {

public:
  nsASDOMWindowEarlyToLateEnumerator(const char16_t* aTypeString,
                                     nsWindowMediator& inMediator);

  virtual ~nsASDOMWindowEarlyToLateEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASXULWindowEarlyToLateEnumerator : public nsASXULWindowEnumerator {

public:
  nsASXULWindowEarlyToLateEnumerator(const char16_t* aTypeString,
                                     nsWindowMediator& inMediator);

  virtual ~nsASXULWindowEarlyToLateEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASDOMWindowFrontToBackEnumerator : public nsASDOMWindowEnumerator {

public:
  nsASDOMWindowFrontToBackEnumerator(const char16_t* aTypeString,
                                     nsWindowMediator& inMediator);

  virtual ~nsASDOMWindowFrontToBackEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASXULWindowFrontToBackEnumerator : public nsASXULWindowEnumerator {

public:
  nsASXULWindowFrontToBackEnumerator(const char16_t* aTypeString,
                                     nsWindowMediator& inMediator);

  virtual ~nsASXULWindowFrontToBackEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASDOMWindowBackToFrontEnumerator : public nsASDOMWindowEnumerator {

public:
  nsASDOMWindowBackToFrontEnumerator(const char16_t* aTypeString,
                                     nsWindowMediator& inMediator);

  virtual ~nsASDOMWindowBackToFrontEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASXULWindowBackToFrontEnumerator : public nsASXULWindowEnumerator {

public:
  nsASXULWindowBackToFrontEnumerator(const char16_t* aTypeString,
                                     nsWindowMediator& inMediator);

  virtual ~nsASXULWindowBackToFrontEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

#endif
