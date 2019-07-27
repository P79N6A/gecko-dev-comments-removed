



"use strict";



let Preferences = Cu.import("resource://gre/modules/Preferences.jsm", {}).Preferences;
let PromiseUtils = Cu.import("resource://gre/modules/PromiseUtils.jsm", {}).PromiseUtils;
let SelfSupportBackend =
  Cu.import("resource:///modules/SelfSupportBackend.jsm", {}).SelfSupportBackend;

const PREF_SELFSUPPORT_ENABLED = "browser.selfsupport.enabled";
const PREF_SELFSUPPORT_URL = "browser.selfsupport.url";
const PREF_UITOUR_ENABLED = "browser.uitour.enabled";

const TEST_WAIT_RETRIES = 60;

const TEST_PAGE_URL = getRootDirectory(gTestPath) + "uitour.html";
const TEST_PAGE_URL_HTTPS = TEST_PAGE_URL.replace("chrome://mochitests/content/", "https://example.com/");








function findSelfSupportBrowser(aURL) {
  let frames = Services.appShell.hiddenDOMWindow.document.querySelectorAll('iframe');
  for (let frame of frames) {
    try {
      let browser = frame.contentDocument.getElementById("win").querySelectorAll('browser')[0];
      let url = browser.getAttribute("src");
      if (url == aURL) {
        return browser;
      }
    } catch (e) {
      continue;
    }
  }
  return null;
}









function promiseSelfSupportLoad(aURL) {
  return new Promise((resolve, reject) => {
    
    let browserPromise = waitForConditionPromise(() => !!findSelfSupportBrowser(aURL),
                                                 "SelfSupport browser not found.",
                                                 TEST_WAIT_RETRIES);

    
    browserPromise.then(() => {
      let browser = findSelfSupportBrowser(aURL);
      if (browser.contentDocument.readyState === "complete") {
        resolve(browser);
      } else {
        let handler = () => {
          browser.removeEventListener("load", handler, true);
          resolve(browser);
        };
        browser.addEventListener("load", handler, true);
      }
    }, reject);
  });
}









function promiseSelfSupportClose(aURL) {
  return waitForConditionPromise(() => !findSelfSupportBrowser(aURL),
                                 "SelfSupport browser is still open.", TEST_WAIT_RETRIES);
}




add_task(function* setupEnvironment() {
  
  
  SelfSupportBackend.uninit();

  
  
  let selfSupportEnabled = Preferences.get(PREF_SELFSUPPORT_ENABLED, true);
  let uitourEnabled = Preferences.get(PREF_UITOUR_ENABLED, false);
  let selfSupportURL = Preferences.get(PREF_SELFSUPPORT_URL, "");

  
  
  Preferences.set(PREF_SELFSUPPORT_ENABLED, true);
  Preferences.set(PREF_UITOUR_ENABLED, true);
  Preferences.set(PREF_SELFSUPPORT_URL, TEST_PAGE_URL_HTTPS);

  
  let pageURI = Services.io.newURI(TEST_PAGE_URL_HTTPS, null, null);
  Services.perms.add(pageURI, "uitour", Services.perms.ALLOW_ACTION);

  registerCleanupFunction(() => {
    Services.perms.remove("example.com", "uitour");
    Preferences.set(PREF_SELFSUPPORT_ENABLED, selfSupportEnabled);
    Preferences.set(PREF_UITOUR_ENABLED, uitourEnabled);
    Preferences.set(PREF_SELFSUPPORT_URL, selfSupportURL);
  });
});




add_task(function* test_selfSupport() {
  
  SelfSupportBackend.init();

  
  info("Sending sessionstore-windows-restored");
  Services.obs.notifyObservers(null, "sessionstore-windows-restored", null);

  
  info("Waiting for the SelfSupport local page to load.");
  let selfSupportBrowser = yield promiseSelfSupportLoad(TEST_PAGE_URL_HTTPS);
  Assert.ok(!!selfSupportBrowser, "SelfSupport browser must exist.");

  
  info("Testing access to the UITour API.");
  let contentWindow =
    Cu.waiveXrays(selfSupportBrowser.contentDocument.defaultView);
  let uitourAPI = contentWindow.Mozilla.UITour;

  
  let pingPromise = new Promise((resolve) => {
    uitourAPI.ping(resolve);
  });
  yield pingPromise;

  
  contentWindow.close();

  
  info("Waiting for the SelfSupport to close.");
  yield promiseSelfSupportClose(TEST_PAGE_URL_HTTPS);

  
  selfSupportBrowser = findSelfSupportBrowser(TEST_PAGE_URL_HTTPS);
  Assert.ok(!selfSupportBrowser, "SelfSupport browser must not exist.");

  
  
  SelfSupportBackend.uninit();
});




add_task(function* test_selfSupport_noHTTPS() {
  Preferences.set(PREF_SELFSUPPORT_URL, TEST_PAGE_URL);

  SelfSupportBackend.init();

  
  info("Sending sessionstore-windows-restored");
  Services.obs.notifyObservers(null, "sessionstore-windows-restored", null);

  
  let selfSupportBrowser = findSelfSupportBrowser(TEST_PAGE_URL);
  Assert.ok(!selfSupportBrowser, "SelfSupport browser must not exist.");

  
  
  SelfSupportBackend.uninit();
})
