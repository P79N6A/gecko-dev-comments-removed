




































#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsISimpleEnumerator.h"
#include "nsIXULWindow.h"

class nsWindowMediator;





struct nsWindowInfo
{
  nsWindowInfo(nsIXULWindow* inWindow, PRInt32 inTimeStamp);
  ~nsWindowInfo();

  nsCOMPtr<nsIXULWindow>    mWindow;
  PRInt32                   mTimeStamp;
  PRUint32                  mZLevel;

  
  nsWindowInfo              *mYounger, 
                            *mOlder;
  nsWindowInfo              *mLower,   
                            *mHigher;
  
  PRBool TypeEquals(const nsAString &aType);
  void   InsertAfter(nsWindowInfo *inOlder, nsWindowInfo *inHigher);
  void   Unlink(PRBool inAge, PRBool inZ);
  void   ReferenceSelf(PRBool inAge, PRBool inZ);
};






class nsAppShellWindowEnumerator : public nsISimpleEnumerator {

friend class nsWindowMediator;

public:
  nsAppShellWindowEnumerator (const PRUnichar* aTypeString,
                      nsWindowMediator& inMediator);
  virtual ~nsAppShellWindowEnumerator();
  NS_IMETHOD GetNext(nsISupports **retval) = 0;
  NS_IMETHOD HasMoreElements(PRBool *retval);

  NS_DECL_ISUPPORTS

protected:

  void AdjustInitialPosition();
  virtual nsWindowInfo *FindNext() = 0;

  void WindowRemoved(nsWindowInfo *inInfo);

  nsWindowMediator *mWindowMediator;
  nsString          mType;
  nsWindowInfo     *mCurrentPosition;
};

class nsASDOMWindowEnumerator : public nsAppShellWindowEnumerator {

public:
  nsASDOMWindowEnumerator (const PRUnichar* aTypeString,
                           nsWindowMediator& inMediator);
  virtual ~nsASDOMWindowEnumerator();
  NS_IMETHOD GetNext(nsISupports **retval);
};

class nsASXULWindowEnumerator : public nsAppShellWindowEnumerator {

public:
  nsASXULWindowEnumerator (const PRUnichar* aTypeString,
                           nsWindowMediator& inMediator);
  virtual ~nsASXULWindowEnumerator();
  NS_IMETHOD GetNext(nsISupports **retval);
};





class nsASDOMWindowEarlyToLateEnumerator : public nsASDOMWindowEnumerator {

public:
  nsASDOMWindowEarlyToLateEnumerator(
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASDOMWindowEarlyToLateEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASXULWindowEarlyToLateEnumerator : public nsASXULWindowEnumerator {

public:
  nsASXULWindowEarlyToLateEnumerator(
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASXULWindowEarlyToLateEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASDOMWindowFrontToBackEnumerator : public nsASDOMWindowEnumerator {

public:
  nsASDOMWindowFrontToBackEnumerator(
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASDOMWindowFrontToBackEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASXULWindowFrontToBackEnumerator : public nsASXULWindowEnumerator {

public:
  nsASXULWindowFrontToBackEnumerator(
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASXULWindowFrontToBackEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASDOMWindowBackToFrontEnumerator : public nsASDOMWindowEnumerator {

public:
  nsASDOMWindowBackToFrontEnumerator (
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASDOMWindowBackToFrontEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};

class nsASXULWindowBackToFrontEnumerator : public nsASXULWindowEnumerator {

public:
  nsASXULWindowBackToFrontEnumerator (
             const PRUnichar* aTypeString,
             nsWindowMediator& inMediator);

  virtual ~nsASXULWindowBackToFrontEnumerator();

protected:
  virtual nsWindowInfo *FindNext();
};


