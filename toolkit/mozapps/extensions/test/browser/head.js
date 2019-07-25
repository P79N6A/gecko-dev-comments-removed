



Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");

const RELATIVE_DIR = "browser/toolkit/mozapps/extensions/test/browser/";

const TESTROOT = "http://example.com/" + RELATIVE_DIR;
const TESTROOT2 = "http://example.org/" + RELATIVE_DIR;
const CHROMEROOT = "chrome://mochikit/content/" + RELATIVE_DIR;

const MANAGER_URI = "about:addons";
const INSTALL_URI = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";
const PREF_LOGGING_ENABLED = "extensions.logging.enabled";

var gPendingTests = [];
var gTestsRun = 0;


Services.prefs.setBoolPref(PREF_LOGGING_ENABLED, true);
registerCleanupFunction(function() {
  Services.prefs.clearUserPref(PREF_LOGGING_ENABLED);
});

function add_test(test) {
  gPendingTests.push(test);
}

function run_next_test() {
  if (gPendingTests.length == 0) {
    end_test();
    return;
  }

  gTestsRun++;
  info("Running test " + gTestsRun);

  gPendingTests.shift()();
}

function get_addon_file_url(aFilename) {
  var cr = Cc["@mozilla.org/chrome/chrome-registry;1"].
           getService(Ci.nsIChromeRegistry);
  var fileurl = cr.convertChromeURL(makeURI(CHROMEROOT + "addons/" + aFilename));
  return fileurl.QueryInterface(Ci.nsIFileURL);
}

function wait_for_view_load(aManagerWindow, aCallback) {
  if (!aManagerWindow.gViewController.isLoading) {
    aCallback(aManagerWindow);
    return;
  }

  aManagerWindow.document.addEventListener("ViewChanged", function() {
    aManagerWindow.document.removeEventListener("ViewChanged", arguments.callee, false);
    aCallback(aManagerWindow);
  }, false);
}

function wait_for_manager_load(aManagerWindow, aCallback) {
  if (!aManagerWindow.gIsInitializing) {
    aCallback(aManagerWindow);
    return;
  }

  aManagerWindow.document.addEventListener("Initialized", function() {
    aManagerWindow.document.removeEventListener("Initialized", arguments.callee, false);
    aCallback(aManagerWindow);
  }, false);
}

function open_manager(aView, aCallback) {
  function setup_manager(aManagerWindow) {
    if (aView)
      aManagerWindow.loadView(aView);

    ok(aManagerWindow != null, "Should have an add-ons manager window");
    is(aManagerWindow.location, MANAGER_URI, "Should be displaying the correct UI");

    wait_for_manager_load(aManagerWindow, function() {
      wait_for_view_load(aManagerWindow, aCallback);
    });
  }

  if ("switchToTabHavingURI" in window) {
    switchToTabHavingURI(MANAGER_URI, true, function(aBrowser) {
      setup_manager(aBrowser.contentWindow.wrappedJSObject);
    });
    return;
  }

  openDialog(MANAGER_URI).addEventListener("load", function() {
    this.removeEventListener("load", arguments.callee, false);
    setup_manager(this);
  }, false);
}

function close_manager(aManagerWindow, aCallback) {
  ok(aManagerWindow != null, "Should have an add-ons manager window to close");
  is(aManagerWindow.location, MANAGER_URI, "Should be closing window with correct URI");

  aManagerWindow.addEventListener("unload", function() {
    this.removeEventListener("unload", arguments.callee, false);
    aCallback();
  }, false);

  aManagerWindow.close();
}

function restart_manager(aManagerWindow, aView, aCallback) {
  close_manager(aManagerWindow, function() { open_manager(aView, aCallback); });
}

function CategoryUtilities(aManagerWindow) {
  this.window = aManagerWindow;

  var self = this;
  this.window.addEventListener("unload", function() {
    self.removeEventListener("unload", arguments.callee, false);
    self.window = null;
  }, false);
}

