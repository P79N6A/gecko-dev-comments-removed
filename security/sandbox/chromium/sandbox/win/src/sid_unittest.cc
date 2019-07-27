





#define _ATL_NO_EXCEPTIONS
#include <atlbase.h>
#include <atlsecurity.h>

#include "sandbox/win/src/sid.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {



BOOL EqualSid(const SID *sid1, const SID *sid2) {
  return ::EqualSid(const_cast<SID*>(sid1), const_cast<SID*>(sid2));
}


TEST(SidTest, Constructors) {
  ATL::CSid sid_world = ATL::Sids::World();
  SID *sid_world_pointer = const_cast<SID*>(sid_world.GetPSID());

  
  Sid sid_sid_star(sid_world_pointer);
  ASSERT_TRUE(EqualSid(sid_world_pointer, sid_sid_star.GetPSID()));

  
  Sid sid_copy(sid_sid_star);
  ASSERT_TRUE(EqualSid(sid_world_pointer, sid_copy.GetPSID()));

  
  
}


TEST(SidTest, GetPSID) {
  
  ASSERT_NE(static_cast<SID*>(NULL), Sid(::WinLocalSid).GetPSID());
  ASSERT_NE(static_cast<SID*>(NULL), Sid(::WinCreatorOwnerSid).GetPSID());
  ASSERT_NE(static_cast<SID*>(NULL), Sid(::WinBatchSid).GetPSID());

  ASSERT_TRUE(EqualSid(Sid(::WinNullSid).GetPSID(),
                       ATL::Sids::Null().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinWorldSid).GetPSID(),
                       ATL::Sids::World().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinDialupSid).GetPSID(),
                       ATL::Sids::Dialup().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinNetworkSid).GetPSID(),
                       ATL::Sids::Network().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinBuiltinAdministratorsSid).GetPSID(),
                       ATL::Sids::Admins().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinBuiltinUsersSid).GetPSID(),
                       ATL::Sids::Users().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinBuiltinGuestsSid).GetPSID(),
                       ATL::Sids::Guests().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinProxySid).GetPSID(),
                       ATL::Sids::Proxy().GetPSID()));
}

}  
