





"use strict";

const kClientIdPref = "browser.translation.bing.clientIdOverride";
const kClientSecretPref = "browser.translation.bing.apiKeyOverride";

const {BingTranslator} = Cu.import("resource:///modules/translation/BingTranslator.jsm", {});
const {TranslationDocument} = Cu.import("resource:///modules/translation/TranslationDocument.jsm", {});
const {Promise} = Cu.import("resource://gre/modules/Promise.jsm", {});

add_task(function* setup() {
  Services.prefs.setCharPref(kClientIdPref, "testClient");
  Services.prefs.setCharPref(kClientSecretPref, "testSecret");

  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(kClientIdPref);
    Services.prefs.clearUserPref(kClientSecretPref);
  });
});




add_task(function* test_bing_translation() {

  
  Services.prefs.setCharPref(kClientIdPref, "testClient");

  
  let url = constructFixtureURL("bug1022725-fr.html");
  let tab = yield promiseTestPageLoad(url);

  
  gBrowser.selectedTab = tab;
  let browser = tab.linkedBrowser;
  let client = new BingTranslator(
    new TranslationDocument(browser.contentDocument), "fr", "en");
  let result = yield client.translate();

  
  Assert.ok(result, "There should be a result.");

  gBrowser.removeTab(tab);
});








add_task(function* test_handling_out_of_valid_key_error() {

  
  Services.prefs.setCharPref(kClientIdPref, "testInactive");

  
  let url = constructFixtureURL("bug1022725-fr.html");
  let tab = yield promiseTestPageLoad(url);

  
  gBrowser.selectedTab = tab;
  let browser = tab.linkedBrowser;
  let client = new BingTranslator(
    new TranslationDocument(browser.contentDocument), "fr", "en");
  client._resetToken();
  try {
    yield client.translate();
  } catch (ex) {
    
  }
  client._resetToken();

  
  Assert.ok(client._serviceUnavailable, "Service should be detected unavailable.");

  
  Services.prefs.setCharPref(kClientIdPref, "testClient");
  gBrowser.removeTab(tab);
});







function constructFixtureURL(filename){
  
  let server = Services.prefs.getCharPref("browser.translation.bing.authURL")
                             .replace("http://", "");
  server = server.substr(0, server.indexOf("/"));
  let url = "http://" + server +
    "/browser/browser/components/translation/test/fixtures/" + filename;
  return url;
}






function promiseTestPageLoad(url) {
  let deferred = Promise.defer();
  let tab = gBrowser.selectedTab = gBrowser.addTab(url);
  let browser = gBrowser.selectedBrowser;
  browser.addEventListener("load", function listener() {
    if (browser.currentURI.spec == "about:blank")
      return;
    info("Page loaded: " + browser.currentURI.spec);
    browser.removeEventListener("load", listener, true);
    deferred.resolve(tab);
  }, true);
  return deferred.promise;
}
