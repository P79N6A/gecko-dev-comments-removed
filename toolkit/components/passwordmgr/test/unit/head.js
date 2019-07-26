








"use strict";




const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");

const LoginInfo =
      Components.Constructor("@mozilla.org/login-manager/loginInfo;1",
                             "nsILoginInfo", "init");




function run_test()
{
  do_get_profile();
  run_next_test();
}

















function promiseTopicObserved(aTopic)
{
  let deferred = Promise.defer();

  Services.obs.addObserver(
    function PTO_observe(aSubject, aTopic, aData) {
      Services.obs.removeObserver(PTO_observe, aTopic);
      deferred.resolve([aSubject, aData]);
    }, aTopic, false);

  return deferred.promise;
}











function newPropertyBag(aProperties)
{
  let propertyBag = Cc["@mozilla.org/hash-property-bag;1"]
                      .createInstance(Ci.nsIWritablePropertyBag);
  if (aProperties) {
    for (let [name, value] of Iterator(aProperties)) {
      propertyBag.setProperty(name, value);
    }
  }
  return propertyBag.QueryInterface(Ci.nsIPropertyBag)
                    .QueryInterface(Ci.nsIPropertyBag2)
                    .QueryInterface(Ci.nsIWritablePropertyBag2);
}




const LoginTest = {
  



  reloadData: function ()
  {
    Services.obs.notifyObservers(null, "passwordmgr-storage-replace", null);
    yield promiseTopicObserved("passwordmgr-storage-replace-complete");
  },

  


  clearData: function ()
  {
    Services.logins.removeAllLogins();
    for (let hostname of Services.logins.getAllDisabledHosts()) {
      Services.logins.setLoginSavingEnabled(hostname, true);
    }
  },

  




  checkLogins: function (aExpectedLogins)
  {
    this.assertLoginListsEqual(Services.logins.getAllLogins(), aExpectedLogins);
  },

  





  assertLoginListsEqual: function (aActual, aExpected)
  {
    do_check_eq(aExpected.length, aActual.length);
    do_check_true(aExpected.every(e => aActual.some(a => a.equals(e))));
  },

  



  assertDisabledHostsEqual: function (aActual, aExpected)
  {
    Assert.deepEqual(aActual.sort(), aExpected.sort());
  },

  



  assertTimeIsAboutNow: function (aTimeMs)
  {
    do_check_true(Math.abs(aTimeMs - Date.now()) < 30000);
  }
};












const TestData = {
  






  formLogin: function (aModifications)
  {
    let loginInfo = new LoginInfo("http://www3.example.com",
                                  "http://www.example.com", null,
                                  "the username", "the password",
                                  "form_field_username", "form_field_password");
    loginInfo.QueryInterface(Ci.nsILoginMetaInfo);
    if (aModifications) {
      for (let [name, value] of Iterator(aModifications)) {
        loginInfo[name] = value;
      }
    }
    return loginInfo;
  },

  






  authLogin: function (aModifications)
  {
    let loginInfo = new LoginInfo("http://www.example.org", null,
                                  "The HTTP Realm", "the username",
                                  "the password", "", "");
    loginInfo.QueryInterface(Ci.nsILoginMetaInfo);
    if (aModifications) {
      for (let [name, value] of Iterator(aModifications)) {
        loginInfo[name] = value;
      }
    }
    return loginInfo;
  },

  



  loginList: function ()
  {
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
      new LoginInfo("http://www3.example.com", "https://www.example.com", null,
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




add_task(function test_common_initialize()
{
  
  
  
  yield OS.File.copy(do_get_file("data/key3.db").path,
                     OS.Path.join(OS.Constants.Path.profileDir, "key3.db"));

  
  yield Services.logins.initializationPromise;

  
  LoginTest.clearData();

  
  do_register_cleanup(() => LoginTest.clearData());
});
