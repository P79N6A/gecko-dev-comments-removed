


Cu.import("resource://testing-common/ContentTaskUtils.jsm", this);
Cu.import("resource://testing-common/LoginTestUtils.jsm", this);




add_task(function* test_save_change() {
  let testCases = [{
    username: "username",
    password: "password",
  }, {
    username: "",
    password: "password",
  }, {
    username: "username",
    oldPassword: "password",
    password: "newPassword",
  }, {
    username: "",
    oldPassword: "password",
    password: "newPassword",
  }];

  for (let { username, oldPassword, password } of testCases) {
    
    if (oldPassword) {
      Services.logins.addLogin(LoginTestUtils.testData.formLogin({
        hostname: "https://example.com",
        formSubmitURL: "https://example.com",
        username,
        password: oldPassword,
      }));
    }

    yield BrowserTestUtils.withNewTab({
      gBrowser,
      url: "https://example.com/browser/toolkit/components/" +
           "passwordmgr/test/browser/form_basic.html",
    }, function* (browser) {
      
      
      let promiseShown = BrowserTestUtils.waitForEvent(PopupNotifications.panel,
                                                       "Shown");
      yield ContentTask.spawn(browser, { username, password },
        function* ({ username, password }) {
          let doc = content.document;
          doc.getElementById("form-basic-username").value = username;
          doc.getElementById("form-basic-password").value = password;
          doc.getElementById("form-basic").submit();
        });
      yield promiseShown;

      
      Assert.equal(document.getElementById("password-notification-username")
                           .getAttribute("value"), username);
      Assert.equal(document.getElementById("password-notification-password")
                           .getAttribute("value"), password);

      
      
      
      let expectedNotification = oldPassword ? "modifyLogin" : "addLogin";
      let promiseLogin = TestUtils.topicObserved("passwordmgr-storage-changed",
                         (_, data) => data == expectedNotification);
      let notificationElement = PopupNotifications.panel.childNodes[0];
      notificationElement.button.doCommand();
      let [result] = yield promiseLogin;

      
      let login = oldPassword ? result.QueryInterface(Ci.nsIArray)
                                      .queryElementAt(1, Ci.nsILoginInfo)
                              : result.QueryInterface(Ci.nsILoginInfo);
      Assert.equal(login.username, username);
      Assert.equal(login.password, password);
    });

    
    Services.logins.removeAllLogins();
  }
});









add_task(function* test_edit_username() {
  let testCases = [{
    usernameInPage: "username",
    usernameChangedTo: "newUsername",
  }, {
    usernameInPage: "username",
    usernameInPageExists: true,
    usernameChangedTo: "newUsername",
  }, {
    usernameInPage: "username",
    usernameChangedTo: "newUsername",
    usernameChangedToExists: true,
  }, {
    usernameInPage: "username",
    usernameInPageExists: true,
    usernameChangedTo: "newUsername",
    usernameChangedToExists: true,
  }, {
    usernameInPage: "",
    usernameChangedTo: "newUsername",
  }, {
    usernameInPage: "newUsername",
    usernameChangedTo: "",
  }, {
    usernameInPage: "",
    usernameChangedTo: "newUsername",
    usernameChangedToExists: true,
  }, {
    usernameInPage: "newUsername",
    usernameChangedTo: "",
    usernameChangedToExists: true,
  }];

  for (let testCase of testCases) {
    info("Test case: " + JSON.stringify(testCase));

    
    if (testCase.usernameInPageExists) {
      Services.logins.addLogin(LoginTestUtils.testData.formLogin({
        hostname: "https://example.com",
        formSubmitURL: "https://example.com",
        username: testCase.usernameInPage,
        password: "old password",
      }));
    }
    if (testCase.usernameChangedToExists) {
      Services.logins.addLogin(LoginTestUtils.testData.formLogin({
        hostname: "https://example.com",
        formSubmitURL: "https://example.com",
        username: testCase.usernameChangedTo,
        password: "old password",
      }));
    }

    yield BrowserTestUtils.withNewTab({
      gBrowser,
      url: "https://example.com/browser/toolkit/components/" +
           "passwordmgr/test/browser/form_basic.html",
    }, function* (browser) {
      
      
      let promiseShown = BrowserTestUtils.waitForEvent(PopupNotifications.panel,
                                                       "Shown");
      yield ContentTask.spawn(browser, testCase.usernameInPage,
        function* (usernameInPage) {
          let doc = content.document;
          doc.getElementById("form-basic-username").value = usernameInPage;
          doc.getElementById("form-basic-password").value = "password";
          doc.getElementById("form-basic").submit();
        });
      yield promiseShown;

      
      if (testCase.usernameChangedTo) {
        document.getElementById("password-notification-username")
                .setAttribute("value", testCase.usernameChangedTo);
      }

      
      
      let expectModifyLogin = testCase.usernameChangedTo
                              ? testCase.usernameChangedToExists
                              : testCase.usernameInPageExists;

      
      
      
      let expectedNotification = expectModifyLogin ? "modifyLogin" : "addLogin";
      let promiseLogin = TestUtils.topicObserved("passwordmgr-storage-changed",
                         (_, data) => data == expectedNotification);
      let notificationElement = PopupNotifications.panel.childNodes[0];
      notificationElement.button.doCommand();
      let [result] = yield promiseLogin;

      
      let login = expectModifyLogin ? result.QueryInterface(Ci.nsIArray)
                                            .queryElementAt(1, Ci.nsILoginInfo)
                                    : result.QueryInterface(Ci.nsILoginInfo);
      Assert.equal(login.username, testCase.usernameChangedTo ||
                                   testCase.usernameInPage);
      Assert.equal(login.password, "password");
    });

    
    Services.logins.removeAllLogins();
  }
});














