








"use strict";








function resetMasterPassword()
{
  let token = Cc["@mozilla.org/security/pk11tokendb;1"]
                .getService(Ci.nsIPK11TokenDB).getInternalKeyToken();
  token.reset();
  token.changePassword("", "");
}







add_task(function test_logins_decrypt_failure()
{
  let logins = TestData.loginList();
  for (let loginInfo of logins) {
    Services.logins.addLogin(loginInfo);
  }

  
  resetMasterPassword();

  
  do_check_eq(Services.logins.getAllLogins().length, 0);
  do_check_eq(Services.logins.findLogins({}, "", "", "").length, 0);
  do_check_eq(Services.logins.searchLogins({}, newPropertyBag()).length, 0);
  Assert.throws(() => Services.logins.modifyLogin(logins[0], newPropertyBag()),
                      /No matching logins/);
  Assert.throws(() => Services.logins.removeLogin(logins[0]),
                      /No matching logins/);

  
  do_check_eq(Services.logins.countLogins("", "", ""), logins.length);

  
  for (let loginInfo of logins) {
    Services.logins.addLogin(loginInfo);
  }
  LoginTestUtils.checkLogins(logins);
  do_check_eq(Services.logins.countLogins("", "", ""), logins.length * 2);

  
  do_check_eq(Services.logins.findLogins({}, "http://www.example.com",
                                         "", "").length, 1);
  let matchData = newPropertyBag({ hostname: "http://www.example.com" });
  do_check_eq(Services.logins.searchLogins({}, matchData).length, 1);

  
  for (let loginInfo of TestData.loginList()) {
    Services.logins.removeLogin(loginInfo);
  }
  do_check_eq(Services.logins.getAllLogins().length, 0);
  do_check_eq(Services.logins.countLogins("", "", ""), logins.length);

  
  Services.logins.removeAllLogins();
  do_check_eq(Services.logins.getAllLogins().length, 0);
  do_check_eq(Services.logins.countLogins("", "", ""), 0);
});
