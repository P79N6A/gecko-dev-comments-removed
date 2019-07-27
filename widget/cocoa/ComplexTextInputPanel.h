


























#ifndef ComplexTextInputPanel_h_
#define ComplexTextInputPanel_h_

#include "nsString.h"
#include "npapi.h"

class ComplexTextInputPanel
{
public:
  static ComplexTextInputPanel* GetSharedComplexTextInputPanel();
  virtual void PlacePanel(int32_t x, int32_t y) = 0; 
  virtual void InterpretKeyEvent(void* aEvent, nsAString& aOutText) = 0;
  virtual bool IsInComposition() = 0;
  virtual void* GetInputContext() = 0;
  virtual void CancelComposition() = 0;

protected:
  virtual ~ComplexTextInputPanel() {};
};

#endif 
