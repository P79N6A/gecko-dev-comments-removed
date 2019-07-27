








"use strict";
















function checkLoginInvalid(aLoginInfo, aExpectedError)
{
  
  Assert.throws(() => Services.logins.addLogin(aLoginInfo), aExpectedError);
  LoginTestUtils.checkLogins([]);

  
  let testLogin = TestData.formLogin({ hostname: "http://modify.example.com" });
  Services.logins.addLogin(testLogin);

  
  Assert.throws(() => Services.logins.modifyLogin(testLogin, aLoginInfo),
                aExpectedError);
  Assert.throws(() => Services.logins.modifyLogin(testLogin, newPropertyBag({
    hostname: aLoginInfo.hostname,
    formSubmitURL: aLoginInfo.formSubmitURL,
    httpRealm: aLoginInfo.httpRealm,
    username: aLoginInfo.username,
    password: aLoginInfo.password,
    usernameField: aLoginInfo.usernameField,
    passwordField: aLoginInfo.passwordField,
  })), aExpectedError);

  
  LoginTestUtils.checkLogins([testLogin]);
  Services.logins.removeLogin(testLogin);
}







add_task(function test_addLogin_removeLogin()
{
  
  for (let loginInfo of TestData.loginList()) {
    Services.logins.addLogin(loginInfo);
  }
  LoginTestUtils.checkLogins(TestData.loginList());

  
  for (let loginInfo of TestData.loginList()) {
    Assert.throws(() => Services.logins.addLogin(loginInfo), /already exists/);
  }

  
  for (let loginInfo of TestData.loginList()) {
    Services.logins.removeLogin(loginInfo);
  }

  LoginTestUtils.checkLogins([]);
});










add_task(function test_invalid_httpRealm_formSubmitURL()
{
  
  checkLoginInvalid(TestData.formLogin({ formSubmitURL: null }),
                    /without a httpRealm or formSubmitURL/);

  
  checkLoginInvalid(TestData.authLogin({ httpRealm: "" }),
                    /without a httpRealm or formSubmitURL/);

  
  
  
  

  
  checkLoginInvalid(TestData.formLogin({ formSubmitURL: "", httpRealm: "" }),
                    /both a httpRealm and formSubmitURL/);

  
  checkLoginInvalid(TestData.formLogin({ httpRealm: "The HTTP Realm" }),
                    /both a httpRealm and formSubmitURL/);

  
  checkLoginInvalid(TestData.formLogin({ httpRealm: "" }),
                    /both a httpRealm and formSubmitURL/);

  
  checkLoginInvalid(TestData.authLogin({ formSubmitURL: "" }),
                    /both a httpRealm and formSubmitURL/);
});




add_task(function test_missing_properties()
{
  checkLoginInvalid(TestData.formLogin({ hostname: null }),
                    /null or empty hostname/);

  checkLoginInvalid(TestData.formLogin({ hostname: "" }),
                    /null or empty hostname/);

  checkLoginInvalid(TestData.formLogin({ username: null }),
                    /null username/);

  checkLoginInvalid(TestData.formLogin({ password: null }),
                    /null or empty password/);

  checkLoginInvalid(TestData.formLogin({ password: "" }),
                    /null or empty password/);
});




