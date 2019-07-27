


Cu.import("resource://testing-common/LoginTestUtils.jsm", this);





add_task(function* test_initialize() {
  Services.prefs.setBoolPref("signon.ui.experimental", true);
  Services.prefs.setBoolPref("signon.autofillForms", false);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("signon.ui.experimental");
    Services.prefs.clearUserPref("signon.autofillForms");
  });
});




add_task(function* test_fill() {
  Services.logins.addLogin(LoginTestUtils.testData.formLogin({
    hostname: "https://example.com",
    formSubmitURL: "https://example.com",
    username: "username",
    password: "password",
  }));

  
  
  
  let anchor = document.getElementById("login-fill-notification-icon");
  let promiseAnchorShown =
      TestUtils.topicObserved("PopupNotifications-updateNotShowing",
                              () => anchor.hasAttribute("showing"));

  yield BrowserTestUtils.withNewTab({
    gBrowser,
    url: "https://example.com/browser/toolkit/components/" +
         "passwordmgr/test/browser/form_basic.html",
  }, function* (browser) {
    yield promiseAnchorShown;

    let promiseShown = BrowserTestUtils.waitForEvent(PopupNotifications.panel,
                                                     "Shown");
    anchor.click();
    yield promiseShown;

    let list = document.getElementById("login-fill-list");
    Assert.equal(list.childNodes.length, 1,
                 "list.childNodes.length === 1");

    
    list.focus();
    yield new Promise(resolve => executeSoon(resolve));
    let details = document.getElementById("login-fill-details");
    let promiseSubview = BrowserTestUtils.waitForEvent(details,
                                                       "transitionend", true,
                                                       e => e.target == details);
    EventUtils.sendMouseEvent({ type: "click" }, list.childNodes[0]);
    yield promiseSubview;

    
    let promiseHidden = BrowserTestUtils.waitForEvent(PopupNotifications.panel,
                                                      "popuphidden");
    document.getElementById("login-fill-use").click();
    yield promiseHidden;

    let result = yield ContentTask.spawn(browser, null, function* () {
      let doc = content.document;
      return {
        username: doc.getElementById("form-basic-username").value,
        password: doc.getElementById("form-basic-password").value,
      }
    });

    Assert.equal(result.username, "username",
                 'result.username === "username"');
    Assert.equal(result.password, "password",
                 'result.password === "password"');
  });

  Services.logins.removeAllLogins();
});