add_task(function* test_edit_password() {
  let testCases = [{
    usernameInPage: "username",
    passwordInPage: "password",
    passwordChangedTo: "newPassword",
    timesUsed: 1,
  }, {
    usernameInPage: "username",
    usernameInPageExists: true,
    passwordInPage: "password",
    passwordInStorage: "oldPassword",
    passwordChangedTo: "newPassword",
    timesUsed: 2,
  }, {
    usernameInPage: "username",
    usernameChangedTo: "newUsername",
    usernameChangedToExists: true,
    passwordInPage: "password",
    passwordChangedTo: "newPassword",
    timesUsed: 2,
  }, {
    usernameInPage: "username",
    usernameChangedTo: "newUsername",
    usernameChangedToExists: true,
    passwordInPage: "password",
    passwordChangedTo: "password",
    timesUsed: 2,
    checkPasswordNotUpdated: true,
  }, {
    usernameInPage: "newUsername",
    usernameChangedTo: "",
    usernameChangedToExists: true,
    passwordInPage: "password",
    passwordChangedTo: "newPassword",
    timesUsed: 1,
  }];

  for (let testCase of testCases) {
    info("Test case: " + JSON.stringify(testCase));

    
    if (testCase.usernameInPageExists) {
      Services.logins.addLogin(LoginTestUtils.testData.formLogin({
        hostname: "https://example.com",
        formSubmitURL: "https://example.com",
        username: testCase.usernameInPage,
        password: testCase.passwordInStorage,
      }));
    }
    if (testCase.usernameChangedToExists) {
      Services.logins.addLogin(LoginTestUtils.testData.formLogin({
        hostname: "https://example.com",
        formSubmitURL: "https://example.com",
        username: testCase.usernameChangedTo,
        password: testCase.passwordChangedTo,
      }));
    }

    yield BrowserTestUtils.withNewTab({
      gBrowser,
      url: "https://example.com/browser/toolkit/components/" +
           "passwordmgr/test/browser/form_basic.html",
    }, function* (browser) {
      
      
      let promiseShown = BrowserTestUtils.waitForEvent(PopupNotifications.panel,
                                                       "popupshown");
      yield ContentTask.spawn(browser, testCase,
        function* (testCase) {
          let doc = content.document;
          doc.getElementById("form-basic-username").value = testCase.usernameInPage;
          doc.getElementById("form-basic-password").value = testCase.passwordInPage;
          doc.getElementById("form-basic").submit();
        });
      yield promiseShown;

      
      if (testCase.usernameChangedTo) {
        document.getElementById("password-notification-username")
                .setAttribute("value", testCase.usernameChangedTo);
      }

      
      if (testCase.passwordChangedTo) {
        document.getElementById("password-notification-password")
                .setAttribute("value", testCase.passwordChangedTo);
      }

      
      
      let expectModifyLogin = testCase.usernameChangedTo
                              ? testCase.usernameChangedToExists
                              : testCase.usernameInPageExists;

      
      
      
      let expectedNotification = expectModifyLogin ? "modifyLogin" : "addLogin";
      let promiseLogin = TestUtils.topicObserved("passwordmgr-storage-changed",
                         (_, data) => data == expectedNotification);
      let notificationElement = PopupNotifications.panel.childNodes[0];
      notificationElement.button.doCommand();
      let [result] = yield promiseLogin;

      
      let login = expectModifyLogin ? result.QueryInterface(Ci.nsIArray)
                                            .queryElementAt(1, Ci.nsILoginInfo)
                                    : result.QueryInterface(Ci.nsILoginInfo);

      Assert.equal(login.username, testCase.usernameChangedTo ||
                                   testCase.usernameInPage);
      Assert.equal(login.password, testCase.passwordChangedTo ||
                                   testCase.passwordInPage);

      let meta = login.QueryInterface(Ci.nsILoginMetaInfo);
      Assert.equal(meta.timesUsed, testCase.timesUsed);

      
      if (testCase.checkPasswordNotUpdated) {
        Assert.ok(meta.timeLastUsed > meta.timeCreated);
        Assert.ok(meta.timeCreated == meta.timePasswordChanged);
      }
    });

    
    Services.logins.removeAllLogins();
  }
});










