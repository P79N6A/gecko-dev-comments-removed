




































"use strict";








add_task(function test_addLogin_wildcard()
{
  let loginInfo = TestData.formLogin({ hostname: "http://any.example.com",
                                       formSubmitURL: "" });
  Services.logins.addLogin(loginInfo);

  
  loginInfo = TestData.formLogin({ hostname: "http://any.example.com" });
  Assert.throws(() => Services.logins.addLogin(loginInfo), /already exists/);

  
  loginInfo = TestData.authLogin({ hostname: "http://any.example.com" });
  Services.logins.addLogin(loginInfo);

  
  loginInfo = TestData.formLogin({ hostname: "http://other.example.com" });
  Services.logins.addLogin(loginInfo);
});






add_task(function test_search_all_wildcard()
{
  
  let matchData = newPropertyBag({ formSubmitURL: "http://www.example.com" });
  do_check_eq(Services.logins.searchLogins({}, matchData).length, 2);

  do_check_eq(Services.logins.findLogins({}, "", "http://www.example.com",
                                         null).length, 2);

  do_check_eq(Services.logins.countLogins("", "http://www.example.com",
                                          null), 2);

  
  matchData.setProperty("hostname", "http://any.example.com");
  do_check_eq(Services.logins.searchLogins({}, matchData).length, 1);

  do_check_eq(Services.logins.findLogins({}, "http://any.example.com",
                                             "http://www.example.com",
                                             null).length, 1);

  do_check_eq(Services.logins.countLogins("http://any.example.com",
                                          "http://www.example.com",
                                          null), 1);
});





add_task(function test_searchLogins_wildcard()
{
  let logins = Services.logins.searchLogins({},
                               newPropertyBag({ formSubmitURL: "" }));

  let loginInfo = TestData.formLogin({ hostname: "http://any.example.com",
                                       formSubmitURL: "" });
  LoginTestUtils.assertLoginListsEqual(logins, [loginInfo]);
});
