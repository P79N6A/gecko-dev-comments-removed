





"use strict";

const kClientIdPref = "browser.translation.bing.clientIdOverride";
const kClientSecretPref = "browser.translation.bing.apiKeyOverride";

const {BingTranslator} = Cu.import("resource:///modules/translation/BingTranslator.jsm", {});
const {TranslationDocument} = Cu.import("resource:///modules/translation/TranslationDocument.jsm", {});

function test() {
  waitForExplicitFinish();

  Services.prefs.setCharPref(kClientIdPref, "testClient");
  Services.prefs.setCharPref(kClientSecretPref, "testSecret");

  
  let server = Services.prefs.getCharPref("browser.translation.bing.authURL")
                             .replace("http://", "");
  server = server.substr(0, server.indexOf("/"));
  let tab = gBrowser.addTab("http://" + server +
    "/browser/browser/components/translation/test/fixtures/bug1022725-fr.html");
  gBrowser.selectedTab = tab;

  registerCleanupFunction(function () {
    gBrowser.removeTab(tab);
    Services.prefs.clearUserPref(kClientIdPref);
    Services.prefs.clearUserPref(kClientSecretPref);
  });

  let browser = tab.linkedBrowser;
  browser.addEventListener("load", function onload() {
    if (browser.currentURI.spec == "about:blank")
      return;

    browser.removeEventListener("load", onload, true);
    let client = new BingTranslator(
      new TranslationDocument(browser.contentDocument), "fr", "en");

    client.translate().then(
      result => {
        
        ok(result, "There should be a result.");
        finish();
      },
      error => {
        ok(false, "Unexpected Client Error: " + error);
        finish();
      }
    );
  }, true);
}
