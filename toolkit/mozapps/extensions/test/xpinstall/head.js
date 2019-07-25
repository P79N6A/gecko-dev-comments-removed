const RELATIVE_DIR = "toolkit/mozapps/extensions/test/xpinstall/";

const TESTROOT = "http://example.com/browser/" + RELATIVE_DIR;
const TESTROOT2 = "http://example.org/browser/" + RELATIVE_DIR;
const XPINSTALL_URL = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";
const PROMPT_URL = "chrome://global/content/commonDialog.xul";
const ADDONS_URL = "chrome://mozapps/content/extensions/extensions.xul";
const PREF_LOGGING_ENABLED = "extensions.logging.enabled";
const PREF_INSTALL_REQUIREBUILTINCERTS = "extensions.install.requireBuiltInCerts";
const CHROME_NAME = "mochikit";

function getChromeRoot(path) {
  if (path === undefined) {
    return "chrome://" + CHROME_NAME + "/content/browser/" + RELATIVE_DIR
  }
  return getRootDirectory(path);
}

function extractChromeRoot(path) {
  var chromeRootPath = getChromeRoot(path);
  var jar = getJar(chromeRootPath);
  if (jar) {
    var tmpdir = extractJarToTmp(jar);
    return "file://" + tmpdir.path + "/";
  }
  return chromeRootPath;
}







var Harness = {
  
  
  installDisabledCallback: null,
  
  
  
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
  runningInstalls: null,

  waitingForFinish: false,

  
  setup: function() {
    if (!this.waitingForFinish) {
      waitForExplicitFinish();
      this.waitingForFinish = true;

      Services.prefs.setBoolPref(PREF_LOGGING_ENABLED, true);
      Services.obs.addObserver(this, "addon-install-started", false);
      Services.obs.addObserver(this, "addon-install-disabled", false);
      Services.obs.addObserver(this, "addon-install-blocked", false);
      Services.obs.addObserver(this, "addon-install-failed", false);
      Services.obs.addObserver(this, "addon-install-complete", false);

      AddonManager.addInstallListener(this);

      Services.wm.addListener(this);

      var self = this;
      registerCleanupFunction(function() {
        Services.prefs.clearUserPref(PREF_LOGGING_ENABLED);
        Services.obs.removeObserver(self, "addon-install-started");
        Services.obs.removeObserver(self, "addon-install-disabled");
        Services.obs.removeObserver(self, "addon-install-blocked");
        Services.obs.removeObserver(self, "addon-install-failed");
        Services.obs.removeObserver(self, "addon-install-complete");

        AddonManager.removeInstallListener(self);

        Services.wm.removeListener(self);

        AddonManager.getAllInstalls(function(aInstalls) {
          is(aInstalls.length, 0, "Should be no active installs at the end of the test");
          aInstalls.forEach(function(aInstall) {
            info("Install for " + aInstall.sourceURI + " is in state " + aInstall.state);
            aInstall.cancel();
          });
        });
      });
    }

    this.installCount = 0;
    this.pendingCount = 0;
    this.runningInstalls = [];
  },

  finish: function() {
    finish();
  },

  endTest: function() {
    
    
    var self = this;
    executeSoon(function() {
      let callback = self.installsCompletedCallback;
      let count = self.installCount;

      is(self.runningInstalls.length, 0, "Should be no running installs left");
      self.runningInstalls.forEach(function(aInstall) {
        info("Install for " + aInstall.sourceURI + " is in state " + aInstall.state);
      });

      self.installBlockedCallback = null;
      self.authenticationCallback = null;
      self.installConfirmCallback = null;
      self.downloadStartedCallback = null;
      self.downloadProgressCallback = null;
      self.downloadCancelledCallback = null;
      self.downloadFailedCallback = null;
      self.downloadEndedCallback = null;
      self.installStartedCallback = null;
      self.installFailedCallback = null;
      self.installEndedCallback = null;
      self.installsCompletedCallback = null;
      self.runningInstalls = null;

      callback(count);
    });
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
        var promptType = window.args.promptType;
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

  

  installDisabled: function(installInfo) {
    ok(!!this.installDisabledCallback, "Installation shouldn't have been disabled");
    if (this.installDisabledCallback)
      this.installDisabledCallback(installInfo);
    installInfo.installs.forEach(function(install) {
      install.cancel();
    });
    this.endTest();
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
    waitForFocus(function() {
      self.windowReady(domwindow);
    }, domwindow);
  },

  onCloseWindow: function(window) {
  },

  

  onNewInstall: function(install) {
    this.runningInstalls.push(install);
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
    isnot(this.runningInstalls.indexOf(install), -1,
          "Should only see cancelations for started installs");
    this.runningInstalls.splice(this.runningInstalls.indexOf(install), 1);

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
    if (--this.pendingCount == 0)
      this.endTest();
  },

  

  observe: function(subject, topic, data) {
    var installInfo = subject.QueryInterface(Components.interfaces.amIWebInstallInfo);
    switch (topic) {
    case "addon-install-started":
      is(this.runningInstalls.length, installInfo.installs.length,
         "Should have seen the expected number of installs started");
      break;
    case "addon-install-disabled":
      this.installDisabled(installInfo);
      break;
    case "addon-install-blocked":
      this.installBlocked(installInfo);
      break;
    case "addon-install-failed":
      installInfo.installs.forEach(function(aInstall) {
        isnot(this.runningInstalls.indexOf(aInstall), -1,
              "Should only see failures for started installs");

        ok(aInstall.error != 0 || aInstall.addon.appDisabled,
           "Failed installs should have an error or be appDisabled");

        this.runningInstalls.splice(this.runningInstalls.indexOf(aInstall), 1);
      }, this);
      break;
    case "addon-install-complete":
      installInfo.installs.forEach(function(aInstall) {
        isnot(this.runningInstalls.indexOf(aInstall), -1,
              "Should only see completed events for started installs");

        is(aInstall.error, 0, "Completed installs should have no error");
        ok(!aInstall.appDisabled, "Completed installs should not be appDisabled");

        
        
        ok(aInstall.state == AddonManager.STATE_INSTALLED ||
           aInstall.state == AddonManager.STATE_CANCELLED,
           "Completed installs should be in the right state");

        this.runningInstalls.splice(this.runningInstalls.indexOf(aInstall), 1);
      }, this);
      break;
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsIWindowMediatorListener,
                                         Ci.nsISupports])
}
