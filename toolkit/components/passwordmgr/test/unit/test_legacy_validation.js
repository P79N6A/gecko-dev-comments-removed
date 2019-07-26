












"use strict";







add_task(function test_addLogin_invalid_characters_legacy()
{
  
  for (let testValue of ["http://newline\n.example.com",
                         "http://carriagereturn.example.com\r"]) {
    let loginInfo = TestData.formLogin({ hostname: testValue });
    Assert.throws(() => Services.logins.addLogin(loginInfo),
                  /login values can't contain newlines/);

    loginInfo = TestData.formLogin({ formSubmitURL: testValue });
    Assert.throws(() => Services.logins.addLogin(loginInfo),
                  /login values can't contain newlines/);

    loginInfo = TestData.authLogin({ httpRealm: testValue });
    Assert.throws(() => Services.logins.addLogin(loginInfo),
                  /login values can't contain newlines/);
  }

  
  for (let testValue of ["newline_field\n", "carriagereturn\r_field"]) {
    let loginInfo = TestData.formLogin({ usernameField: testValue });
    Assert.throws(() => Services.logins.addLogin(loginInfo),
                  /login values can't contain newlines/);

    loginInfo = TestData.formLogin({ passwordField: testValue });
    Assert.throws(() => Services.logins.addLogin(loginInfo),
                  /login values can't contain newlines/);
  }

  
  let loginInfo = TestData.formLogin({ usernameField: "." });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /login values can't be periods/);

  loginInfo = TestData.formLogin({ formSubmitURL: "." });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /login values can't be periods/);

  
  loginInfo = TestData.formLogin({ hostname: "http://parens (.example.com" });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /bad parens in hostname/);
});




add_task(function test_setLoginSavingEnabled_invalid_characters_legacy()
{
  for (let hostname of ["http://newline\n.example.com",
                        "http://carriagereturn.example.com\r",
                        "."]) {
    Assert.throws(() => Services.logins.setLoginSavingEnabled(hostname, false),
                  /Invalid hostname/);
  }
});
