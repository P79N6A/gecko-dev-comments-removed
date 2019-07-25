



Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");

const RELATIVE_DIR = "browser/toolkit/mozapps/extensions/test/browser/";

const TESTROOT = "http://example.com/" + RELATIVE_DIR;
const TESTROOT2 = "http://example.org/" + RELATIVE_DIR;

const MANAGER_URI = "about:addons";
const INSTALL_URI = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";
const PREF_LOGGING_ENABLED = "extensions.logging.enabled";
const PREF_SEARCH_MAXRESULTS = "extensions.getAddons.maxResults";
const CHROME_NAME = "mochikit";

function getChromeRoot(path) {
  if (path === undefined) {
    return "chrome://" + CHROME_NAME + "/content/" + RELATIVE_DIR;
  }
  return getRootDirectory(path);
}

var gPendingTests = [];
var gTestsRun = 0;
var gTestStart = null;

var gUseInContentUI = ("switchToTabHavingURI" in window);


Services.prefs.setBoolPref(PREF_LOGGING_ENABLED, true);

Services.prefs.setIntPref(PREF_SEARCH_MAXRESULTS, 0);
registerCleanupFunction(function() {
  Services.prefs.clearUserPref(PREF_LOGGING_ENABLED);
  try {
    Services.prefs.clearUserPref(PREF_SEARCH_MAXRESULTS);
  }
  catch (e) {
  }
});

function add_test(test) {
  gPendingTests.push(test);
}

function run_next_test() {
  if (gTestsRun > 0)
    info("Test " + gTestsRun + " took " + (Date.now() - gTestStart) + "ms");

  if (gPendingTests.length == 0) {
    end_test();
    return;
  }

  gTestsRun++;
  var test = gPendingTests.shift();
  if (test.name)
    info("Running test " + gTestsRun + " (" + test.name + ")");
  else
    info("Running test " + gTestsRun);

  gTestStart = Date.now();
  test();
}

function get_addon_file_url(aFilename) {
  var chromeroot = getChromeRoot(gTestPath);
  try {
    var cr = Cc["@mozilla.org/chrome/chrome-registry;1"].
             getService(Ci.nsIChromeRegistry);
    var fileurl = cr.convertChromeURL(makeURI(chromeroot + "addons/" + aFilename));
    return fileurl.QueryInterface(Ci.nsIFileURL);
  } catch(ex) {
    var jar = getJar(chromeroot + "addons/" + aFilename);
    var tmpDir = extractJarToTmp(jar);
    tmpDir.append(aFilename);

    var ios = Components.classes["@mozilla.org/network/io-service;1"].
                getService(Components.interfaces.nsIIOService);
    return ios.newFileURI(tmpDir).QueryInterface(Ci.nsIFileURL);
  }
}

function get_test_items_in_list(aManager) {
  var tests = "@tests.mozilla.org";

  let view = aManager.document.getElementById("view-port").selectedPanel;
  let listid = view.id == "search-view" ? "search-list" : "addon-list";
  let item = aManager.document.getElementById(listid).firstChild;
  let items = [];

  while (item) {
    if (item.localName != "richlistitem") {
      item = item.nextSibling;
      continue;
    }

    if (!item.mAddon || item.mAddon.id.substring(item.mAddon.id.length - tests.length) == tests)
      items.push(item);
    item = item.nextSibling;
  }

  return items;
}

function check_all_in_list(aManager, aIds, aIgnoreExtras) {
  var doc = aManager.document;
  var view = doc.getElementById("view-port").selectedPanel;
  var listid = view.id == "search-view" ? "search-list" : "addon-list";
  var list = doc.getElementById(listid);

  var inlist = [];
  var node = list.firstChild;
  while (node) {
    if (node.value)
      inlist.push(node.value);
    node = node.nextSibling;
  }

  for (var i = 0; i < aIds.length; i++) {
    if (inlist.indexOf(aIds[i]) == -1)
      ok(false, "Should find " + aIds[i] + " in the list");
  }

  if (aIgnoreExtras)
    return;

  for (i = 0; i < inlist.length; i++) {
    if (aIds.indexOf(inlist[i]) == -1)
      ok(false, "Shouldn't have seen " + inlist[i] + " in the list");
  }
}

function get_addon_element(aManager, aId) {
  var doc = aManager.document;
  var view = doc.getElementById("view-port").selectedPanel;
  var listid = "addon-list";
  if (view.id == "search-view")
    listid = "search-list";
  else if (view.id == "updates-view")
    listid = "updates-list";
  var list = doc.getElementById(listid);

  var node = list.firstChild;
  while (node) {
    if (node.value == aId)
      return node;
    node = node.nextSibling;
  }
  return null;
}

