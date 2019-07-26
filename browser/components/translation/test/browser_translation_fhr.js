


"use strict";

let tmp = {};
Cu.import("resource:///modules/translation/Translation.jsm", tmp);
let {Translation} = tmp;

add_task(function* setup() {
  Services.prefs.setBoolPref("toolkit.telemetry.enabled", true);
  Services.prefs.setBoolPref("browser.translation.detectLanguage", true);
  Services.prefs.setBoolPref("browser.translation.ui.show", true);

  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("toolkit.telemetry.enabled");
    Services.prefs.clearUserPref("browser.translation.detectLanguage");
    Services.prefs.clearUserPref("browser.translation.ui.show");
  });
});

add_task(function* test_fhr() {
  let start = new Date();

  
  yield translate("<h1>Hallo Welt!</h1>", "de", "en");
  let [pageCount, charCount] = yield retrieveTranslationCounts();

  
  yield translate("<h1>Hallo Welt!</h1><h1>Bratwurst!</h1>", "de", "en");

  let [pageCount2, charCount2] = yield retrieveTranslationCounts();

  
  
  
  if (start.getDate() == new Date().getDate()) {
    Assert.equal(pageCount2, pageCount + 1);
    Assert.equal(charCount2, charCount + 21);
  }
});

function retrieveTranslationCounts() {
  return Task.spawn(function* task_retrieve_counts() {
    let svc = Cc["@mozilla.org/datareporting/service;1"].getService();
    let reporter = svc.wrappedJSObject.healthReporter;
    yield reporter.onInit();

    
    let provider = reporter.getProvider("org.mozilla.translation");
    let measurement = provider.getMeasurement("translation", 1);
    let values = yield measurement.getValues();

    let day = values.days.getDay(new Date());
    if (!day) {
      
      return [0, 0];
    }

    return [day.get("pageTranslatedCount"), day.get("charactersTranslatedCount")];
  });
}

function translate(text, from, to) {
  return Task.spawn(function* task_translate() {
    
    let tab = gBrowser.selectedTab =
      gBrowser.addTab("data:text/html;charset=utf-8," + text);

    
    let browser = tab.linkedBrowser;
    yield promiseBrowserLoaded(browser);

    
    Translation.documentStateReceived(browser, {state: Translation.STATE_OFFER,
                                                originalShown: true,
                                                detectedLanguage: from});

    
    browser.translationUI.translate(from, to);
    yield waitForMessage(browser, "Translation:Finished");

    
    gBrowser.removeTab(tab);
  });
}

function waitForMessage({messageManager}, name) {
  return new Promise(resolve => {
    messageManager.addMessageListener(name, function onMessage() {
      messageManager.removeMessageListener(name, onMessage);
      resolve();
    });
  });
}

function promiseBrowserLoaded(browser) {
  return new Promise(resolve => {
    browser.addEventListener("load", function onLoad(event) {
      if (event.target == browser.contentDocument) {
        browser.removeEventListener("load", onLoad, true);
        resolve();
      }
    }, true);
  });
}
