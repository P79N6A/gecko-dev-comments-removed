








thisTestLeaksUncaughtRejectionsAndShouldBeFixed("TypeError: window.location is null");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "fxAccounts",
  "resource://gre/modules/FxAccounts.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");

const CHROME_BASE = "chrome://mochitests/content/browser/browser/base/content/test/general/";

let changedPrefs = new Set();

function setPref(name, value) {
  changedPrefs.add(name);
  Services.prefs.setCharPref(name, value);
}

registerCleanupFunction(function() {
  
  for (let name of changedPrefs) {
    Services.prefs.clearUserPref(name);
  }
});

let gTests = [
{
  desc: "Test the remote commands",
  teardown: function* () {
    gBrowser.removeCurrentTab();
    yield fxAccounts.signOut();
  },
  run: function* ()
  {
    setPref("identity.fxaccounts.remote.signup.uri",
            "https://example.com/browser/browser/base/content/test/general/accounts_testRemoteCommands.html");
    yield promiseNewTabLoadEvent("about:accounts");

    let deferred = Promise.defer();

    let results = 0;
    try {
      let win = gBrowser.contentWindow;
      win.addEventListener("message", function testLoad(e) {
        if (e.data.type == "testResult") {
          ok(e.data.pass, e.data.info);
          results++;
        }
        else if (e.data.type == "testsComplete") {
          is(results, e.data.count, "Checking number of results received matches the number of tests that should have run");
          win.removeEventListener("message", testLoad, false, true);
          deferred.resolve();
        }

      }, false, true);

    } catch(e) {
      ok(false, "Failed to get all commands");
      deferred.reject();
    }
    yield deferred.promise;
  }
},
{
  desc: "Test action=signin - no user logged in",
  teardown: () => gBrowser.removeCurrentTab(),
  run: function* ()
  {
    
    const expected_url = "https://example.com/?is_sign_in";
    setPref("identity.fxaccounts.remote.signin.uri", expected_url);
    let [tab, url] = yield promiseNewTabWithIframeLoadEvent("about:accounts?action=signin");
    is(url, expected_url, "action=signin got the expected URL");
    
    yield checkVisibilities(tab, {
      stage: false, 
      manage: false,
      intro: false, 
      remote: true
    });
  }
},
{
  desc: "Test action=signin - user logged in",
  teardown: function* () {
    gBrowser.removeCurrentTab();
    yield signOut();
  },
  run: function* ()
  {
    
    
    const expected_url = "https://example.com/?is_sign_in";
    setPref("identity.fxaccounts.remote.signin.uri", expected_url);
    yield setSignedInUser();
    let tab = yield promiseNewTabLoadEvent("about:accounts?action=signin");
    
    
    
    yield fxAccounts.getSignedInUser();
    
    yield checkVisibilities(tab, {
      stage: true, 
      manage: true,
      intro: false, 
      remote: false
    });
  }
},
{
  desc: "Test action=signup - no user logged in",
  teardown: () => gBrowser.removeCurrentTab(),
  run: function* ()
  {
    const expected_url = "https://example.com/?is_sign_up";
    setPref("identity.fxaccounts.remote.signup.uri", expected_url);
    let [tab, url] = yield promiseNewTabWithIframeLoadEvent("about:accounts?action=signup");
    is(url, expected_url, "action=signup got the expected URL");
    
    yield checkVisibilities(tab, {
      stage: false, 
      manage: false,
      intro: false, 
      remote: true
    });
  },
},
{
  desc: "Test action=signup - user logged in",
  teardown: () => gBrowser.removeCurrentTab(),
  run: function* ()
  {
    const expected_url = "https://example.com/?is_sign_up";
    setPref("identity.fxaccounts.remote.signup.uri", expected_url);
    yield setSignedInUser();
    let tab = yield promiseNewTabLoadEvent("about:accounts?action=signup");
    yield fxAccounts.getSignedInUser();
    
    yield checkVisibilities(tab, {
      stage: true, 
      manage: true,
      intro: false, 
      remote: false
    });
  },
},
{
  desc: "Test action=reauth",
  teardown: function* () {
    gBrowser.removeCurrentTab();
    yield signOut();
  },
  run: function* ()
  {
    const expected_url = "https://example.com/?is_force_auth";
    setPref("identity.fxaccounts.remote.force_auth.uri", expected_url);
    let userData = {
      email: "foo@example.com",
      uid: "1234@lcip.org",
      assertion: "foobar",
      sessionToken: "dead",
      kA: "beef",
      kB: "cafe",
      verified: true
    };

    yield setSignedInUser();
    let [tab, url] = yield promiseNewTabWithIframeLoadEvent("about:accounts?action=reauth");
    
    let expected = expected_url + "&email=foo%40example.com";
    is(url, expected, "action=reauth got the expected URL");
  },
},
{
  desc: "Test with migrateToDevEdition enabled (success)",
  teardown: function* () {
    gBrowser.removeCurrentTab();
    yield signOut();
  },
  run: function* ()
  {
    let fxAccountsCommon = {};
    Cu.import("resource://gre/modules/FxAccountsCommon.js", fxAccountsCommon);
    const pref = "identity.fxaccounts.migrateToDevEdition";
    changedPrefs.add(pref);
    Services.prefs.setBoolPref(pref, true);

    
    
    let signedInUser = {
      version: 1,
      accountData: {
        email: "foo@example.com",
        uid: "1234@lcip.org",
        sessionToken: "dead",
        verified: true
      }
    };
    
    
    let profD = Services.dirsvc.get("ProfD", Ci.nsIFile);
    let mockDir = profD.clone();
    mockDir.append("about-accounts-mock-profd");
    mockDir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
    let fxAccountsStorage = OS.Path.join(mockDir.path, fxAccountsCommon.DEFAULT_STORAGE_FILENAME);
    yield OS.File.writeAtomic(fxAccountsStorage, JSON.stringify(signedInUser));
    info("Wrote file " + fxAccountsStorage);

    
    
    
    let tab = yield promiseNewTabLoadEvent("about:blank?");
    let readyPromise = promiseOneMessage(tab, "test:load-with-mocked-profile-path-response");

    let mm = tab.linkedBrowser.messageManager;
    mm.sendAsyncMessage("test:load-with-mocked-profile-path", {
      url: "about:accounts",
      profilePath: mockDir.path,
    });

    let response = yield readyPromise;
    
    let expected = yield fxAccounts.promiseAccountsForceSigninURI();
    is(response.data.url, expected);

    let userData = yield fxAccounts.getSignedInUser();
    SimpleTest.isDeeply(userData, signedInUser.accountData, "All account data were migrated");
    
    is(Services.prefs.getBoolPref(pref), false, pref + " got the expected value");

    yield OS.File.remove(fxAccountsStorage);
    yield OS.File.removeEmptyDir(mockDir.path);
  },
},
{
  desc: "Test with migrateToDevEdition enabled (no user to migrate)",
  teardown: function* () {
    gBrowser.removeCurrentTab();
    yield signOut();
  },
  run: function* ()
  {
    const pref = "identity.fxaccounts.migrateToDevEdition";
    changedPrefs.add(pref);
    Services.prefs.setBoolPref(pref, true);

    let profD = Services.dirsvc.get("ProfD", Ci.nsIFile);
    let mockDir = profD.clone();
    mockDir.append("about-accounts-mock-profd");
    mockDir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
    

    let tab = yield promiseNewTabLoadEvent("about:blank?");
    let readyPromise = promiseOneMessage(tab, "test:load-with-mocked-profile-path-response");

    let mm = tab.linkedBrowser.messageManager;
    mm.sendAsyncMessage("test:load-with-mocked-profile-path", {
      url: "about:accounts",
      profilePath: mockDir.path,
    });

    let response = yield readyPromise;
    
    let expected = fxAccounts.getAccountsSignUpURI();
    is(response.data.url, expected);

    
    let userData = yield fxAccounts.getSignedInUser();
    is(userData, null);
    
    is(Services.prefs.getBoolPref(pref), false, pref + " got the expected value");
    yield OS.File.removeEmptyDir(mockDir.path);
  },
},
{
  desc: "Test observers about:accounts",
  teardown: function() {
    gBrowser.removeCurrentTab();
  },
  run: function* () {
    setPref("identity.fxaccounts.remote.signup.uri", "https://example.com/");
    yield setSignedInUser();
    let tab = yield promiseNewTabLoadEvent("about:accounts");
    
    yield signOut();
    
    yield promiseOneMessage(tab, "test:document:load");
    is(tab.linkedBrowser.contentDocument.location.href, "about:accounts?action=signin");
  }
},
{
  desc: "Test entrypoint query string, no action, no user logged in",
  teardown: () => gBrowser.removeCurrentTab(),
  run: function* () {
    
    setPref("identity.fxaccounts.remote.signup.uri", "https://example.com/");
    let [tab, url] = yield promiseNewTabWithIframeLoadEvent("about:accounts?entrypoint=abouthome");
    is(url, "https://example.com/?entrypoint=abouthome", "entrypoint=abouthome got the expected URL");
  },
},
{
  desc: "Test entrypoint query string for signin",
  teardown: () => gBrowser.removeCurrentTab(),
  run: function* () {
    
    const expected_url = "https://example.com/?is_sign_in";
    setPref("identity.fxaccounts.remote.signin.uri", expected_url);
    let [tab, url] = yield promiseNewTabWithIframeLoadEvent("about:accounts?action=signin&entrypoint=abouthome");
    is(url, expected_url + "&entrypoint=abouthome", "entrypoint=abouthome got the expected URL");
  },
},
{
  desc: "Test entrypoint query string for signup",
  teardown: () => gBrowser.removeCurrentTab(),
  run: function* () {
    
    const sign_up_url = "https://example.com/?is_sign_up";
    setPref("identity.fxaccounts.remote.signup.uri", sign_up_url);
    let [tab, url] = yield promiseNewTabWithIframeLoadEvent("about:accounts?entrypoint=abouthome&action=signup");
    is(url, sign_up_url + "&entrypoint=abouthome", "entrypoint=abouthome got the expected URL");
  },
},
{
  desc: "about:accounts URL params should be copied to remote URL params " +
        "when remote URL has no URL params, except for 'action'",
  teardown() {
    gBrowser.removeCurrentTab();
  },
  run: function* () {
    let signupURL = "https://example.com/";
    setPref("identity.fxaccounts.remote.signup.uri", signupURL);
    let queryStr = "email=foo%40example.com&foo=bar&baz=quux";
    let [tab, url] =
      yield promiseNewTabWithIframeLoadEvent("about:accounts?" + queryStr +
                                             "&action=action");
    is(url, signupURL + "?" + queryStr, "URL params are copied to signup URL");
  },
},
{
  desc: "about:accounts URL params should be copied to remote URL params " +
        "when remote URL already has some URL params, except for 'action'",
  teardown() {
    gBrowser.removeCurrentTab();
  },
  run: function* () {
    let signupURL = "https://example.com/?param";
    setPref("identity.fxaccounts.remote.signup.uri", signupURL);
    let queryStr = "email=foo%40example.com&foo=bar&baz=quux";
    let [tab, url] =
      yield promiseNewTabWithIframeLoadEvent("about:accounts?" + queryStr +
                                             "&action=action");
    is(url, signupURL + "&" + queryStr, "URL params are copied to signup URL");
  },
},
]; 

