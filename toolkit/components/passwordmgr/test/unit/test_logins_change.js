








"use strict";







add_task(function test_addLogin_removeLogin()
{
  
  for (let loginInfo of TestData.loginList()) {
    Services.logins.addLogin(loginInfo);
  }
  LoginTest.checkLogins(TestData.loginList());

  
  for (let loginInfo of TestData.loginList()) {
    Assert.throws(() => Services.logins.addLogin(loginInfo), /already exists/);
  }

  
  for (let loginInfo of TestData.loginList()) {
    Services.logins.removeLogin(loginInfo);
  }

  LoginTest.checkLogins([]);
});










add_task(function test_addLogin_invalid_httpRealm_formSubmitURL()
{
  
  let loginInfo = TestData.formLogin({ formSubmitURL: null });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /without a httpRealm or formSubmitURL/);

  
  loginInfo = TestData.authLogin({ httpRealm: "" });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /without a httpRealm or formSubmitURL/);

  
  loginInfo = TestData.formLogin({ formSubmitURL: "" });
  
  
  

  
  loginInfo = TestData.formLogin({ formSubmitURL: "", httpRealm: "" });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /both a httpRealm and formSubmitURL/);

  
  loginInfo = TestData.formLogin({ httpRealm: "The HTTP Realm" });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /both a httpRealm and formSubmitURL/);

  
  loginInfo = TestData.formLogin({ httpRealm: "" });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /both a httpRealm and formSubmitURL/);

  
  loginInfo = TestData.authLogin({ formSubmitURL: "" });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /both a httpRealm and formSubmitURL/);

  
  LoginTest.checkLogins([]);
});




add_task(function test_addLogin_missing_properties()
{
  let loginInfo = TestData.formLogin({ hostname: null });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /null or empty hostname/);

  loginInfo = TestData.formLogin({ hostname: "" });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /null or empty hostname/);

  loginInfo = TestData.formLogin({ username: null });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /null username/);

  loginInfo = TestData.formLogin({ password: null });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /null or empty password/);

  loginInfo = TestData.formLogin({ password: "" });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /null or empty password/);

  
  LoginTest.checkLogins([]);
});




add_task(function test_addLogin_invalid_characters()
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
    Assert.throws(() => Services.logins.addLogin(loginInfo),
                  /login values can't contain nulls/);
  }

  
  LoginTest.checkLogins([]);
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
  LoginTest.checkLogins([]);

  
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

  
  LoginTest.checkLogins([updatedLoginInfo]);
  Assert.throws(() => Services.logins.modifyLogin(loginInfo, updatedLoginInfo),
                /No matching logins/);

  
  Services.logins.modifyLogin(updatedLoginInfo, differentLoginInfo);
  LoginTest.checkLogins([differentLoginInfo]);

  
  Services.logins.addLogin(loginInfo);
  LoginTest.checkLogins([loginInfo, differentLoginInfo]);

  
  
  Services.logins.modifyLogin(loginInfo, differentLoginInfo);
  LoginTest.checkLogins([differentLoginInfo, differentLoginInfo]);

  
  Services.logins.removeLogin(differentLoginInfo);
  LoginTest.checkLogins([differentLoginInfo]);
  Services.logins.removeLogin(differentLoginInfo);
  LoginTest.checkLogins([]);
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

  
  LoginTest.checkLogins([updatedLoginInfo]);
  Assert.throws(() => Services.logins.modifyLogin(loginInfo, newPropertyBag()),
                /No matching logins/);

  
  Services.logins.modifyLogin(updatedLoginInfo, newPropertyBag());

  
  Assert.throws(() => Services.logins.modifyLogin(loginInfo, newPropertyBag({
    usernameField: null,
  })));

  
  Services.logins.modifyLogin(updatedLoginInfo, differentLoginProperties);
  LoginTest.checkLogins([differentLoginInfo]);

  
  Services.logins.addLogin(loginInfo);
  LoginTest.checkLogins([loginInfo, differentLoginInfo]);

  
  
  Services.logins.modifyLogin(loginInfo, differentLoginProperties);
  LoginTest.checkLogins([differentLoginInfo, differentLoginInfo]);

  
  Services.logins.removeLogin(differentLoginInfo);
  LoginTest.checkLogins([differentLoginInfo]);
  Services.logins.removeLogin(differentLoginInfo);
  LoginTest.checkLogins([]);
});
