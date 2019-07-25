






































#ifndef mozilla_hal_WindowIdentifier_h
#define mozilla_hal_WindowIdentifier_h

#include "mozilla/Types.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"

namespace mozilla {
namespace hal {





















class WindowIdentifier
{
public:
  





  WindowIdentifier();

  


  WindowIdentifier(const WindowIdentifier& other);

  







  explicit WindowIdentifier(nsIDOMWindow* window);

  




  WindowIdentifier(const nsTArray<uint64>& id, nsIDOMWindow* window);

  


  typedef InfallibleTArray<uint64> IDArrayType;
  const IDArrayType& AsArray() const;

  



  void AppendProcessID();

  




  bool HasTraveledThroughIPC() const;

  


  nsIDOMWindow* GetWindow() const;

private:
  


  uint64 GetWindowID() const;

  AutoInfallibleTArray<uint64, 3> mID;
  nsCOMPtr<nsIDOMWindow> mWindow;
  bool mIsEmpty;
};

} 
} 

#endif 
