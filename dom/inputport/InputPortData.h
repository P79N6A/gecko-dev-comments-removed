





#ifndef mozilla_dom_InputPortData_h
#define mozilla_dom_InputPortData_h

#include "mozilla/dom/InputPortBinding.h"
#include "nsIInputPortService.h"

class nsString;

namespace mozilla {
namespace dom {

enum class InputPortType : uint32_t
{
  Av,
  Displayport,
  Hdmi,
  EndGuard_
};

class InputPortData final : public nsIInputPortData
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTPORTDATA

  InputPortData();

  const nsString& GetId() const;

  const InputPortType GetType() const;

private:
  ~InputPortData();

  nsString mId;
  nsString mType;
  bool mIsConnected;
};

} 
} 

#endif