CategoryUtilities.prototype = {
  window: null,

  get selectedCategory() {
    isnot(this.window, null, "Should not get selected category when manager window is not loaded");
    var selectedItem = this.window.document.getElementById("categories").selectedItem;
    isnot(selectedItem, null, "A category should be selected");
    var view = this.window.gViewController.parseViewId(selectedItem.value);
    return (view.type == "list") ? view.param : view.type;
  },

  get: function(aCategoryType) {
    isnot(this.window, null, "Should not get category when manager window is not loaded");
    var categories = this.window.document.getElementById("categories");

    var viewId = "addons://list/" + aCategoryType;
    var items = categories.getElementsByAttribute("value", viewId);
    if (items.length)
      return items[0];

    viewId = "addons://" + aCategoryType + "/";
    items = categories.getElementsByAttribute("value", viewId);
    if (items.length)
      return items[0];

    ok(false, "Should have found a category with type " + aCategoryType);
    return null;
  },

  getViewId: function(aCategoryType) {
    isnot(this.window, null, "Should not get view id when manager window is not loaded");
    return this.get(aCategoryType).value;
  },

  isVisible: function(aCategory) {
    isnot(this.window, null, "Should not check visible state when manager window is not loaded");
    if (aCategory.hasAttribute("disabled") &&
        aCategory.getAttribute("disabled") == "true")
      return false;

    var style = this.window.document.defaultView.getComputedStyle(aCategory, "");
    return style.display != "none" && style.visibility == "visible";
  },

  isTypeVisible: function(aCategoryType) {
    return this.isVisible(this.get(aCategoryType));
  },

  open: function(aCategory, aCallback) {
    isnot(this.window, null, "Should not open category when manager window is not loaded");
    ok(this.isVisible(aCategory), "Category should be visible if attempting to open it");

    EventUtils.synthesizeMouse(aCategory, 2, 2, { }, this.window);

    if (aCallback)
      wait_for_view_load(this.window, aCallback);
  },

  openType: function(aCategoryType, aCallback) {
    this.open(this.get(aCategoryType), aCallback);
  }
}

function CertOverrideListener(host, bits) {
  this.host = host;
  this.bits = bits;
}

CertOverrideListener.prototype = {
  host: null,
  bits: null,

  getInterface: function (aIID) {
    return this.QueryInterface(aIID);
  },

  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIBadCertListener2) ||
        aIID.equals(Ci.nsIInterfaceRequestor) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  notifyCertProblem: function (socketInfo, sslStatus, targetHost) {
    cert = sslStatus.QueryInterface(Components.interfaces.nsISSLStatus)
                    .serverCert;
    var cos = Cc["@mozilla.org/security/certoverride;1"].
              getService(Ci.nsICertOverrideService);
    cos.rememberValidityOverride(this.host, -1, cert, this.bits, false);
    return true;
  }
}


function addCertOverride(host, bits) {
  var req = new XMLHttpRequest();
  try {
    req.open("GET", "https://" + host + "/", false);
    req.channel.notificationCallbacks = new CertOverrideListener(host, bits);
    req.send(null);
  }
  catch (e) {
    
  }
}



function MockProvider(aUseAsyncCallbacks) {
  this.addons = [];
  this.installs = [];
  this.callbackTimers = [];
  this.useAsyncCallbacks = (aUseAsyncCallbacks === undefined) ? true : aUseAsyncCallbacks;

  var self = this;
  registerCleanupFunction(function() {
    if (self.started)
      self.unregister();
  });

  this.register();
}