add_task(function test_invalid_characters()
{
  let loginList = [
    TestData.authLogin({ hostname: "http://null\0X.example.com" }),
    TestData.authLogin({ httpRealm: "realm\0" }),
    TestData.formLogin({ formSubmitURL: "http://null\0X.example.com" }),
    TestData.formLogin({ usernameField: "field\0_null" }),
    TestData.formLogin({ usernameField: ".\0" }), 
    TestData.formLogin({ passwordField: "field\0_null" }),
    TestData.formLogin({ username: "user\0name" }),
    TestData.formLogin({ password: "pass\0word" }),
  ];
  for (let loginInfo of loginList) {
    checkLoginInvalid(loginInfo, /login values can't contain nulls/);
  }
});




add_task(function test_removeLogin_nonexisting()
{
  Assert.throws(() => Services.logins.removeLogin(TestData.formLogin()),
                /No matching logins/);
});




add_task(function test_removeAllLogins()
{
  for (let loginInfo of TestData.loginList()) {
    Services.logins.addLogin(loginInfo);
  }
  Services.logins.removeAllLogins();
  LoginTestUtils.checkLogins([]);

  
  Services.logins.removeAllLogins();
});




add_task(function test_modifyLogin_nsILoginInfo()
{
  let loginInfo = TestData.formLogin();
  let updatedLoginInfo = TestData.formLogin({
    username: "new username",
    password: "new password",
    usernameField: "new_form_field_username",
    passwordField: "new_form_field_password",
  });
  let differentLoginInfo = TestData.authLogin();

  
  Assert.throws(() => Services.logins.modifyLogin(loginInfo, updatedLoginInfo),
                /No matching logins/);

  
  Services.logins.addLogin(loginInfo);
  Services.logins.modifyLogin(loginInfo, updatedLoginInfo);

  
  LoginTestUtils.checkLogins([updatedLoginInfo]);
  Assert.throws(() => Services.logins.modifyLogin(loginInfo, updatedLoginInfo),
                /No matching logins/);

  
  Services.logins.modifyLogin(updatedLoginInfo, differentLoginInfo);
  LoginTestUtils.checkLogins([differentLoginInfo]);

  
  Services.logins.addLogin(loginInfo);
  LoginTestUtils.checkLogins([loginInfo, differentLoginInfo]);

  
  Assert.throws(
         () => Services.logins.modifyLogin(loginInfo, differentLoginInfo),
         /already exists/);
  LoginTestUtils.checkLogins([loginInfo, differentLoginInfo]);

  LoginTestUtils.clearData();
});




add_task(function test_modifyLogin_nsIProperyBag()
{
  let loginInfo = TestData.formLogin();
  let updatedLoginInfo = TestData.formLogin({
    username: "new username",
    password: "new password",
    usernameField: "",
    passwordField: "new_form_field_password",
  });
  let differentLoginInfo = TestData.authLogin();
  let differentLoginProperties = newPropertyBag({
    hostname: differentLoginInfo.hostname,
    formSubmitURL: differentLoginInfo.formSubmitURL,
    httpRealm: differentLoginInfo.httpRealm,
    username: differentLoginInfo.username,
    password: differentLoginInfo.password,
    usernameField: differentLoginInfo.usernameField,
    passwordField: differentLoginInfo.passwordField,
  });

  
  Assert.throws(() => Services.logins.modifyLogin(loginInfo, newPropertyBag()),
                /No matching logins/);

  
  
  Services.logins.addLogin(loginInfo);
  Services.logins.modifyLogin(loginInfo, newPropertyBag({
    username: "new username",
    password: "new password",
    usernameField: "",
    passwordField: "new_form_field_password",
  }));

  
  LoginTestUtils.checkLogins([updatedLoginInfo]);
  Assert.throws(() => Services.logins.modifyLogin(loginInfo, newPropertyBag()),
                /No matching logins/);

  
  Services.logins.modifyLogin(updatedLoginInfo, newPropertyBag());

  
  Assert.throws(() => Services.logins.modifyLogin(loginInfo, newPropertyBag({
    usernameField: null,
  })));

  
  Services.logins.modifyLogin(updatedLoginInfo, differentLoginProperties);
  LoginTestUtils.checkLogins([differentLoginInfo]);

  
  Services.logins.addLogin(loginInfo);
  LoginTestUtils.checkLogins([loginInfo, differentLoginInfo]);

  
  Assert.throws(
         () => Services.logins.modifyLogin(loginInfo, differentLoginProperties),
         /already exists/);
  LoginTestUtils.checkLogins([loginInfo, differentLoginInfo]);

  LoginTestUtils.clearData();
});
