






"use strict";

this.EXPORTED_SYMBOLS = [
  "LoginTestUtils",
];

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://testing-common/TestUtils.jsm");

const LoginInfo =
      Components.Constructor("@mozilla.org/login-manager/loginInfo;1",
                             "nsILoginInfo", "init");


let Assert = null;

this.LoginTestUtils = {
  set Assert(assert) {
    Assert = assert;
  },

  



  reloadData() {
    Services.obs.notifyObservers(null, "passwordmgr-storage-replace", null);
    yield TestUtils.topicObserved("passwordmgr-storage-replace-complete");
  },

  


  clearData() {
    Services.logins.removeAllLogins();
    for (let hostname of Services.logins.getAllDisabledHosts()) {
      Services.logins.setLoginSavingEnabled(hostname, true);
    }
  },

  




  checkLogins(expectedLogins) {
    this.assertLoginListsEqual(Services.logins.getAllLogins(), expectedLogins);
  },

  





  assertLoginListsEqual(actual, expected) {
    Assert.equal(expected.length, actual.length);
    Assert.ok(expected.every(e => actual.some(a => a.equals(e))));
  },

  



  assertLoginListsMatches(actual, expected, ignorePassword) {
    Assert.equal(expected.length, actual.length);
    Assert.ok(expected.every(e => actual.some(a => a.matches(e, ignorePassword))));
  },

  



  assertDisabledHostsEqual(actual, expected) {
    Assert.deepEqual(actual.sort(), expected.sort());
  },

  



  assertTimeIsAboutNow(timeMs) {
    Assert.ok(Math.abs(timeMs - Date.now()) < 30000);
  },
};









this.LoginTestUtils.testData = {
  






  formLogin(modifications) {
    let loginInfo = new LoginInfo("http://www3.example.com",
                                  "http://www.example.com", null,
                                  "the username", "the password",
                                  "form_field_username", "form_field_password");
    loginInfo.QueryInterface(Ci.nsILoginMetaInfo);
    if (modifications) {
      for (let [name, value] of Iterator(modifications)) {
        loginInfo[name] = value;
      }
    }
    return loginInfo;
  },

  






  authLogin(modifications) {
    let loginInfo = new LoginInfo("http://www.example.org", null,
                                  "The HTTP Realm", "the username",
                                  "the password", "", "");
    loginInfo.QueryInterface(Ci.nsILoginMetaInfo);
    if (modifications) {
      for (let [name, value] of Iterator(modifications)) {
        loginInfo[name] = value;
      }
    }
    return loginInfo;
  },

  



  loginList() {
    return [
      

      
      new LoginInfo("http://www.example.com", "http://www.example.com", null,
                    "the username", "the password for www.example.com",
                    "form_field_username", "form_field_password"),

      
      new LoginInfo("https://www.example.com", "https://www.example.com", null,
                    "the username", "the password for https",
                    "form_field_username", "form_field_password"),

      
      new LoginInfo("https://example.com", "https://example.com", null,
                    "the username", "the password for example.com",
                    "form_field_username", "form_field_password"),

      
      
      new LoginInfo("http://www3.example.com", "http://www.example.com", null,
                    "the username", "the password",
                    "form_field_username", "form_field_password"),
      new LoginInfo("http://www3.example.com", "http://example.com", null,
                    "the username", "the password",
                    "form_field_username", "form_field_password"),

      
      
      
      new LoginInfo("http://www4.example.com", "http://www4.example.com", null,
                    "username one", "password one",
                    "form_field_username", "form_field_password"),
      new LoginInfo("http://www4.example.com", "http://www4.example.com", null,
                    "username two", "password two",
                    "form_field_username", "form_field_password"),
      new LoginInfo("http://www4.example.com", "http://www4.example.com", null,
                    "", "password three",
                    "form_field_username", "form_field_password"),

      
      new LoginInfo("http://www5.example.com", "http://www5.example.com", null,
                    "multi username", "multi password", "", ""),

      
      new LoginInfo("http://www6.example.com", "http://www6.example.com", null,
                    "", "12345", "", "form_field_password"),

      

      
      new LoginInfo("http://www.example.org", null, "The HTTP Realm",
                    "the username", "the password", "", ""),

      
      new LoginInfo("ftp://ftp.example.org", null, "ftp://ftp.example.org",
                    "the username", "the password", "", ""),

      
      new LoginInfo("http://www2.example.org", null, "The HTTP Realm",
                    "the username", "the password", "", ""),
      new LoginInfo("http://www2.example.org", null, "The HTTP Realm Other",
                    "the username other", "the password other", "", ""),

      

      new LoginInfo("http://example.net", "http://example.net", null,
                    "the username", "the password",
                    "form_field_username", "form_field_password"),
      new LoginInfo("http://example.net", "http://www.example.net", null,
                    "the username", "the password",
                    "form_field_username", "form_field_password"),
      new LoginInfo("http://example.net", "http://www.example.net", null,
                    "username two", "the password",
                    "form_field_username", "form_field_password"),
      new LoginInfo("http://example.net", null, "The HTTP Realm",
                    "the username", "the password", "", ""),
      new LoginInfo("http://example.net", null, "The HTTP Realm Other",
                    "username two", "the password", "", ""),
      new LoginInfo("ftp://example.net", null, "ftp://example.net",
                    "the username", "the password", "", ""),

      

      new LoginInfo("chrome://example_extension", null, "Example Login One",
                    "the username", "the password one", "", ""),
      new LoginInfo("chrome://example_extension", null, "Example Login Two",
                    "the username", "the password two", "", ""),
    ];
  },
};