MockProvider.prototype = {
  addons: null,
  installs: null,
  started: null,
  apiDelay: 100,
  callbackTimers: null,
  useAsyncCallbacks: null,

  

  


  register: function MP_register() {
    AddonManagerPrivate.registerProvider(this);
  },

  


  unregister: function MP_unregister() {
    AddonManagerPrivate.unregisterProvider(this);
  },

  






  addAddon: function MP_addAddon(aAddon) {
    this.addons.push(aAddon);
    aAddon._provider = this;

    if (!this.started)
      return;

    let requiresRestart = (aAddon.operationsRequiringRestart &
                           AddonManager.OP_NEEDS_RESTART_INSTALL) != 0;
    AddonManagerPrivate.callInstallListeners("onExternalInstall", null, aAddon,
                                             null, requiresRestart)
  },

  






  removeAddon: function MP_removeAddon(aAddon) {
    var pos = this.addons.indexOf(aAddon);
    if (pos == -1) {
      ok(false, "Tried to remove an add-on that wasn't registered with the mock provider");
      return;
    }

    this.addons.splice(pos, 1);

    if (!this.started)
      return;

    AddonManagerPrivate.callAddonListeners("onUninstalled", aAddon);
  },

  






  addInstall: function MP_addInstall(aInstall) {
    this.installs.push(aInstall);

    if (!this.started)
      return;

    aInstall.callListeners("onNewInstall");
  },

  







  createAddons: function MP_createAddons(aAddonProperties) {
    var newAddons = [];
    aAddonProperties.forEach(function(aAddonProp) {
      var addon = new MockAddon(aAddonProp.id);
      for (var prop in aAddonProp) {
        if (prop == "id")
          continue;
        addon[prop] = aAddonProp[prop];
      }
      this.addAddon(addon);
      newAddons.push(addon);
    }, this);

    return newAddons;
  },

  







  createInstalls: function MP_createInstalls(aInstallProperties) {
    var newInstalls = [];
    aInstallProperties.forEach(function(aInstallProp) {
      var install = new MockInstall();
      for (var prop in aInstallProp) {
        if (prop == "sourceURI") {
          install[prop] = NetUtil.newURI(aInstallProp[prop]);
          continue;
        }

        install[prop] = aInstallProp[prop];
      }
      this.addInstall(install);
      newInstalls.push(install);
    }, this);

    return newInstalls;
  },

  

  


  startup: function MP_startup() {
    this.started = true;
  },

  


  shutdown: function MP_shutdown() {
    this.callbackTimers.forEach(function(aTimer) {
      aTimer.cancel();
    });
    this.callbackTimers = [];

    this.started = false;
  },

  







  getAddonByID: function MP_getAddon(aId, aCallback) {
    for (let i = 0; i < this.addons.length; i++) {
      if (this.addons[i].id == aId) {
        this._delayCallback(aCallback, this.addons[i]);
        return;
      }
    }

    aCallback(null);
  },

  







  getAddonsByTypes: function MP_getAddonsByTypes(aTypes, aCallback) {
    var addons = this.addons.filter(function(aAddon) {
      if (aTypes && aTypes.length > 0 && aTypes.indexOf(aAddon.type) == -1)
        return false;
      return true;
    });
    this._delayCallback(aCallback, addons);
  },

  







  getAddonsWithOperationsByTypes: function MP_getAddonsWithOperationsByTypes(aTypes, aCallback) {
    var addons = this.addons.filter(function(aAddon) {
      if (aTypes && aTypes.length > 0 && aTypes.indexOf(aAddon.type) == -1)
        return false;
      return aAddon.pendingOperations != 0;
    });
    this._delayCallback(aCallback, addons);
  },

  







  getInstallsByTypes: function MP_getInstallsByTypes(aTypes, aCallback) {
    var installs = this.installs.filter(function(aInstall) {
      
      if (aInstall.state == AddonManager.STATE_CANCELLED)
        return false;

      if (aTypes && aTypes.length > 0 && aTypes.indexOf(aInstall.type) == -1)
        return false;

      return true;
    });
    this._delayCallback(aCallback, installs);
  },

  











  addonChanged: function MP_addonChanged(aId, aType, aPendingRestart) {
    
  },

  


  updateAddonAppDisabledStates: function MP_updateAddonAppDisabledStates() {
    
  },

  

















  getInstallForURL: function MP_getInstallForURL(aUrl, aHash, aName, aIconURL,
                                                  aVersion, aLoadGroup, aCallback) {
    
  },

  







  getInstallForFile: function MP_getInstallForFile(aFile, aCallback) {
    
  },

  




  isInstallEnabled: function MP_isInstallEnabled() {
    return false;
  },

  







  supportsMimetype: function MP_supportsMimetype(aMimetype) {
    return false;
  },

  






  isInstallAllowed: function MP_isInstallAllowed(aUri) {
    return false;
  },


  

  







  _delayCallback: function MP_delayCallback(aCallback) {
    var params = Array.splice(arguments, 1);

    if (!this.useAsyncCallbacks) {
      aCallback.apply(null, params);
      return;
    }

    var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    
    var pos = this.callbackTimers.length;
    this.callbackTimers.push(timer);
    var self = this;
    timer.initWithCallback(function() {
      self.callbackTimers.splice(pos, 1);
      aCallback.apply(null, params);
    }, this.apiDelay, timer.TYPE_ONE_SHOT);
  }
};



