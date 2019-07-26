


"use strict";

let tmp = {};
Cu.import("resource:///modules/translation/Translation.jsm", tmp);
let {Translation} = tmp;

let MetricsChecker = {
  _metricsTime: new Date(),
  _midnightError: new Error("Getting metrics around midnight may fail sometimes"),

  updateMetrics: Task.async(function* () {
    let svc = Cc["@mozilla.org/datareporting/service;1"].getService();
    let reporter = svc.wrappedJSObject.healthReporter;
    yield reporter.onInit();

    
    let provider = reporter.getProvider("org.mozilla.translation");
    let measurement = provider.getMeasurement("translation", 1);
    let values = yield measurement.getValues();

    let metricsTime = new Date();
    let day = values.days.getDay(metricsTime);
    if (!day) {
      
      throw this._midnightError;
    }

    
    this._metrics = {
      pageCount: day.get("pageTranslatedCount") || 0,
      charCount: day.get("charactersTranslatedCount") || 0,
      deniedOffers: day.get("deniedTranslationOffer") || 0
    };
    this._metricsTime = metricsTime;
  }),

  checkAdditions: Task.async(function* (additions) {
    let prevMetrics = this._metrics, prevMetricsTime = this._metricsTime;
    try {
      yield this.updateMetrics();
    } catch(ex if ex == this._midnightError) {
      return;
    }

    
    
    
    if (this._metricsTime.getDate() != prevMetricsTime.getDate()) {
      for (let metric of Object.keys(prevMetrics)) {
        prevMetrics[metric] = 0;
      }
    }

    for (let metric of Object.keys(additions)) {
      Assert.equal(prevMetrics[metric] + additions[metric], this._metrics[metric]);
    }
  })
};
add_task(function* setup() {
  Services.prefs.setBoolPref("toolkit.telemetry.enabled", true);
  Services.prefs.setBoolPref("browser.translation.detectLanguage", true);
  Services.prefs.setBoolPref("browser.translation.ui.show", true);

  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("toolkit.telemetry.enabled");
    Services.prefs.clearUserPref("browser.translation.detectLanguage");
    Services.prefs.clearUserPref("browser.translation.ui.show");
  });

  yield MetricsChecker.updateMetrics();
});

add_task(function* test_fhr() {
  
  yield translate("<h1>Hallo Welt!</h1>", "de", "en");

  
  yield translate("<h1>Hallo Welt!</h1><h1>Bratwurst!</h1>", "de", "en");
  yield MetricsChecker.checkAdditions({ pageCount: 1, charCount: 21, deniedOffers: 0});
});

add_task(function* test_deny_translation_metric() {
  function* offerAndDeny(elementAnonid) {
    let tab = yield offerTranslatationFor("<h1>Hallo Welt!</h1>", "de", "en");
    getInfobarElement(tab.linkedBrowser, elementAnonid).doCommand();
    yield MetricsChecker.checkAdditions({ deniedOffers: 1 });
    gBrowser.removeTab(tab);
  }

  yield offerAndDeny("notNow");
  yield offerAndDeny("neverForSite");
  yield offerAndDeny("neverForLanguage");
  yield offerAndDeny("closeButton");

  
  
  let tab =
    yield translate("<h1>Hallo Welt!</h1>", "de", "en", false);
  yield MetricsChecker.checkAdditions({ deniedOffers: 0 });
  gBrowser.removeTab(tab);
});

function getInfobarElement(browser, anonid) {
  let notif = browser.translationUI
                     .notificationBox.getNotificationWithValue("translation");
  return notif._getAnonElt(anonid);
}

function offerTranslatationFor(text, from) {
  return Task.spawn(function* task_offer_translation() {
    
    let tab = gBrowser.selectedTab =
      gBrowser.addTab("data:text/html;charset=utf-8," + text);

    
    let browser = tab.linkedBrowser;
    yield promiseBrowserLoaded(browser);

    
    Translation.documentStateReceived(browser, {state: Translation.STATE_OFFER,
                                                originalShown: true,
                                                detectedLanguage: from});

    return tab;
  });
}

function acceptTranslationOffer(tab, to) {
  return Task.spawn(function* task_accept_translation_offer() {
    let browser = tab.linkedBrowser;
    getInfobarElement(browser, "toLanguage").value = to;
    getInfobarElement(browser, "toLanguage").doCommand();
    yield waitForMessage(browser, "Translation:Finished");
  });
}

function translate(text, from, to, closeTab = true) {
  return Task.spawn(function* task_translate() {
    let tab = yield offerTranslatationFor(text, from);
    yield acceptTranslationOffer(tab, to);
    if (closeTab) {
      gBrowser.removeTab(tab);
    } else {
      return tab;
    }
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