function wait_for_view_load(aManagerWindow, aCallback, aForceWait) {
  if (!aForceWait && !aManagerWindow.gViewController.isLoading) {
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

  info("Waiting for initialization");
  aManagerWindow.document.addEventListener("Initialized", function() {
    aManagerWindow.document.removeEventListener("Initialized", arguments.callee, false);
    aCallback(aManagerWindow);
  }, false);
}

function open_manager(aView, aCallback, aLoadCallback) {
  function setup_manager(aManagerWindow) {
    if (aLoadCallback)
      aLoadCallback(aManagerWindow);

    if (aView)
      aManagerWindow.loadView(aView);

    ok(aManagerWindow != null, "Should have an add-ons manager window");
    is(aManagerWindow.location, MANAGER_URI, "Should be displaying the correct UI");

    wait_for_manager_load(aManagerWindow, function() {
      wait_for_view_load(aManagerWindow, aCallback);
    });
  }

  if (gUseInContentUI) {
    gBrowser.selectedTab = gBrowser.addTab();
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

function restart_manager(aManagerWindow, aView, aCallback, aLoadCallback) {
  if (!aManagerWindow) {
    open_manager(aView, aCallback, aLoadCallback);
    return;
  }

  close_manager(aManagerWindow, function() {
    open_manager(aView, aCallback, aLoadCallback);
  });
}

function is_hidden(aElement) {
  var style = aElement.ownerDocument.defaultView.getComputedStyle(aElement, "");
  if (style.display == "none")
    return true;
  if (style.visibility != "visible")
    return true;

  
  if (aElement.parentNode != aElement.ownerDocument)
    return is_hidden(aElement.parentNode);

  return false;
}

function is_element_visible(aElement, aMsg) {
  isnot(aElement, null, "Element should not be null, when checking visibility");
  ok(!is_hidden(aElement), aMsg);
}

function is_element_hidden(aElement, aMsg) {
  isnot(aElement, null, "Element should not be null, when checking visibility");
  ok(is_hidden(aElement), aMsg);
}

function CategoryUtilities(aManagerWindow) {
  this.window = aManagerWindow;

  var self = this;
  this.window.addEventListener("unload", function() {
    self.window.removeEventListener("unload", arguments.callee, false);
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

    return !is_hidden(aCategory);
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
    var oldAddons = this.addons.filter(function(aOldAddon) aOldAddon.id == aAddon.id);
    var oldAddon = oldAddons.length > 0 ? oldAddons[0] : null;

    this.addons = this.addons.filter(function(aOldAddon) aOldAddon.id != aAddon.id);

    this.addons.push(aAddon);
    aAddon._provider = this;

    if (!this.started)
      return;

    let requiresRestart = (aAddon.operationsRequiringRestart &
                           AddonManager.OP_NEEDS_RESTART_INSTALL) != 0;
    AddonManagerPrivate.callInstallListeners("onExternalInstall", null, aAddon,
                                             oldAddon, requiresRestart)
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
        if (prop == "applyBackgroundUpdates") {
          addon._applyBackgroundUpdates = aAddonProp[prop];
          continue;
        }
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
  this._applyBackgroundUpdates = AddonManager.AUTOUPDATE_ENABLE;
  this.scope = AddonManager.SCOPE_PROFILE;
  this.isActive = true;
  this.creator = "";
  this.pendingOperations = 0;
  this._permissions = AddonManager.PERM_CAN_UNINSTALL |
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

  get permissions() {
    let permissions = this._permissions;
    if (this.appDisabled || !this._userDisabled)
      permissions &= ~AddonManager.PERM_CAN_ENABLE;
    if (this.appDisabled || this._userDisabled)
      permissions &= ~AddonManager.PERM_CAN_DISABLE;
    return permissions;
  },

  set permissions(val) {
    return this._permissions = val;
  },

  get applyBackgroundUpdates() {
    return this._applyBackgroundUpdates;
  },
  
  set applyBackgroundUpdates(val) {
    if (val != AddonManager.AUTOUPDATE_DEFAULT &&
        val != AddonManager.AUTOUPDATE_DISABLE &&
        val != AddonManager.AUTOUPDATE_ENABLE) {
      ok(false, "addon.applyBackgroundUpdates set to an invalid value: " + val);
    }
    this._applyBackgroundUpdates = val;
    AddonManagerPrivate.callAddonListeners("onPropertyChanged", this, ["applyBackgroundUpdates"]);
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
      this.pendingOperations -= (newActive ? AddonManager.PENDING_DISABLE : AddonManager.PENDING_ENABLE);
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

