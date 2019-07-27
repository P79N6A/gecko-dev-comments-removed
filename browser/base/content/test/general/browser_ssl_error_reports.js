"use strict";

let badChainURL = "https://badchain.include-subdomains.pinning.example.com";
let noCertURL = "https://fail-handshake.example.com";
let enabledPref = false;
let automaticPref = false;
let urlPref = "security.ssl.errorReporting.url";
let enforcement_level = 1;
let ROOT = getRootDirectory(gTestPath);

SimpleTest.requestCompleteLog();

add_task(function* test_send_report_manual_badchain() {
  yield testSendReportManual(badChainURL, "succeed");
});

add_task(function* test_send_report_manual_nocert() {
  yield testSendReportManual(noCertURL, "nocert");
});


function createNetworkErrorMessagePromise(aBrowser) {
  let progressListener;
  let promise = new Promise(function(resolve, reject) {
    
    let originalDocumentURI = aBrowser.contentDocument.documentURI;

    progressListener = {
      onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags) {
        
        if (!(aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_ERROR_PAGE)) {
          reject("location change was not to an error page");
        }
      },

      onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
        let doc = aBrowser.contentDocument;

        if (doc && doc.getElementById("reportCertificateError")) {
          
          
          let documentURI = doc.documentURI;
          if (documentURI == originalDocumentURI) {
            return;
          }

          aWebProgress.removeProgressListener(progressListener,
            Ci.nsIWebProgress.NOTIFY_LOCATION |
            Ci.nsIWebProgress.NOTIFY_STATE_REQUEST);
          let matchArray = /about:neterror\?.*&d=([^&]*)/.exec(documentURI);
          if (!matchArray) {
            reject("no network error message found in URI")
          return;
          }

          let errorMsg = matchArray[1];
          resolve(decodeURIComponent(errorMsg));
        }
      },

      QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                          Ci.nsISupportsWeakReference])
    };

    aBrowser.addProgressListener(progressListener,
            Ci.nsIWebProgress.NOTIFY_LOCATION |
            Ci.nsIWebProgress.NOTIFY_STATE_REQUEST);
  });

  
  createNetworkErrorMessagePromise.listeners.set(promise, progressListener);

  return promise;
}



createNetworkErrorMessagePromise.listeners = new WeakMap();


add_task(function* test_set_automatic() {
  setup();
  let tab = gBrowser.addTab(badChainURL, {skipAnimation: true});
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;
  mm.loadFrameScript(ROOT + "browser_ssl_error_reports_content.js", true);

  gBrowser.selectedTab = tab;

  
  let netError = createNetworkErrorMessagePromise(browser);
  yield netError;

  
  let prefEnabled = new Promise(function(resolve, reject){
    let prefUpdateListener = function() {
      mm.removeMessageListener("ssler-test:AutoPrefUpdated", prefUpdateListener);
      if (Services.prefs.getBoolPref("security.ssl.errorReporting.automatic")) {
        resolve();
      } else {
        reject();
      }
    };
    mm.addMessageListener("ssler-test:AutoPrefUpdated", prefUpdateListener);
  });

  mm.sendAsyncMessage("ssler-test:SetAutoPref",{value:true});

  yield prefEnabled;

  
  let prefDisabled = new Promise(function(resolve, reject){
    let prefUpdateListener = function () {
      mm.removeMessageListener("ssler-test:AutoPrefUpdated", prefUpdateListener);
      if (!Services.prefs.getBoolPref("security.ssl.errorReporting.automatic")) {
        resolve();
      } else {
        reject();
      }
    };
    mm.addMessageListener("ssler-test:AutoPrefUpdated", prefUpdateListener);
  });

  mm.sendAsyncMessage("ssler-test:SetAutoPref",{value:false});

  yield prefDisabled;

  gBrowser.removeTab(tab);
  cleanup();
});


let testSendReportManual = function*(testURL, suffix) {
  setup();
  Services.prefs.setBoolPref("security.ssl.errorReporting.enabled", true);
  Services.prefs.setCharPref("security.ssl.errorReporting.url",
    "https://example.com/browser/browser/base/content/test/general/pinning_reports.sjs?" + suffix);

  let tab = gBrowser.addTab(testURL, {skipAnimation: true});
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;
  mm.loadFrameScript(ROOT + "browser_ssl_error_reports_content.js", true);

  gBrowser.selectedTab = tab;

  
  let netError = createNetworkErrorMessagePromise(browser);
  yield netError;
  netError.then(function(val){
    is(val.startsWith("An error occurred during a connection to"), true,
                      "ensure the correct error message came from about:neterror");
  });

  let btn = browser.contentDocument.getElementById("reportCertificateError");
  let deferredReportSucceeds = Promise.defer();

  
  let statusListener = function() {
    let active = false;
    return function(message) {
      switch(message.data.reportStatus) {
        case "activity":
          if (!active) {
            active = true;
          }
          break;
        case "complete":
          mm.removeMessageListener("ssler-test:SSLErrorReportStatus", statusListener);
          if (active) {
            deferredReportSucceeds.resolve(message.data.reportStatus);
          } else {
            deferredReportSucceeds.reject('activity should be seen before success');
          }
          break;
        case "error":
          mm.removeMessageListener("ssler-test:SSLErrorReportStatus", statusListener);
          deferredReportSucceeds.reject();
          break;
      }
    };
  }();
  mm.addMessageListener("ssler-test:SSLErrorReportStatus", statusListener);

  
  mm.sendAsyncMessage("ssler-test:SendBtnClick",{});

  yield deferredReportSucceeds.promise;

  gBrowser.removeTab(tab);
  cleanup();
};


