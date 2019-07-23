const TESTROOT = "http://example.com/browser/xpinstall/tests/";
const TESTROOT2 = "http://example.org/browser/xpinstall/tests/";
const CHROMEROOT = "chrome://mochikit/content/browser/xpinstall/tests/"
const XPINSTALL_URL = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";
const PROMPT_URL = "chrome://global/content/commonDialog.xul";
const ADDONS_URL = "chrome://mozapps/content/extensions/extensions.xul";







var Harness = {
  
  
  installBlockedCallback: null,
  
  
  
  authenticationCallback: null,
  
  
  installConfirmCallback: null,
  
  downloadStartedCallback: null,
  
  downloadProgressCallback: null,
  
  downloadEndedCallback: null,
  
  
  installStartedCallback: null,
  
  
  installEndedCallback: null,
  
  
  installsCompletedCallback: null,

  listenerIndex: null,

  
  setup: function() {
    waitForExplicitFinish();

    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    os.addObserver(this, "xpinstall-install-blocked", false);

    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    wm.addListener(this);

    var em = Components.classes["@mozilla.org/extensions/manager;1"]
                       .getService(Components.interfaces.nsIExtensionManager);
    this.listenerIndex = em.addInstallListener(this);
  },

  finish: function() {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    os.removeObserver(this, "xpinstall-install-blocked");

    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    wm.removeListener(this);
    var win = wm.getMostRecentWindow("Extension:Manager");
    if (win)
      win.close();

    var em = Components.classes["@mozilla.org/extensions/manager;1"]
                       .getService(Components.interfaces.nsIExtensionManager);
    em.removeInstallListenerAt(this.listenerIndex);
    finish();
  },

  endTest: function() {
    
    
    executeSoon(this.installsCompletedCallback);

    this.installBlockedCallback = null;
    this.authenticationCallback = null;
    this.installConfirmCallback = null;
    this.downloadStartedCallback = null;
    this.downloadProgressCallback = null;
    this.downloadEndedCallback = null;
    this.installStartedCallback = null;
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
        case 0: if (window.opener.document.location.href == ADDONS_URL) {
                  
                  
                  
                  window.document.documentElement.acceptDialog();
                }
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
      var mgr = Components.classes["@mozilla.org/xpinstall/install-manager;1"]
                          .createInstance(Components.interfaces.nsIXPInstallManager);
      mgr.initManagerWithInstallInfo(installInfo);
    }
    else {
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

  

  onDownloadStarted: function(addon) {
    if (this.downloadStartedCallback)
      this.downloadStartedCallback(addon);
  },

  onDownloadProgress: function(addon, value, maxValue) {
    if (this.downloadProgressCallback)
      this.downloadProgressCallback(addon, value, maxValue);
  },

  onDownloadEnded: function(addon) {
    if (this.downloadEndedCallback)
      this.downloadEndedCallback(addon);
  },

  onInstallStarted: function(addon) {
    if (this.installStartedCallback)
      this.installStartedCallback(addon);
  },

  onCompatibilityCheckStarted: function(addon) {
  },

  onCompatibilityCheckEnded: function(addon, status) {
  },

  onInstallEnded: function(addon, status) {
    if (this.installEndedCallback)
      this.installEndedCallback(addon, status);
  },

  onInstallsCompleted: function() {
    this.endTest();
  },

  

  observe: function(subject, topic, data) {
    var installInfo = subject.QueryInterface(Components.interfaces.nsIXPIInstallInfo);
    this.installBlocked(installInfo);
  },

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIObserver) ||
        iid.equals(Components.interfaces.nsIAddonInstallListener) ||
        iid.equals(Components.interfaces.nsIWindowMediatorListener) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}
