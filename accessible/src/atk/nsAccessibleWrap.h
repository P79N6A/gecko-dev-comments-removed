








































#ifndef __NS_ACCESSIBLE_WRAP_H__
#define __NS_ACCESSIBLE_WRAP_H__

#include "nsCOMPtr.h"
#include "nsAccessible.h"
#include "prlog.h"

#ifdef PR_LOGGING
#define MAI_LOGGING
#endif 

struct _AtkObject;
typedef struct _AtkObject AtkObject;





class nsAccessibleWrap: public nsAccessible
{
public:
    nsAccessibleWrap(nsIDOMNode*, nsIWeakReference *aShell);
    virtual ~nsAccessibleWrap();

#ifdef MAI_LOGGING
    virtual void DumpnsAccessibleWrapInfo(int aDepth) {}
    static PRInt32 mAccWrapCreated;
    static PRInt32 mAccWrapDeleted;
#endif

public:
    
    NS_IMETHOD GetNativeInterface(void **aOutAccessible);
    NS_IMETHOD FireAccessibleEvent(nsIAccessibleEvent *aEvent);

    AtkObject * GetAtkObject(void);

    PRBool IsValidObject();

    static const char * ReturnString(nsAString &aString) {
      static nsCString returnedString;
      returnedString = NS_ConvertUTF16toUTF8(aString);
      return returnedString.get();
    }
    
protected:
    AtkObject *mAtkObject;

private:
    PRUint16 CreateMaiInterfaces(void);
};

#endif 
