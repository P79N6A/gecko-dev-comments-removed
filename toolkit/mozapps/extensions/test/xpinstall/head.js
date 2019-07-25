const RELATIVE_DIR = "toolkit/mozapps/extensions/test/xpinstall/";

const TESTROOT = "http://example.com/browser/" + RELATIVE_DIR;
const TESTROOT2 = "http://example.org/browser/" + RELATIVE_DIR;
const CHROMEROOT = "chrome://mochikit/content/browser/" + RELATIVE_DIR;
const XPINSTALL_URL = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";
const PROMPT_URL = "chrome://global/content/commonDialog.xul";
const ADDONS_URL = "chrome://mozapps/content/extensions/extensions.xul";
const PREF_LOGGING_ENABLED = "extensions.logging.enabled";

Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");







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
    Services.prefs.setBoolPref(PREF_LOGGING_ENABLED, true);
    Services.obs.addObserver(this, "addon-install-blocked", false);
    Services.wm.addListener(this);

    AddonManager.addInstallListener(this);
    this.installCount = 0;
    this.pendingCount = 0;
  },

  finish: function() {
    Services.prefs.clearUserPref(PREF_LOGGING_ENABLED);
    Services.obs.removeObserver(this, "addon-install-blocked");
    Services.wm.removeListener(this);

    var win = Services.wm.getMostRecentWindow("Extension:Manager");
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
        var promptType = window.gArgs.getProperty("promptType");
        switch (promptType) {
          case "alert":
          case "alertCheck":
          case "confirmCheck":
          case "confirm":
          case "confirmEx":
                window.document.documentElement.acceptDialog();
                break;
          case "promptUserAndPass":
                  
                  
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
                break;
          default:
                ok(false, "prompt type " + promptType + " not handled in test.");
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
      domwindow.removeEventListener("load", arguments.callee, false);
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

  onDownloadFailed: function(install) {
    if (this.downloadFailedCallback)
      this.downloadFailedCallback(install);
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

  onInstallFailed: function(install) {
    if (this.installFailedCallback)
      this.installFailedCallback(install);
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
