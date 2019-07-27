



#ifndef mozilla_dom_GamepadServiceTest_h_
#define mozilla_dom_GamepadServiceTest_h_

#include "nsIGamepadServiceTest.h"

namespace mozilla {
namespace dom {

class GamepadService;


class GamepadServiceTest : public nsIGamepadServiceTest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGAMEPADSERVICETEST

  GamepadServiceTest();

  static already_AddRefed<GamepadServiceTest> CreateService();

private:
  static GamepadServiceTest* sSingleton;
  
  
  nsRefPtr<GamepadService> mService;
  virtual ~GamepadServiceTest();
};

}  
}  

#define NS_GAMEPAD_TEST_CID \
{ 0xfb1fcb57, 0xebab, 0x4cf4, \
{ 0x96, 0x3b, 0x1e, 0x4d, 0xb8, 0x52, 0x16, 0x96 } }
#define NS_GAMEPAD_TEST_CONTRACTID "@mozilla.org/gamepad-test;1"

#endif