function test()
{
  waitForExplicitFinish();

  Task.spawn(function () {
    for (let test of gTests) {
      info(test.desc);
      try {
        yield test.run();
      } finally {
        yield test.teardown();
      }
    }

    finish();
  });
}

function promiseOneMessage(tab, messageName) {
  let mm = tab.linkedBrowser.messageManager;
  let deferred = Promise.defer();
  mm.addMessageListener(messageName, function onmessage(message) {
    mm.removeMessageListener(messageName, onmessage);
    deferred.resolve(message);
  });
  return deferred.promise;
}

function promiseNewTabLoadEvent(aUrl)
{
  let tab = gBrowser.selectedTab = gBrowser.addTab(aUrl);
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;

  
  mm.loadFrameScript(CHROME_BASE + "content_aboutAccounts.js", true);
  
  return promiseOneMessage(tab, "test:document:load").then(
    () => tab
  );
}



function promiseNewTabWithIframeLoadEvent(aUrl) {
  let deferred = Promise.defer();
  let tab = gBrowser.selectedTab = gBrowser.addTab(aUrl);
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;

  
  mm.loadFrameScript(CHROME_BASE + "content_aboutAccounts.js", true);
  
  mm.addMessageListener("test:iframe:load", function onFrameLoad(message) {
    mm.removeMessageListener("test:iframe:load", onFrameLoad);
    deferred.resolve([tab, message.data.url]);
  });
  return deferred.promise;
}

function checkVisibilities(tab, data) {
  let ids = Object.keys(data);
  let mm = tab.linkedBrowser.messageManager;
  let deferred = Promise.defer();
  mm.addMessageListener("test:check-visibilities-response", function onResponse(message) {
    mm.removeMessageListener("test:check-visibilities-response", onResponse);
    for (let id of ids) {
      is(message.data[id], data[id], "Element '" + id + "' has correct visibility");
    }
    deferred.resolve();
  });
  mm.sendAsyncMessage("test:check-visibilities", {ids: ids});
  return deferred.promise;
}



function setSignedInUser(data) {
  if (!data) {
    data = {
      email: "foo@example.com",
      uid: "1234@lcip.org",
      assertion: "foobar",
      sessionToken: "dead",
      kA: "beef",
      kB: "cafe",
      verified: true
    }
  }
 return fxAccounts.setSignedInUser(data);
}

function signOut() {
  
  return fxAccounts.signOut(true);
}
