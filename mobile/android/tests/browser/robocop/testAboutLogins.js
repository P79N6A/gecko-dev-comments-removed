



"use strict"

const { interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AndroidLog.jsm");

function ok(passed, text) {
  do_report_result(passed, text, Components.stack.caller, false);
}

const LOGIN_FIELDS = {
  hostname: "http://example.org/tests/robocop/robocop_blank_01.html",
  formSubmitUrl: "",
  realmAny: null,
  username: "username1",
  password: "password1",
  usernameField: "",
  passwordField: ""
};

const LoginInfo = Components.Constructor("@mozilla.org/login-manager/loginInfo;1", "nsILoginInfo", "init");

let BrowserApp;
let browser;

function add_login(login) {
  let newLogin = new LoginInfo(login.hostname,
                               login.formSubmitUrl,
                               login.realmAny,
                               login.username,
                               login.password,
                               login.usernameField,
                               login.passwordField);

  Services.logins.addLogin(newLogin);
}

add_test(function password_setup() {
  add_login(LOGIN_FIELDS);

  
  BrowserApp = Services.wm.getMostRecentWindow("navigator:browser").BrowserApp;
  browser = BrowserApp.addTab("about:logins", { selected: true, parentId: BrowserApp.selectedTab.id }).browser;

  browser.addEventListener("load", () => {
    browser.removeEventListener("load", this, true);
    Services.tm.mainThread.dispatch(run_next_test, Ci.nsIThread.DISPATCH_NORMAL);
  }, true);
});

add_test(function test_passwords_list() {
  
  let logins_list = browser.contentDocument.getElementById("logins-list");

  let hostname = logins_list.querySelector(".hostname");
  do_check_eq(hostname.textContent, LOGIN_FIELDS.hostname);

  let username = logins_list.querySelector(".username");
  do_check_eq(username.textContent, LOGIN_FIELDS.username);

  run_next_test();
});

run_next_test();
