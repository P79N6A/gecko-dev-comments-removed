





#ifndef __NS_ACCESSIBLE_WRAP_H__
#define __NS_ACCESSIBLE_WRAP_H__

#include "nsCOMPtr.h"
#include "Accessible.h"

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
  int32_t type;     
  void *oldvalue;
  void *newvalue;
};

namespace mozilla {
namespace a11y {

class MaiHyperlink;





class AccessibleWrap : public Accessible
{
public:
  AccessibleWrap(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~AccessibleWrap();
  void ShutdownAtkObject();

  virtual void Shutdown() override;

  
  virtual void GetNativeInterface(void** aOutAccessible) override;
  virtual nsresult HandleAccEvent(AccEvent* aEvent) override;

  AtkObject * GetAtkObject(void);
  static AtkObject* GetAtkObject(Accessible* aAccessible);

  bool IsValidObject();

  static const char * ReturnString(nsAString &aString) {
    static nsCString returnedString;
    returnedString = NS_ConvertUTF16toUTF8(aString);
    return returnedString.get();
  }

protected:

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

  uint16_t CreateMaiInterfaces();
};

} 
} 

#endif
