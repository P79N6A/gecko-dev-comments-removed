







































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

enum AtkProperty {
  PROP_0,           
  PROP_NAME,
  PROP_DESCRIPTION,
  PROP_PARENT,      
  PROP_ROLE,
  PROP_LAYER,
  PROP_MDI_ZORDER,
  PROP_TABLE_CAPTION,
  PROP_TABLE_COLUMN_DESCRIPTION,
  PROP_TABLE_COLUMN_HEADER,
  PROP_TABLE_ROW_DESCRIPTION,
  PROP_TABLE_ROW_HEADER,
  PROP_TABLE_SUMMARY,
  PROP_LAST         
};

struct AtkPropertyChange {
  PRInt32 type;     
  void *oldvalue;
  void *newvalue;
};

class MaiHyperlink;





class nsAccessibleWrap: public nsAccessible
{
public:
    nsAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell);
    virtual ~nsAccessibleWrap();
    void ShutdownAtkObject();

    
    virtual void Shutdown();

#ifdef MAI_LOGGING
    virtual void DumpnsAccessibleWrapInfo(int aDepth) {}
    static PRInt32 mAccWrapCreated;
    static PRInt32 mAccWrapDeleted;
#endif

    
    NS_IMETHOD GetNativeInterface(void **aOutAccessible);
    virtual nsresult HandleAccEvent(AccEvent* aEvent);

    AtkObject * GetAtkObject(void);
    static AtkObject * GetAtkObject(nsIAccessible * acc);

    bool IsValidObject();
    
    
    MaiHyperlink* GetMaiHyperlink(bool aCreate = true);
    void SetMaiHyperlink(MaiHyperlink* aMaiHyperlink);

    static const char * ReturnString(nsAString &aString) {
      static nsCString returnedString;
      returnedString = NS_ConvertUTF16toUTF8(aString);
      return returnedString.get();
    }

protected:
    virtual nsresult FirePlatformEvent(AccEvent* aEvent);

    nsresult FireAtkStateChangeEvent(AccEvent* aEvent, AtkObject *aObject);
    nsresult FireAtkTextChangedEvent(AccEvent* aEvent, AtkObject *aObject);
    nsresult FireAtkShowHideEvent(AccEvent* aEvent, AtkObject *aObject,
                                  bool aIsAdded);

    AtkObject *mAtkObject;

private:

  




  enum EAvailableAtkSignals {
    eUnknown,
    eHaveNewAtkTextSignals,
    eNoNewAtkSignals
  };

  static EAvailableAtkSignals gAvailableAtkSignals;

    PRUint16 CreateMaiInterfaces(void);
};

#endif