function MockAddon(aId, aName, aType, aOperationsRequiringRestart) {
  
  this.id = aId || "";
  this.name = aName || "";
  this.type = aType || "extension";
  this.version = "";
  this.isCompatible = true;
  this.providesUpdatesSecurely = true;
  this.blocklistState = 0;
  this.appDisabled = false;
  this._userDisabled = false;
  this.scope = AddonManager.SCOPE_PROFILE;
  this.isActive = true;
  this.creator = "";
  this.pendingOperations = 0;
  this.permissions = AddonManager.PERM_CAN_UNINSTALL |
                     AddonManager.PERM_CAN_ENABLE |
                     AddonManager.PERM_CAN_DISABLE |
                     AddonManager.PERM_CAN_UPGRADE;
  this.operationsRequiringRestart = aOperationsRequiringRestart ||
    (AddonManager.OP_NEEDS_RESTART_INSTALL |
     AddonManager.OP_NEEDS_RESTART_UNINSTALL |
     AddonManager.OP_NEEDS_RESTART_ENABLE |
     AddonManager.OP_NEEDS_RESTART_DISABLE);
}

MockAddon.prototype = {
  get shouldBeActive() {
    return !this.appDisabled && !this._userDisabled;
  },

  get userDisabled() {
    return this._userDisabled;
  },

  set userDisabled(val) {
    if (val == this._userDisabled)
      return val;

    var currentActive = this.shouldBeActive;
    this._userDisabled = val;
    var newActive = this.shouldBeActive;
    this._updateActiveState(currentActive, newActive);

    return val;
  },

  isCompatibleWith: function(aAppVersion, aPlatformVersion) {
    return true;
  },

  findUpdates: function(aListener, aReason, aAppVersion, aPlatformVersion) {
    
  },

  uninstall: function() {
    if (this.pendingOperations & AddonManager.PENDING_UNINSTALL)
      throw new Error("Add-on is already pending uninstall");

    var needsRestart = !!(this.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL);
    this.pendingOperations |= AddonManager.PENDING_UNINSTALL;
    AddonManagerPrivate.callAddonListeners("onUninstalling", this, needsRestart);
    if (!needsRestart) {
      this.pendingOperations -= AddonManager.PENDING_UNINSTALL;
      this._provider.removeAddon(this);
    }
  },

  cancelUninstall: function() {
    if (!(this.pendingOperations & AddonManager.PENDING_UNINSTALL))
      throw new Error("Add-on is not pending uninstall");

    this.pendingOperations -= AddonManager.PENDING_UNINSTALL;
    AddonManagerPrivate.callAddonListeners("onOperationCancelled", this);
  },

  _updateActiveState: function(currentActive, newActive) {
    if (currentActive == newActive)
      return;

    if (newActive == this.isActive) {
      AddonManagerPrivate.callAddonListeners("onOperationCancelled", this);
    }
    else if (newActive) {
      var needsRestart = !!(this.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_ENABLE);
      this.pendingOperations |= AddonManager.PENDING_ENABLE;
      AddonManagerPrivate.callAddonListeners("onEnabling", this, needsRestart);
      if (!needsRestart) {
        this.isActive = newActive;
        this.pendingOperations -= AddonManager.PENDING_ENABLE;
        AddonManagerPrivate.callAddonListeners("onEnabled", this);
      }
    }
    else {
      var needsRestart = !!(this.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_DISABLE);
      this.pendingOperations |= AddonManager.PENDING_DISABLE;
      AddonManagerPrivate.callAddonListeners("onDisabling", this, needsRestart);
      if (!needsRestart) {
        this.isActive = newActive;
        this.pendingOperations -= AddonManager.PENDING_DISABLE;
        AddonManagerPrivate.callAddonListeners("onDisabled", this);
      }
    }
  }
};



