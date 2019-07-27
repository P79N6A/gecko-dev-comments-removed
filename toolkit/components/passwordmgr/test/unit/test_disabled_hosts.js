








"use strict";







add_task(function test_setLoginSavingEnabled_getAllDisabledHosts()
{
  
  
  let hostname1 = "http://disabled1.example.com";
  let hostname2 = "http://disabled2.example.com";
  let hostname3 = "https://disabled2.example.com";
  Services.logins.setLoginSavingEnabled(hostname1, false);
  Services.logins.setLoginSavingEnabled(hostname2, false);
  Services.logins.setLoginSavingEnabled(hostname3, false);

  LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                          [hostname1, hostname2, hostname3]);

  
  Services.logins.setLoginSavingEnabled(hostname2, false);
  LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                          [hostname1, hostname2, hostname3]);

  
  Services.logins.setLoginSavingEnabled(hostname2, true);
  LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                          [hostname1, hostname3]);

  
  Services.logins.setLoginSavingEnabled(hostname1, true);
  Services.logins.setLoginSavingEnabled(hostname3, true);
  LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                          []);
});




add_task(function test_setLoginSavingEnabled_getLoginSavingEnabled()
{
  let hostname1 = "http://disabled.example.com";
  let hostname2 = "https://disabled.example.com";

  
  do_check_true(Services.logins.getLoginSavingEnabled(hostname1));
  do_check_true(Services.logins.getLoginSavingEnabled(hostname2));

  
  Services.logins.setLoginSavingEnabled(hostname1, false);
  Services.logins.setLoginSavingEnabled(hostname2, true);
  do_check_false(Services.logins.getLoginSavingEnabled(hostname1));
  do_check_true(Services.logins.getLoginSavingEnabled(hostname2));

  
  Services.logins.setLoginSavingEnabled(hostname1, true);
  Services.logins.setLoginSavingEnabled(hostname2, false);
  do_check_true(Services.logins.getLoginSavingEnabled(hostname1));
  do_check_false(Services.logins.getLoginSavingEnabled(hostname2));

  
  Services.logins.setLoginSavingEnabled(hostname2, true);
});




add_task(function test_setLoginSavingEnabled_invalid_characters()
{
  let hostname = "http://null\0X.example.com";
  Assert.throws(() => Services.logins.setLoginSavingEnabled(hostname, false),
                /Invalid hostname/);

  
  LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                          []);
});




add_task(function test_rememberSignons()
{
  let hostname1 = "http://example.com";
  let hostname2 = "http://localhost";

  
  do_check_true(Services.prefs.getBoolPref("signon.rememberSignons"));

  
  Services.logins.setLoginSavingEnabled(hostname1, false);
  do_check_false(Services.logins.getLoginSavingEnabled(hostname1));
  do_check_true(Services.logins.getLoginSavingEnabled(hostname2));

  
  Services.prefs.setBoolPref("signon.rememberSignons", false);
  do_register_cleanup(
    () => Services.prefs.clearUserPref("signon.rememberSignons"));

  
  do_check_false(Services.logins.getLoginSavingEnabled(hostname1));
  do_check_false(Services.logins.getLoginSavingEnabled(hostname2));

  
  LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                          [hostname1]);

  
  Services.logins.setLoginSavingEnabled(hostname1, true);
  Services.logins.setLoginSavingEnabled(hostname2, false);

  
  do_check_false(Services.logins.getLoginSavingEnabled(hostname1));
  do_check_false(Services.logins.getLoginSavingEnabled(hostname2));

  
  LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                          [hostname2]);

  
  Services.prefs.setBoolPref("signon.rememberSignons", true);

  
  do_check_true(Services.logins.getLoginSavingEnabled(hostname1));
  do_check_false(Services.logins.getLoginSavingEnabled(hostname2));

  
  Services.logins.setLoginSavingEnabled(hostname2, true);
  LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                          []);
});




add_task(function test_storage_setLoginSavingEnabled_nonascii()
{
  let hostname = "http://" + String.fromCharCode(355) + ".example.com";
  Services.logins.setLoginSavingEnabled(hostname, false);

  yield LoginTestUtils.reloadData();
  LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                          [hostname]);
  LoginTestUtils.clearData();
});