add_task(function* test_send_report_auto() {
  setup();
  Services.prefs.setBoolPref("security.ssl.errorReporting.enabled", true);
  Services.prefs.setBoolPref("security.ssl.errorReporting.automatic", true);
  Services.prefs.setCharPref("security.ssl.errorReporting.url", "https://example.com/browser/browser/base/content/test/general/pinning_reports.sjs?succeed");

  let tab = gBrowser.addTab(badChainURL, {skipAnimation: true});
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;
  mm.loadFrameScript(ROOT + "browser_ssl_error_reports_content.js", true);

  gBrowser.selectedTab = tab;


  
  let netError = createNetworkErrorMessagePromise(browser);
  yield netError;

  let reportWillStart = Promise.defer();
  let startListener = function() {
    mm.removeMessageListener("Browser:SendSSLErrorReport", startListener);
    reportWillStart.resolve();
  };
  mm.addMessageListener("Browser:SendSSLErrorReport", startListener);

  let deferredReportSucceeds = Promise.defer();

  let statusListener = function(message) {
    switch(message.data.reportStatus) {
      case "complete":
        mm.removeMessageListener("ssler-test:SSLErrorReportStatus", statusListener);
        deferredReportSucceeds.resolve(message.data.reportStatus);
        break;
      case "error":
        mm.removeMessageListener("ssler-test:SSLErrorReportStatus", statusListener);
        deferredReportSucceeds.reject();
        break;
    }
  };

  mm.addMessageListener("ssler-test:SSLErrorReportStatus", statusListener);

  
  yield deferredReportSucceeds.promise;

  gBrowser.removeTab(tab);
  cleanup();
});


add_task(function* test_send_report_error() {
  setup();
  
  Services.prefs.setBoolPref("security.ssl.errorReporting.enabled", true);
  Services.prefs.setBoolPref("security.ssl.errorReporting.automatic", true);
  Services.prefs.setCharPref("security.ssl.errorReporting.url", "https://example.com/browser/browser/base/content/test/general/pinning_reports.sjs?error");

  
  let tab = gBrowser.addTab(badChainURL, {skipAnimation: true});
  let browser = tab.linkedBrowser;
  gBrowser.selectedTab = tab;
  let mm = browser.messageManager;
  mm.loadFrameScript(ROOT + "browser_ssl_error_reports_content.js", true);

  let reportErrors = new Promise(function(resolve, reject) {
    let statusListener = function(message) {
      switch(message.data.reportStatus) {
        case "complete":
          reject(message.data.reportStatus);
          mm.removeMessageListener("ssler-test:SSLErrorReportStatus", statusListener);
          break;
        case "error":
          resolve(message.data.reportStatus);
          mm.removeMessageListener("ssler-test:SSLErrorReportStatus", statusListener);
          break;
      }
    };
    mm.addMessageListener("ssler-test:SSLErrorReportStatus", statusListener);
  });

  
  yield reportErrors;

  gBrowser.removeTab(tab);
  cleanup();
});

add_task(function* test_send_report_disabled() {
  setup();
  Services.prefs.setBoolPref("security.ssl.errorReporting.enabled", false);
  Services.prefs.setCharPref("security.ssl.errorReporting.url", "https://offdomain.com");

  let tab = gBrowser.addTab(badChainURL, {skipAnimation: true});
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;
  mm.loadFrameScript(ROOT + "browser_ssl_error_reports_content.js", true);

  gBrowser.selectedTab = tab;

  
  let netError = createNetworkErrorMessagePromise(browser);
  yield netError;

  let reportErrors = new Promise(function(resolve, reject) {
    let statusListener = function(message) {
      switch(message.data.reportStatus) {
        case "complete":
          mm.removeMessageListener("ssler-test:SSLErrorReportStatus", statusListener);
          reject(message.data.reportStatus);
          break;
        case "error":
          mm.removeMessageListener("ssler-test:SSLErrorReportStatus", statusListener);
          resolve(message.data.reportStatus);
          break;
      }
    };
    mm.addMessageListener("ssler-test:SSLErrorReportStatus", statusListener);
  });

  
  mm.sendAsyncMessage("ssler-test:SendBtnClick",{forceUI:true});

  
  yield reportErrors;

  gBrowser.removeTab(tab);
  cleanup();
});

function setup() {
  
  enabledPref = Services.prefs.getBoolPref("security.ssl.errorReporting.enabled");
  automaticPref = Services.prefs.getBoolPref("security.ssl.errorReporting.automatic");
  urlPref = Services.prefs.getCharPref("security.ssl.errorReporting.url");

  enforcement_level = Services.prefs.getIntPref("security.cert_pinning.enforcement_level");
  Services.prefs.setIntPref("security.cert_pinning.enforcement_level", 2);
}

function cleanup() {
  
  Services.prefs.setBoolPref("security.ssl.errorReporting.enabled", enabledPref);
  Services.prefs.setBoolPref("security.ssl.errorReporting.automatic", automaticPref);
  Services.prefs.setCharPref("security.ssl.errorReporting.url", urlPref);
}
