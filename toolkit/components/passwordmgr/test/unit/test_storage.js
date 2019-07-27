










"use strict";




function reloadAndCheckLogins(aExpectedLogins)
{
  yield LoginTestUtils.reloadData();
  LoginTestUtils.checkLogins(aExpectedLogins);
  LoginTestUtils.clearData();
}







add_task(function test_storage_addLogin_nonascii()
{
  let hostname = "http://" + String.fromCharCode(355) + ".example.com";

  
  let loginInfo = TestData.formLogin({
    hostname: hostname,
    formSubmitURL: hostname,
    username: String.fromCharCode(533, 537, 7570, 345),
    password: String.fromCharCode(421, 259, 349, 537),
    usernameField: "field_" + String.fromCharCode(533, 537, 7570, 345),
    passwordField: "field_" + String.fromCharCode(421, 259, 349, 537),
  });
  Services.logins.addLogin(loginInfo);
  yield reloadAndCheckLogins([loginInfo]);

  
  loginInfo = TestData.authLogin({
    httpRealm: String.fromCharCode(355, 277, 349, 357),
  });
  Services.logins.addLogin(loginInfo);
  yield reloadAndCheckLogins([loginInfo]);
});




add_task(function test_storage_addLogin_newlines()
{
  let loginInfo = TestData.formLogin({
    username: "user\r\nname",
    password: "password\r\n",
  });
  Services.logins.addLogin(loginInfo);
  yield reloadAndCheckLogins([loginInfo]);
});






add_task(function test_storage_addLogin_dot()
{
  let loginInfo = TestData.formLogin({ hostname: ".", passwordField: "." });
  Services.logins.addLogin(loginInfo);
  yield reloadAndCheckLogins([loginInfo]);

  loginInfo = TestData.authLogin({ httpRealm: "." });
  Services.logins.addLogin(loginInfo);
  yield reloadAndCheckLogins([loginInfo]);
});






add_task(function test_storage_addLogin_parentheses()
{
  let loginList = [
    TestData.authLogin({ httpRealm: "(realm" }),
    TestData.authLogin({ httpRealm: "realm)" }),
    TestData.authLogin({ httpRealm: "(realm)" }),
    TestData.authLogin({ httpRealm: ")realm(" }),
    TestData.authLogin({ hostname: "http://parens(.example.com" }),
    TestData.authLogin({ hostname: "http://parens).example.com" }),
    TestData.authLogin({ hostname: "http://parens(example).example.com" }),
    TestData.authLogin({ hostname: "http://parens)example(.example.com" }),
  ];
  for (let loginInfo of loginList) {
    Services.logins.addLogin(loginInfo);
  }
  yield reloadAndCheckLogins(loginList);
});
