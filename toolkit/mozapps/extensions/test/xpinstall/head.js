const RELATIVE_DIR = "toolkit/mozapps/extensions/test/xpinstall/";

const TESTROOT = "http://example.com/browser/" + RELATIVE_DIR;
const TESTROOT2 = "http://example.org/browser/" + RELATIVE_DIR;
const CHROMEROOT = "chrome://mochikit/content/browser/" + RELATIVE_DIR;
const XPINSTALL_URL = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";
const PROMPT_URL = "chrome://global/content/commonDialog.xul";
const ADDONS_URL = "chrome://mozapps/content/extensions/extensions.xul";

Components.utils.import("resource://gre/modules/AddonManager.jsm");







var Harness = {
  
  
  installBlockedCallback: null,
  
  
  
  authenticationCallback: null,
  
  
  installConfirmCallback: null,
  
  downloadStartedCallback: null,
  
  downloadProgressCallback: null,
  
  downloadFailedCallback: null,
  
  downloadCancelledCallback: null,
  
  downloadEndedCallback: null,
  
  
  installStartedCallback: null,
  
  installFailedCallback: null,
  
  
  installEndedCallback: null,
  
  
  installsCompletedCallback: null,

  pendingCount: null,
  installCount: null,

  
  setup: function() {
    waitForExplicitFinish();

    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    os.addObserver(this, "addon-install-blocked", false);

    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    wm.addListener(this);

    AddonManager.addInstallListener(this);
    this.installCount = 0;
    this.pendingCount = 0;
  },

  finish: function() {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    os.removeObserver(this, "addon-install-blocked");

    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    wm.removeListener(this);
    var win = wm.getMostRecentWindow("Extension:Manager");
    if (win)
      win.close();

    AddonManager.removeInstallListener(this);

    AddonManager.getAllInstalls(function(installs) {
      is(installs.length, 0, "Should be no active installs at the end of the test");
      finish();
    });
  },

  endTest: function() {
    
    
    let callback = this.installsCompletedCallback;
    let count = this.installCount;
    executeSoon(function() {
      callback(count);
    });

    this.installBlockedCallback = null;
    this.authenticationCallback = null;
    this.installConfirmCallback = null;
    this.downloadStartedCallback = null;
    this.downloadProgressCallback = null;
    this.downloadCancelledCallback = null;
    this.downloadFailedCallback = null;
    this.downloadEndedCallback = null;
    this.installStartedCallback = null;
    this.installFailedCallback = null;
    this.installEndedCallback = null;
    this.installsCompletedCallback = null;
  },

  
  windowLoad: function(window) {
    
    var self = this;
    executeSoon(function() { self.windowReady(window); } );
  },

  windowReady: function(window) {
    if (window.document.location.href == XPINSTALL_URL) {
      if (this.installBlockedCallback)
        ok(false, "Should have been blocked by the whitelist");
      this.pendingCount = window.document.getElementById("itemList").childNodes.length;

      
      
      if (this.installConfirmCallback && !this.installConfirmCallback(window)) {
        window.document.documentElement.cancelDialog();
        this.endTest();
      }
      else {
        
        var button = window.document.documentElement.getButton("accept");
        button.disabled = false;
        window.document.documentElement.acceptDialog();
      }
    }
    else if (window.document.location.href == PROMPT_URL) {
      switch (window.gCommonDialogParam.GetInt(3)) {
        case 0: window.document.documentElement.acceptDialog();
                break;
        case 2: if (window.gCommonDialogParam.GetInt(4) != 1) {
                  
                  
                  if (this.authenticationCallback) {
                    var auth = this.authenticationCallback();
                    if (auth && auth.length == 2) {
                      window.document.getElementById("loginTextbox").value = auth[0];
                      window.document.getElementById("password1Textbox").value = auth[1];
                      window.document.documentElement.acceptDialog();
                    }
                    else {
                      window.document.documentElement.cancelDialog();
                    }
                  }
                  else {
                    window.document.documentElement.cancelDialog();
                  }
                }
                break;
      }
    }
  },

  

  installBlocked: function(installInfo) {
    ok(!!this.installBlockedCallback, "Shouldn't have been blocked by the whitelist");
    if (this.installBlockedCallback && this.installBlockedCallback(installInfo)) {
      this.installBlockedCallback = null;
      installInfo.install();
    }
    else {
      installInfo.installs.forEach(function(install) {
        install.cancel();
      });
      this.endTest();
    }
  },

  

  onWindowTitleChange: function(window, title) {
  },

  onOpenWindow: function(window) {
    var domwindow = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                          .getInterface(Components.interfaces.nsIDOMWindowInternal);
    var self = this;
    domwindow.addEventListener("load", function() {
      self.windowLoad(domwindow);
    }, false);
  },

  onCloseWindow: function(window) {
  },

  

  onDownloadStarted: function(install) {
    this.pendingCount++;
    if (this.downloadStartedCallback)
      this.downloadStartedCallback(install);
  },

  onDownloadProgress: function(install) {
    if (this.downloadProgressCallback)
      this.downloadProgressCallback(install);
  },

  onDownloadEnded: function(install) {
    if (this.downloadEndedCallback)
      this.downloadEndedCallback(install);
  },

  onDownloadCancelled: function(install) {
    if (this.downloadCancelledCallback)
      this.downloadCancelledCallback(install);
    this.checkTestEnded();
  },

  onDownloadFailed: function(install, status) {
    if (this.downloadFailedCallback)
      this.downloadFailedCallback(install, status);
    this.checkTestEnded();
  },

  onInstallStarted: function(install) {
    if (this.installStartedCallback)
      this.installStartedCallback(install);
  },

  onInstallEnded: function(install, addon) {
    if (this.installEndedCallback)
      this.installEndedCallback(install, addon);
    this.installCount++;
    this.checkTestEnded();
  },

  onInstallFailed: function(install, status) {
    if (this.installFailedCallback)
      this.installFailedCallback(install, status);
    this.checkTestEnded();
  },

  checkTestEnded: function() {
    dump("checkTestPending " + this.pendingCount + "\n");
    if (--this.pendingCount == 0)
      this.endTest();
  },

  

  observe: function(subject, topic, data) {
    var installInfo = subject.QueryInterface(Components.interfaces.amIWebInstallInfo);
    this.installBlocked(installInfo);
  },

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIObserver) ||
        iid.equals(Components.interfaces.nsIWindowMediatorListener) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}
