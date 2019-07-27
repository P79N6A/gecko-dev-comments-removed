





"use strict";

const kEnginePref = "browser.translation.engine";
const kApiKeyPref = "browser.translation.yandex.apiKeyOverride";

const {YandexTranslator} = Cu.import("resource:///modules/translation/YandexTranslator.jsm", {});
const {TranslationDocument} = Cu.import("resource:///modules/translation/TranslationDocument.jsm", {});
const {Promise} = Cu.import("resource://gre/modules/Promise.jsm", {});

add_task(function* setup() {
  Services.prefs.setCharPref(kEnginePref, "yandex");
  Services.prefs.setCharPref(kApiKeyPref, "yandexValidKey");

  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(kEnginePref);
    Services.prefs.clearUserPref(kApiKeyPref);
  });
});





add_task(function* test_yandex_translation() {

  
  let url = constructFixtureURL("bug1022725-fr.html");
  let tab = yield promiseTestPageLoad(url);

  
  gBrowser.selectedTab = tab;
  let browser = tab.linkedBrowser;
  let client = new YandexTranslator(
    new TranslationDocument(browser.contentDocument), "fr", "en");
  let result = yield client.translate();

  Assert.ok(result, "There should be a result.");

  gBrowser.removeTab(tab);
});







function constructFixtureURL(filename){
  
  let server = Services.prefs.getCharPref("browser.translation.yandex.translateURLOverride")
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
