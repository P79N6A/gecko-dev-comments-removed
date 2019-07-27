var badPin = "https://include-subdomains.pinning.example.com";
var enabledPref = false;
var automaticPref = false;
var urlPref = "security.ssl.errorReporting.url";
var enforcement_level = 1;

function loadFrameScript() {
  let mm = Cc["@mozilla.org/globalmessagemanager;1"]
           .getService(Ci.nsIMessageListenerManager);
  const ROOT = getRootDirectory(gTestPath);
  mm.loadFrameScript(ROOT+"browser_bug846489_content.js", true);
}

add_task(function*(){
  waitForExplicitFinish();
  loadFrameScript();
  SimpleTest.requestCompleteLog();
  yield testSendReportDisabled();
  yield testSendReportManual();
  yield testSendReportAuto();
  yield testSendReportError();
  yield testSetAutomatic();
});


function createNetworkErrorMessagePromise(aBrowser) {
  return new Promise(function(resolve, reject) {
    
    var originalDocumentURI = aBrowser.contentDocument.documentURI;

    var progressListener = {
      onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags) {
        
        if (!(aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_ERROR_PAGE)) {
          reject("location change was not to an error page");
        }
      },

      onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
        let doc = aBrowser.contentDocument;

        if (doc.getElementById("reportCertificateError")) {
          
          
          var documentURI = doc.documentURI;
          if (documentURI == originalDocumentURI) {
            return;
          }

          aWebProgress.removeProgressListener(progressListener,
            Ci.nsIWebProgress.NOTIFY_LOCATION |
            Ci.nsIWebProgress.NOTIFY_STATE_REQUEST);
          var matchArray = /about:neterror\?.*&d=([^&]*)/.exec(documentURI);
          if (!matchArray) {
            reject("no network error message found in URI")
          return;
          }

          var errorMsg = matchArray[1];
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
}


let testSetAutomatic = Task.async(function*() {
  setup();
  let tab = gBrowser.addTab(badPin, {skipAnimation: true});
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;

  gBrowser.selectedTab = tab;

  
  let netError = createNetworkErrorMessagePromise(browser);
  yield netError;

  
  let prefEnabled = new Promise(function(resolve, reject){
    mm.addMessageListener("ssler-test:AutoPrefUpdated", function() {
      if (Services.prefs.getBoolPref("security.ssl.errorReporting.automatic")) {
        resolve();
      } else {
        reject();
      }
    });
  });

  mm.sendAsyncMessage("ssler-test:SetAutoPref",{value:true});

  yield prefEnabled;

  
  let prefDisabled = new Promise(function(resolve, reject){
    mm.addMessageListener("ssler-test:AutoPrefUpdated", function () {
      if (!Services.prefs.getBoolPref("security.ssl.errorReporting.automatic")) {
        resolve();
      } else {
        reject();
      }
    });
  });

  mm.sendAsyncMessage("ssler-test:SetAutoPref",{value:false});

  yield prefDisabled;

  gBrowser.removeTab(tab);
  cleanup();
});


let testSendReportManual = Task.async(function*() {
  setup();
  Services.prefs.setBoolPref("security.ssl.errorReporting.enabled", true);
  Services.prefs.setCharPref("security.ssl.errorReporting.url", "https://example.com/browser/browser/base/content/test/general/pinning_reports.sjs?succeed");

  let tab = gBrowser.addTab(badPin, {skipAnimation: true});
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;

  gBrowser.selectedTab = tab;

  
  let netError = createNetworkErrorMessagePromise(browser);
  yield netError;
  netError.then(function(val){
    is(val.startsWith("An error occurred during a connection to include-subdomains.pinning.example.com"), true ,"ensure the correct error message came from about:neterror");
  });

  
  let btn = browser.contentDocument.getElementById("reportCertificateError");

  
  let reportWillStart = new Promise(function(resolve, reject){
    mm.addMessageListener("Browser:SendSSLErrorReport", function() {
      resolve();
    });
  });

  let deferredReportActivity = Promise.defer()
  let deferredReportSucceeds = Promise.defer();

  
  mm.addMessageListener("ssler-test:SSLErrorReportStatus", function(message) {
    switch(message.data.reportStatus) {
      case "activity":
        deferredReportActivity.resolve(message.data.reportStatus);
        break;
      case "complete":
        deferredReportSucceeds.resolve(message.data.reportStatus);
        break;
      case "error":
        deferredReportSucceeds.reject();
        deferredReportActivity.reject();
        break;
    }
  });

  
  mm.sendAsyncMessage("ssler-test:SendBtnClick",{});

  yield reportWillStart;

  yield deferredReportActivity.promise;
  yield deferredReportSucceeds.promise;

  gBrowser.removeTab(tab);
  cleanup();
});


let testSendReportAuto = Task.async(function*() {
  setup();
  Services.prefs.setBoolPref("security.ssl.errorReporting.enabled", true);
  Services.prefs.setBoolPref("security.ssl.errorReporting.automatic", true);
  Services.prefs.setCharPref("security.ssl.errorReporting.url", "https://example.com/browser/browser/base/content/test/general/pinning_reports.sjs?succeed");

  let tab = gBrowser.addTab(badPin, {skipAnimation: true});
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;

  gBrowser.selectedTab = tab;

  let reportWillStart = Promise.defer();
  mm.addMessageListener("Browser:SendSSLErrorReport", function() {
    reportWillStart.resolve();
  });

  let deferredReportActivity = Promise.defer();
  let deferredReportSucceeds = Promise.defer();

  mm.addMessageListener("ssler-test:SSLErrorReportStatus", function(message) {
    switch(message.data.reportStatus) {
      case "activity":
        deferredReportActivity.resolve(message.data.reportStatus);
        break;
      case "complete":
        deferredReportSucceeds.resolve(message.data.reportStatus);
        break;
      case "error":
        deferredReportSucceeds.reject();
        deferredReportActivity.reject();
        break;
    }
  });

  
  let netError = createNetworkErrorMessagePromise(browser);
  yield netError;

  
  yield reportWillStart;
  yield deferredReportActivity.promise;
  yield deferredReportSucceeds.promise;

  gBrowser.removeTab(tab);
  cleanup();
});


let testSendReportError = Task.async(function*() {
  setup();
  Services.prefs.setBoolPref("security.ssl.errorReporting.enabled", true);
  Services.prefs.setBoolPref("security.ssl.errorReporting.automatic", true);
  Services.prefs.setCharPref("security.ssl.errorReporting.url", "https://example.com/browser/browser/base/content/test/general/pinning_reports.sjs?error");

  let tab = gBrowser.addTab(badPin, {skipAnimation: true});
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;

  gBrowser.selectedTab = tab;

  
  let reportWillStart = new Promise(function(resolve, reject){
    mm.addMessageListener("Browser:SendSSLErrorReport", function() {
      resolve();
    });
  });

  let netError = createNetworkErrorMessagePromise(browser);
  yield netError;
  yield reportWillStart;

  
  let reportErrors = new Promise(function(resolve, reject) {
    mm.addMessageListener("ssler-test:SSLErrorReportStatus", function(message) {
      switch(message.data.reportStatus) {
      case "complete":
        reject(message.data.reportStatus);
        break;
      case "error":
        resolve(message.data.reportStatus);
        break;
      }
    });
  });

  yield reportErrors;

  gBrowser.removeTab(tab);
  cleanup();
});

let testSendReportDisabled = Task.async(function*() {
  setup();
  Services.prefs.setBoolPref("security.ssl.errorReporting.enabled", false);
  Services.prefs.setCharPref("security.ssl.errorReporting.url", "https://offdomain.com");

  let tab = gBrowser.addTab(badPin, {skipAnimation: true});
  let browser = tab.linkedBrowser;
  let mm = browser.messageManager;

  gBrowser.selectedTab = tab;

  
  let netError = createNetworkErrorMessagePromise(browser);
  yield netError;

  let reportErrors = new Promise(function(resolve, reject) {
    mm.addMessageListener("ssler-test:SSLErrorReportStatus", function(message) {
      switch(message.data.reportStatus) {
      case "complete":
        reject(message.data.reportStatus);
        break;
      case "error":
        resolve(message.data.reportStatus);
        break;
      }
    });
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