function MockInstall(aName, aType, aAddonToInstall) {
  this.name = aName || "";
  this.type = aType || "extension";
  this.version = "1.0";
  this.iconURL = "";
  this.infoURL = "";
  this.state = AddonManager.STATE_AVAILABLE;
  this.error = 0;
  this.sourceURI = null;
  this.file = null;
  this.progress = 0;
  this.maxProgress = -1;
  this.certificate = null;
  this.certName = "";
  this.existingAddon = null;
  this.addon = null;
  this._addonToInstall = aAddonToInstall;
  this.listeners = [];

  
  
  this.testListeners = [];
}

MockInstall.prototype = {
  install: function() {
    switch (this.state) {
      case AddonManager.STATE_AVAILABLE:
      case AddonManager.STATE_DOWNLOADED:
        

        this.state = AddonManager.STATE_INSTALLING;
        if (!this.callListeners("onInstallStarted")) {
          
          this.state = AddonManager.STATE_CANCELLED;
          this.callListeners("onInstallCancelled");
          return;
        }

        
        if (this._addonToInstall)
          this.addon = this._addonToInstall;
        else {
          this.addon = new MockAddon("", this.name, this.type);
          this.addon.pendingOperations = AddonManager.PENDING_INSTALL;
        }

        this.state = AddonManager.STATE_INSTALLED;
        this.callListeners("onInstallEnded");
        break;
      case AddonManager.STATE_DOWNLOADING:
      case AddonManager.STATE_CHECKING:
      case AddonManger.STATE_INSTALLING:
        
        return;
      default:
        ok(false, "Cannot start installing when state = " + this.state);
    }
  },

  cancel: function() {
    switch (this.state) {
      case AddonManager.STATE_AVAILABLE:
        this.state = AddonManager.STATE_CANCELLED;
        break;
      case AddonManager.STATE_INSTALLED:
        this.state = AddonManager.STATE_CANCELLED;
        this.callListeners("onInstallCancelled");
        break;
      default:
        
        ok(false, "Cannot cancel when state = " + this.state);
    }
  },


  addListener: function(aListener) {
    if (!this.listeners.some(function(i) i == aListener))
      this.listeners.push(aListener);
  },

  removeListener: function(aListener) {
    this.listeners = this.listeners.filter(function(i) i != aListener);
  },

  addTestListener: function(aListener) {
    if (!this.testListeners.some(function(i) i == aListener))
      this.testListeners.push(aListener);
  },

  removeTestListener: function(aListener) {
    this.testListeners = this.testListeners.filter(function(i) i != aListener);
  },

  callListeners: function(aMethod) {
    var result = AddonManagerPrivate.callInstallListeners(aMethod, this.listeners,
                                                          this, this.addon);

    
    
    this.testListeners.forEach(function(aListener) {
      if (aMethod in aListener)
        if (aListener[aMethod].call(aListener, this, this.addon) === false)
          result = false;
    });

    return result;
  }
};

