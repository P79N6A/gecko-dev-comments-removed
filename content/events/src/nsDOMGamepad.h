



#ifndef nsDomGamepad_h
#define nsDomGamepad_h

#include "mozilla/StandardInteger.h"
#include "nsIDOMGamepad.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"

class nsDOMGamepad : public nsIDOMGamepad
{
public:
  nsDOMGamepad(const nsAString& aID, uint32_t aIndex,
               uint32_t aNumButtons, uint32_t aNumAxes);
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGAMEPAD

  nsDOMGamepad();
  void SetConnected(bool aConnected);
  void SetButton(uint32_t aButton, double aValue);
  void SetAxis(uint32_t aAxis, double aValue);
  void SetIndex(uint32_t aIndex);

  
  void SyncState(nsDOMGamepad* other);

  
  already_AddRefed<nsDOMGamepad> Clone();

private:
  virtual ~nsDOMGamepad() {}

protected:
  nsString mID;
  uint32_t mIndex;

  
  bool mConnected;

  
  nsTArray<double> mButtons;
  nsTArray<double> mAxes;
};

#endif 