add_task(function* test_empty_password() {
  if (Services.appinfo.OS == "Linux") {
    Assert.ok(true, "Skipping test on Linux.");
    return;
  }
  yield BrowserTestUtils.withNewTab({
      gBrowser,
      url: "https://example.com/browser/toolkit/components/" +
           "passwordmgr/test/browser/form_basic.html",
    }, function* (browser) {
      
      
      let promiseShown = BrowserTestUtils.waitForEvent(PopupNotifications.panel,
                                                       "popupshown");
      yield ContentTask.spawn(browser, null,
        function* () {
          let doc = content.document;
          doc.getElementById("form-basic-username").value = "username";
          doc.getElementById("form-basic-password").value = "p";
          doc.getElementById("form-basic").submit();
        });
      yield promiseShown;

      let notificationElement = PopupNotifications.panel.childNodes[0];
      let passwordTextbox = notificationElement.querySelector("#password-notification-password");

      
      let focusPassword = BrowserTestUtils.waitForEvent(passwordTextbox, "focus");
      passwordTextbox.focus();
      yield focusPassword;

      
      yield ContentTaskUtils.waitForCondition(() => passwordTextbox.type == "", "Password textbox changed type");

      
      EventUtils.synthesizeKey("VK_RIGHT", {});
      yield EventUtils.synthesizeKey("VK_BACK_SPACE", {});

      let mainActionButton = document.getAnonymousElementByAttribute(notificationElement.button, "anonid", "button");

      
      yield ContentTaskUtils.waitForCondition(() => mainActionButton.disabled, "Main action button is disabled");

      
      Assert.throws(notificationElement.button.doCommand(),
                    "Can't add a login with a null or empty password.",
                    "Should fail for an empty password");
    });
});








add_task(function* test_unfocus_click() {
  if (Services.appinfo.OS == "Linux") {
    Assert.ok(true, "Skipping test on Linux.");
    return;
  }
  yield BrowserTestUtils.withNewTab({
      gBrowser,
      url: "https://example.com/browser/toolkit/components/" +
           "passwordmgr/test/browser/form_basic.html",
    }, function* (browser) {
      
      

      let promiseShown = BrowserTestUtils.waitForEvent(PopupNotifications.panel,
                                                       "popupshown");
      yield ContentTask.spawn(browser, null,
        function* () {
          let doc = content.document;
          doc.getElementById("form-basic-username").value = "username";
          doc.getElementById("form-basic-password").value = "password";
          doc.getElementById("form-basic").submit();
        });
      yield promiseShown;

      let notificationElement = PopupNotifications.panel.childNodes[0];
      let passwordTextbox = notificationElement.querySelector("#password-notification-password");

      
      let focusPassword = BrowserTestUtils.waitForEvent(passwordTextbox, "focus");
      passwordTextbox.focus();
      yield focusPassword;

      
      yield ContentTaskUtils.waitForCondition(() => passwordTextbox.type == "",
                                              "Password textbox changed type");

      let notificationIcon = document.getAnonymousElementByAttribute(notificationElement,
                                                                     "class",
                                                                     "popup-notification-icon");

      yield EventUtils.synthesizeMouseAtCenter(notificationIcon, {});

      
      yield ContentTaskUtils.waitForCondition(() => passwordTextbox.type == "password",
                                              "Password textbox changed type back to password");
    });
});
