



#ifndef SANDBOX_SRC_RESTRICTED_TOKEN_H_
#define SANDBOX_SRC_RESTRICTED_TOKEN_H_

#include <windows.h>
#include <vector>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/restricted_token_utils.h"
#include "sandbox/win/src/security_level.h"
#include "sandbox/win/src/sid.h"


#ifndef SE_GROUP_INTEGRITY
#define SE_GROUP_INTEGRITY (0x00000020L)
#endif
#ifndef SE_GROUP_INTEGRITY_ENABLED
#define SE_GROUP_INTEGRITY_ENABLED (0x00000040L)
#endif

namespace sandbox {



















class RestrictedToken {
 public:
  
  RestrictedToken()
      : init_(false), effective_token_(NULL),
        integrity_level_(INTEGRITY_LEVEL_LAST) { }

  ~RestrictedToken() {
    if (effective_token_)
      CloseHandle(effective_token_);
  }

  
  
  
  unsigned Init(HANDLE effective_token);

  
  
  
  
  
  unsigned GetRestrictedTokenHandle(HANDLE *token_handle) const;

  
  
  
  
  
  
  
  
  
  
  unsigned GetRestrictedTokenHandleForImpersonation(HANDLE *token_handle) const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  unsigned AddAllSidsForDenyOnly(std::vector<Sid> *exceptions);

  
  
  
  
  
  
  unsigned AddSidForDenyOnly(const Sid &sid);

  
  
  
  
  unsigned AddUserSidForDenyOnly();

  
  
  
  
  
  
  
  
  
  
  
  
  
  unsigned DeleteAllPrivileges(
      const std::vector<base::string16> *exceptions);

  
  
  
  
  
  
  
  
  
  
  unsigned DeletePrivilege(const wchar_t *privilege);

  
  
  
  
  
  
  
  
  
  
  unsigned AddRestrictingSid(const Sid &sid);

  
  
  
  
  
  
  unsigned AddRestrictingSidLogonSession();

  
  
  
  
  
  
  unsigned AddRestrictingSidCurrentUser();

  
  
  
  
  
  unsigned AddRestrictingSidAllSids();

  
  
  unsigned SetIntegrityLevel(IntegrityLevel integrity_level);

 private:
  
  std::vector<Sid> sids_to_restrict_;
  
  std::vector<LUID> privileges_to_disable_;
  
  std::vector<Sid> sids_for_deny_only_;
  
  HANDLE effective_token_;
  
  IntegrityLevel integrity_level_;
  
  bool init_;

  DISALLOW_COPY_AND_ASSIGN(RestrictedToken);
};

}  

#endif  
