




































const PREFIX_ITEM_URI = "urn:mozilla:item:";
const PREFIX_NS_EM = "http://www.mozilla.org/2004/em-rdf#";

const PREF_GETADDONS_MAXRESULTS = "extensions.getAddons.maxResults";

const kAddonPageSize = 5;

#ifdef ANDROID
const URI_GENERIC_ICON_XPINSTALL = "drawable://alertaddons";
#else
const URI_GENERIC_ICON_XPINSTALL = "chrome://browser/skin/images/alert-addons-30.png";
#endif
const ADDONS_NOTIFICATION_NAME = "addons";

XPCOMUtils.defineLazyGetter(this, "AddonManager", function() {
  Cu.import("resource://gre/modules/AddonManager.jsm");
  return AddonManager;
});

XPCOMUtils.defineLazyGetter(this, "AddonRepository", function() {
  Cu.import("resource://gre/modules/AddonRepository.jsm");
  return AddonRepository;
});

var ExtensionsView = {
  _strings: {},
  _list: null,
  _localItem: null,
  _repoItem: null,
  _msg: null,
  _dloadmgr: null,
  _restartCount: 0,
  _observerIndex: -1,

  _getOpTypeForOperations: function ev__getOpTypeForOperations(aOperations) {
    if (aOperations & AddonManager.PENDING_UNINSTALL)
      return "needs-uninstall";
    if (aOperations & AddonManager.PENDING_ENABLE)
      return "needs-enable";
    if (aOperations & AddonManager.PENDING_DISABLE)
      return "needs-disable";
    return "";
  },

  _createItem: function ev__createItem(aAddon, aTypeName) {
    let item = document.createElement("richlistitem");
    item.setAttribute("id", PREFIX_ITEM_URI + aAddon.id);
    item.setAttribute("addonID", aAddon.id);
    item.setAttribute("typeName", aTypeName);
    item.setAttribute("type", aAddon.type);
    item.setAttribute("typeLabel", this._strings["addonType." + aAddon.type]);
    item.setAttribute("name", aAddon.name);
    item.setAttribute("version", aAddon.version);
    item.setAttribute("iconURL", aAddon.iconURL);
    return item;
  },

  clearSection: function ev_clearSection(aSection) {
    let start = null;
    let end = null;

    if (aSection == "local") {
      start = this._localItem;
      end = this._repoItem;
    }
    else {
      start = this._repoItem;
    }

    while (start.nextSibling != end)
      this._list.removeChild(start.nextSibling);
  },

  _messageActions: function ev__messageActions(aData) {
    if (aData == "addons-restart-app") {
      
      var cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");

      
      if (cancelQuit.data == false) {
        let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
        appStartup.quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
      }
    }
  },

  getElementForAddon: function ev_getElementForAddon(aKey) {
    let element = document.getElementById(PREFIX_ITEM_URI + aKey);
    if (!element && this._list)
      element = this._list.getElementsByAttribute("sourceURL", aKey)[0];
    return element;
  },

  showMessage: function ev_showMessage(aMsg, aValue, aButtonLabel, aShowCloseButton, aNotifyData) {
    let notification = this._msg.getNotificationWithValue(aValue);
    if (notification)
      return;

    let self = this;
    let buttons = null;
    if (aButtonLabel) {
      buttons = [ {
        label: aButtonLabel,
        accessKey: "",
        data: aNotifyData,
        callback: function(aNotification, aButton) {
          self._messageActions(aButton.data);
          return true;
        }
      } ];
    }

    this._msg.appendNotification(aMsg, aValue, "", this._msg.PRIORITY_WARNING_LOW, buttons).hideclose = !aShowCloseButton;
  },

  showRestart: function ev_showRestart(aMode) {
    
    this._restartCount++;

    
    aMode = aMode || "normal";

    if (this._msg) {
      this.hideAlerts();
      let strings = Strings.browser;
      let message = "notificationRestart." + aMode;
      this.showMessage(strings.GetStringFromName(message), "restart-app",
                       strings.GetStringFromName("notificationRestart.button"), false, "addons-restart-app");
    }
  },

  hideRestart: function ev_hideRestart() {
    this._restartCount--;
    if (this._restartCount == 0 && this._msg) {
      let notification = this._msg.getNotificationWithValue("restart-app");
      if (notification)
        notification.close();
    }
  },

  showOptions: function ev_showOptions(aID) {
    this.hideOptions();

    let item = this.getElementForAddon(aID);
    if (!item)
      return;

    
    if (item != this._list.selectedItem)
      this._list.selectedItem = item;

    item.showOptions();
  },

  hideOptions: function ev_hideOptions() {
    if (!this._list)
      return;

    let items = this._list.childNodes;
    for (let i = 0; i < items.length; i++) {
      let item = items[i];
      if (item.hideOptions)
        item.hideOptions();
    }
    this._list.ensureSelectedElementIsVisible();
  },

  get visible() {
    let items = document.getElementById("panel-items");
    if (BrowserUI.isPanelVisible() && items.selectedPanel.id == "addons-container")
      return true;
    return false;
  },

  init: function ev_init() {
    if (this._dloadmgr)
      return;

    this._dloadmgr = new AddonInstallListener();
    AddonManager.addInstallListener(this._dloadmgr);

    
    let os = Services.obs;
    os.addObserver(this, "addon-update-started", false);
    os.addObserver(this, "addon-update-ended", false);

    if (!Services.prefs.getBoolPref("extensions.hideUpdateButton"))
      document.getElementById("addons-update-all").hidden = false;

#ifdef ANDROID
    
    let alertsService = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    let progressListener = alertsService.QueryInterface(Ci.nsIAlertsProgressListener);
    if (progressListener)
      progressListener.onCancel(ADDONS_NOTIFICATION_NAME);
#endif
  },

  delayedInit: function ev__delayedInit() {
    if (this._list)
      return;

    this.init(); 

    this._list = document.getElementById("addons-list");
    this._localItem = document.getElementById("addons-local");
    this._repoItem = document.getElementById("addons-repo");
    this._msg = document.getElementById("addons-messages");

    
    
    let notification = this._msg.getNotificationWithValue("restart-app");
    if (this._restartCount > 0 && !notification) {
      this.showRestart();
      this._restartCount--; 
    }

    let strings = Strings.browser;
    this._strings["addonType.extension"] = strings.GetStringFromName("addonType.2");
    this._strings["addonType.theme"] = strings.GetStringFromName("addonType.4");
    this._strings["addonType.locale"] = strings.GetStringFromName("addonType.8");
    this._strings["addonType.search"] = strings.GetStringFromName("addonType.1024");

    let self = this;
    setTimeout(function() {
      self.getAddonsFromLocal();
      self.getAddonsFromRepo("");
    }, 0);
  },

  uninit: function ev_uninit() {
    let os = Services.obs;
    os.removeObserver(this, "addon-update-started");
    os.removeObserver(this, "addon-update-ended");

    AddonManager.removeInstallListener(this._dloadmgr);
  },

  hideOnSelect: function ev_handleEvent(aEvent) {
    
    if (aEvent.target == this._list)
      this.hideOptions();
  },

  _createLocalAddon: function ev__createLocalAddon(aAddon) {
    let strings = Services.strings.createBundle("chrome://browser/locale/browser.properties");

    let appManaged = (aAddon.scope == AddonManager.SCOPE_APPLICATION);
    let opType = this._getOpTypeForOperations(aAddon.pendingOperations);
    let updateable = (aAddon.permissions & AddonManager.PERM_CAN_UPGRADE) > 0;
    let uninstallable = (aAddon.permissions & AddonManager.PERM_CAN_UNINSTALL) > 0;

    let blocked = "";
    switch(aAddon.blocklistState) {
      case Ci.nsIBlocklistService.STATE_BLOCKED:
        blocked = strings.getString("addonBlocked.blocked")
        break;
      case Ci.nsIBlocklistService.STATE_SOFTBLOCKED:
        blocked = strings.getString("addonBlocked.softBlocked");
        break;
      case Ci.nsIBlocklistService.STATE_OUTDATED:
        blocked = srings.getString("addonBlocked.outdated");
        break;
    }

    let listitem = this._createItem(aAddon, "local");
    listitem.setAttribute("isDisabled", !aAddon.isActive);
    listitem.setAttribute("appDisabled", aAddon.appDisabled);
    listitem.setAttribute("appManaged", appManaged);
    listitem.setAttribute("description", aAddon.description);
    listitem.setAttribute("optionsURL", aAddon.optionsURL);
    listitem.setAttribute("opType", opType);
    listitem.setAttribute("updateable", updateable);
    listitem.setAttribute("isReadonly", !uninstallable);
    if (blocked)
      listitem.setAttribute("blockedStatus", blocked);
    listitem.addon = aAddon;
    return listitem;
  },

  getAddonsFromLocal: function ev_getAddonsFromLocal() {
    this.clearSection("local");

    let self = this;
    AddonManager.getAddonsByTypes(["extension", "theme", "locale"], function(items) {
      let strings = Strings.browser;
      let anyUpdateable = false;
      for (let i = 0; i < items.length; i++) {
        let listitem = self._createLocalAddon(items[i]);
        if ((items[i].permissions & AddonManager.PERM_CAN_UPGRADE) > 0)
          anyUpdateable = true;

        self.addItem(listitem);
      }

      
      let defaults = Services.search.getDefaultEngines({ }).map(function (e) e.name);
      function isDefault(aEngine)
        defaults.indexOf(aEngine.name) != -1

      let defaultDescription = strings.GetStringFromName("addonsSearchEngine.description");

      let engines = Services.search.getEngines({ });
      for (let e = 0; e < engines.length; e++) {
        let engine = engines[e];
        let addon = {};
        addon.id = engine.name;
        addon.type = "search";
        addon.name = engine.name;
        addon.version = "";
        addon.iconURL = engine.iconURI ? engine.iconURI.spec : "";

        let listitem = self._createItem(addon, "searchplugin");
        listitem._engine = engine;
        listitem.setAttribute("isDisabled", engine.hidden ? "true" : "false");
        listitem.setAttribute("appDisabled", "false");
        listitem.setAttribute("appManaged", isDefault(engine));
        listitem.setAttribute("description", engine.description || defaultDescription);
        listitem.setAttribute("optionsURL", "");
        listitem.setAttribute("opType", engine.hidden ? "needs-disable" : "");
        listitem.setAttribute("updateable", "false");
        self.addItem(listitem);
      }

      if (engines.length + items.length == 0)
        self.displaySectionMessage("local", strings.GetStringFromName("addonsLocalNone.label"), null, true);

      if (!anyUpdateable)
        document.getElementById("addons-update-all").disabled = true;
    });
  },

  addItem : function ev_addItem(aItem, aPosition) {
    if (aPosition == "repo")
      return this._list.appendChild(aItem);
    else if (aPosition == "local")
      return this._list.insertBefore(aItem, this._localItem.nextSibling);
    else
      return this._list.insertBefore(aItem, this._repoItem);
  },

  removeItem : function ev_moveItem(aItem) {
    this._list.removeChild(aItem);
  },

  enable: function ev_enable(aItem) {
    let opType;
    if (aItem.getAttribute("type") == "search") {
      aItem.setAttribute("isDisabled", false);
      aItem._engine.hidden = false;
      opType = "needs-enable";
    } else if (aItem.getAttribute("type") == "theme") {
      
      let theme = null;
      let item = this._localItem.nextSibling;
      while (item != this._repoItem) {
        if (item.addon && (item.addon.type == "theme") && (item.addon.isActive)) {
          theme = item;
          break;
        }
        item = item.nextSibling;
      }
      if (theme)
        this.disable(theme);

      aItem.addon.userDisabled = false;
      aItem.setAttribute("isDisabled", false);
    } else {
      aItem.addon.userDisabled = false;
      opType = this._getOpTypeForOperations(aItem.addon.pendingOperations);

      if (aItem.addon.pendingOperations & AddonManager.PENDING_ENABLE) {
        this.showRestart();
      } else {
        aItem.removeAttribute("isDisabled");
        if (aItem.getAttribute("opType") == "needs-disable")
          this.hideRestart();
      };
    }

    aItem.setAttribute("opType", opType);
  },

  disable: function ev_disable(aItem) {
    let opType;
    if (aItem.getAttribute("type") == "search") {
      aItem.setAttribute("isDisabled", true);
      aItem._engine.hidden = true;
      opType = "needs-disable";
    } else if (aItem.getAttribute("type") == "theme") {
      aItem.addon.userDisabled = true;
      aItem.setAttribute("isDisabled", true);
    } else {
      aItem.addon.userDisabled = true;
      opType = this._getOpTypeForOperations(aItem.addon.pendingOperations);

      if (aItem.addon.pendingOperations & AddonManager.PENDING_DISABLE) {
        this.showRestart();
      } else {
        aItem.setAttribute("isDisabled", !aItem.addon.isActive);
        if (aItem.getAttribute("opType") == "needs-enable")
          this.hideRestart();
      }
    }

    aItem.setAttribute("opType", opType);
  },

  uninstall: function ev_uninstall(aItem) {
    let opType;
    if (aItem.getAttribute("type") == "search") {
      
      
      aItem._engine.hidden = false;
      Services.search.removeEngine(aItem._engine);
      
      
    } else {
      if (!aItem.addon) {
        this._list.removeChild(aItem);
        return;
      }

      aItem.addon.uninstall();
      opType = this._getOpTypeForOperations(aItem.addon.pendingOperations);

      if (aItem.addon.pendingOperations & AddonManager.PENDING_UNINSTALL) {
        this.showRestart();

        
        
        if (!aItem.addon.isActive && opType == "")
          opType = "needs-uninstall";

        aItem.setAttribute("opType", opType);
      } else {
        this._list.removeChild(aItem);
      }
    }
  },

  cancelUninstall: function ev_cancelUninstall(aItem) {
    aItem.addon.cancelUninstall();

    this.hideRestart();

    let opType = this._getOpTypeForOperations(aItem.addon.pendingOperations);
    aItem.setAttribute("opType", opType);
  },

  installFromRepo: function ev_installFromRepo(aItem) {
    aItem.install.install();

    
    let opType = aItem.getAttribute("opType");
    if (!opType)
      aItem.setAttribute("opType", "needs-install");
  },

  _isSafeURI: function ev_isSafeURI(aURL) {
    if (!aURL)
      return true;

    try {
      var uri = Services.io.newURI(aURL, null, null);
      var scheme = uri.scheme;
    } catch (ex) {}

    return (uri && (scheme == "http" || scheme == "https" || scheme == "ftp"));
  },

  displaySectionMessage: function ev_displaySectionMessage(aSection, aMessage, aButtonLabel, aHideThrobber) {
    let item = document.createElement("richlistitem");
    item.setAttribute("typeName", "message");
    item.setAttribute("message", aMessage);
    if (aButtonLabel)
      item.setAttribute("button", aButtonLabel);
    else
      item.setAttribute("hidebutton", "true");
    item.setAttribute("hidethrobber", aHideThrobber);

    this.addItem(item, aSection);

    return item;
  },

  getAddonsFromRepo: function ev_getAddonsFromRepo(aTerms, aSelectFirstResult) {
    this.clearSection("repo");

    
    Util.forceOnline();

    if (AddonRepository.isSearching)
      AddonRepository.cancelSearch();

    let strings = Strings.browser;
    if (aTerms) {
      AddonSearchResults.selectFirstResult = aSelectFirstResult;
      this.displaySectionMessage("repo", strings.GetStringFromName("addonsSearchStart.label"), strings.GetStringFromName("addonsSearchStart.button"), false);
      AddonRepository.searchAddons(aTerms, Services.prefs.getIntPref(PREF_GETADDONS_MAXRESULTS), AddonSearchResults);
    }
    else {
      this.displaySectionMessage("repo", strings.GetStringFromName("addonsSearchStart.label"), strings.GetStringFromName("addonsSearchStart.button"), false);
      AddonRepository.retrieveRecommendedAddons(Services.prefs.getIntPref(PREF_GETADDONS_MAXRESULTS), RecommendedSearchResults);
    }
  },

  appendSearchResults: function(aAddons, aShowRating, aShowCount) {
    let urlproperties = [ "iconURL", "homepageURL" ];
    let foundItem = false;
    for (let i = 0; i < aAddons.length; i++) {
      let addon = aAddons[i];

      
      
      let element = ExtensionsView.getElementForAddon(addon.install.sourceURI.spec);
      if (element)
        continue;

      
      if (urlproperties.some(function (p) !this._isSafeURI(addon[p]), this))
        continue;
      if (addon.screenshots && addon.screenshots.length) {
        if (addon.screenshots.some(function (aScreenshot) !this._isSafeURI(aScreenshot), this))
          continue;
      }

      
      let types = {"2":"extension", "4":"theme", "8":"locale"};
      addon.type = types[addon.type];

      let listitem = this._createItem(addon, "search");
      listitem.setAttribute("description", addon.description);
      if (addon.homepageURL)
        listitem.setAttribute("homepageURL", addon.homepageURL);
      listitem.install = addon.install;
      listitem.setAttribute("sourceURL", addon.install.sourceURI.spec);
      if (aShowRating)
        listitem.setAttribute("rating", addon.averageRating);

      let item = this.addItem(listitem, "repo");

      
      
      aShowCount--;
      if (aShowCount < 0)
        item.hidden = true;
    }
  },

  showMoreSearchResults: function showMoreSearchResults() {
    
    let showCount = kAddonPageSize;

    
    let item = this._repoItem.nextSibling;
    while (item && !item.hidden)
      item = item.nextSibling;

    
    while (showCount > 0 && item && item.hidden) {
      showCount--;
      item.hidden = false;
      item = item.nextSibling;
    }

    
    if (item == this._list.lastChild)
      item.setAttribute("hidepage", "true");
  },

  displayRecommendedResults: function ev_displaySearchResults(aRecommendedAddons, aBrowseAddons) {
    this.clearSection("repo");

    let formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].getService(Ci.nsIURLFormatter);
    let browseURL = formatter.formatURLPref("extensions.getAddons.browseAddons");

    let strings = Strings.browser;
    let brandShortName = Strings.brand.GetStringFromName("brandShortName");

    let whatare = document.createElement("richlistitem");
    whatare.setAttribute("typeName", "banner");
    whatare.setAttribute("label", strings.GetStringFromName("addonsWhatAre.label"));

    let desc = strings.GetStringFromName("addonsWhatAre.description");
    desc = desc.replace(/#1/g, brandShortName);
    whatare.setAttribute("description", desc);

    whatare.setAttribute("button", strings.GetStringFromName("addonsWhatAre.button"));
    whatare.setAttribute("onbuttoncommand", "BrowserUI.newTab('" + browseURL + "');");
    this.addItem(whatare, "repo");

    if (aRecommendedAddons.length == 0 && aBrowseAddons.length == 0) {
      let msg = strings.GetStringFromName("addonsSearchNone.recommended");
      let button = strings.GetStringFromName("addonsSearchNone.button");
      let item = this.displaySectionMessage("repo", msg, button, true);

      this._list.scrollBoxObject.scrollToElement(item);
      return;
    }

    
    function nameCompare(a, b) {
      return String.localeCompare(a.name, b.name);
    }
    aRecommendedAddons.sort(nameCompare);

    
    function ratingCompare(a, b) {
      return a.averageRating < b.averageRating;
    }
    aBrowseAddons.sort(ratingCompare);

    
    
    this.appendSearchResults(aRecommendedAddons, false, aRecommendedAddons.length);
    this.appendSearchResults(aBrowseAddons, true, (aRecommendedAddons.length >= kAddonPageSize ? 0 : kAddonPageSize));

    let totalAddons = aRecommendedAddons.length + aBrowseAddons.length;

    let showmore = document.createElement("richlistitem");
    showmore.setAttribute("typeName", "showmore");
    showmore.setAttribute("pagelabel", strings.GetStringFromName("addonsBrowseAll.seeMore"));
    showmore.setAttribute("onpagecommand", "ExtensionsView.showMoreSearchResults();");
    showmore.setAttribute("hidepage", totalAddons > kAddonPageSize ? "false" : "true");
    showmore.setAttribute("sitelabel", strings.GetStringFromName("addonsBrowseAll.browseSite"));
    showmore.setAttribute("onsitecommand", "ExtensionsView.showMoreResults('" + browseURL + "');");
    this.addItem(showmore, "repo");

    let evt = document.createEvent("Events");
    evt.initEvent("ViewChanged", true, false);
    this._list.dispatchEvent(evt);
  },

  displaySearchResults: function ev_displaySearchResults(aAddons, aTotalResults, aSelectFirstResult) {
    this.clearSection("repo");

    let strings = Strings.browser;
    if (aAddons.length == 0) {
      let msg = strings.GetStringFromName("addonsSearchNone.search");
      let button = strings.GetStringFromName("addonsSearchSuccess2.button");
      let item = this.displaySectionMessage("repo", msg, button, true);

      if (aSelectFirstResult)
        this._list.scrollBoxObject.scrollToElement(item);
      return;
    }

    let firstItem = this.appendSearchResults(aAddons, true);
    if (aSelectFirstResult) {
      this._list.selectItem(firstItem);
      this._list.scrollBoxObject.scrollToElement(firstItem);
    }

    let formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].getService(Ci.nsIURLFormatter);

    if (aTotalResults > aAddons.length) {
      let showmore = document.createElement("richlistitem");
      showmore.setAttribute("typeName", "showmore");
      showmore.setAttribute("hidepage", "true");

      let labelBase = strings.GetStringFromName("addonsSearchMore.label");
      let label = PluralForm.get(aTotalResults, labelBase).replace("#1", aTotalResults);

      showmore.setAttribute("sitelabel", label);

      let url = Services.prefs.getCharPref("extensions.getAddons.search.browseURL");
      url = url.replace(/%TERMS%/g, encodeURIComponent(this.searchBox.value));
      url = formatter.formatURL(url);
      showmore.setAttribute("onsitecommand", "ExtensionsView.showMoreResults('" + url + "');");
      this.addItem(showmore, "repo");
    }

    this.displaySectionMessage("repo", null, strings.GetStringFromName("addonsSearchSuccess2.button"), true);
  },

  showPage: function ev_showPage(aItem) {
    let uri = aItem.getAttribute("homepageURL");
    if (uri)
      BrowserUI.newTab(uri);
  },

  get searchBox() {
    delete this.searchBox;
    return this.searchBox = document.getElementById("addons-search-text");
  },

  doSearch: function ev_doSearch(aTerms) {
    this.searchBox.value = aTerms;
    this.getAddonsFromRepo(aTerms, true);
  },

  resetSearch: function ev_resetSearch() {
    this.searchBox.value = "";
    this.getAddonsFromRepo("");
  },

  showMoreResults: function ev_showMoreResults(aURL) {
    if (aURL)
      BrowserUI.newTab(aURL);
  },

  updateAll: function ev_updateAll() {
    let aus = Cc["@mozilla.org/browser/addon-update-service;1"].getService(Ci.nsITimerCallback);
    aus.notify(null);

    if (this._list.selectedItem)
      this._list.selectedItem.focus();
  },

  observe: function ev_observe(aSubject, aTopic, aData) {
    if (!document)
      return;

    let json = aSubject.QueryInterface(Ci.nsISupportsString).data;
    let update = JSON.parse(json);

    let strings = Strings.browser;
    let element = this.getElementForAddon(update.id);
    if (!element)
      return;

    let addon = element.addon;

    switch (aTopic) {
      case "addon-update-started":
        element.setAttribute("updateStatus", strings.GetStringFromName("addonUpdate.checking"));
        break;
      case "addon-update-ended":
        let updateable = false;
        let statusMsg = null;
        switch (aData) {
          case "update":
            statusMsg = strings.formatStringFromName("addonUpdate.updating", [update.version], 1);
            updateable = true;
            break;
          case "compatibility":
            if (addon.pendingOperations & AddonManager.PENDING_INSTALL || addon.pendingOperations & AddonManager.PENDING_UPGRADE)
              updateable = true;

            
            if (addon.pendingOperations & AddonManager.PENDING_ENABLE &&
                addon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_ENABLE) {
              statusMsg = strings.GetStringFromName("addonUpdate.compatibility");
              this.showRestart();
            }
            break;
          case "error":
            statusMsg = strings.GetStringFromName("addonUpdate.error");
            break;
          case "no-update":
            
            
            break;
          default:
            
            
        }

        if (statusMsg)
          element.setAttribute("updateStatus", statusMsg);
        else
          element.removeAttribute("updateStatus");

        
        if (updateable)
          element.setAttribute("updating", "true");
        break;
    }
  },

  showAlert: function ev_showAlert(aMessage, aForceDisplay) {
    let strings = Strings.browser;

    let observer = {
      observe: function (aSubject, aTopic, aData) {
        if (aTopic == "alertclickcallback")
          BrowserUI.showPanel("addons-container");
      }
    };

    if (aForceDisplay) {
      
      let toaster = Cc["@mozilla.org/toaster-alerts-service;1"].getService(Ci.nsIAlertsService);
      let image = "chrome://browser/skin/images/alert-addons-30.png";
      if (this.visible)
        toaster.showAlertNotification(image, strings.GetStringFromName("alertAddons"), aMessage, false, "", null);
      else
        toaster.showAlertNotification(image, strings.GetStringFromName("alertAddons"), aMessage, true, "", observer);
    } else {
      
      if (!this.visible) {
        let alerts = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
        alerts.showAlertNotification(URI_GENERIC_ICON_XPINSTALL, strings.GetStringFromName("alertAddons"),
                                     aMessage, true, "", observer, ADDONS_NOTIFICATION_NAME);
      }
    }
  },

  hideAlerts: function ev_hideAlerts() {
#ifdef ANDROID
    let alertsService = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    let progressListener = alertsService.QueryInterface(Ci.nsIAlertsProgressListener);
    progressListener.onCancel(ADDONS_NOTIFICATION_NAME);
#endif
  },
};


function searchFailed() {
  ExtensionsView.clearSection("repo");

  let strings = Strings.browser;
  let brand = Strings.brand;

  let failLabel = strings.formatStringFromName("addonsSearchFail.label",
                                             [brand.GetStringFromName("brandShortName")], 1);
  let failButton = strings.GetStringFromName("addonsSearchFail.button");
  ExtensionsView.displaySectionMessage("repo", failLabel, failButton, true);
}




var RecommendedSearchResults = {
  cache: null,

  searchSucceeded: function(aAddons, aAddonCount, aTotalResults) {
    this.cache = aAddons;
    AddonRepository.searchAddons(" ", Services.prefs.getIntPref(PREF_GETADDONS_MAXRESULTS), BrowseSearchResults);
  },

  searchFailed: searchFailed
}



var BrowseSearchResults = {
  searchSucceeded: function(aAddons, aAddonCount, aTotalResults) {
    ExtensionsView.displayRecommendedResults(RecommendedSearchResults.cache, aAddons);
  },

  searchFailed: searchFailed
}



var AddonSearchResults = {
  
  selectFirstResult: false,

  searchSucceeded: function(aAddons, aAddonCount, aTotalResults) {
    ExtensionsView.displaySearchResults(aAddons, aTotalResults, false, this.selectFirstResult);
  },

  searchFailed: searchFailed
}



function AddonInstallListener() {
}

AddonInstallListener.prototype = {

  onInstallEnded: function(aInstall, aAddon) {
    let needsRestart = false;
    let mode = "";
    if (aInstall.existingAddon && (aInstall.existingAddon.pendingOperations & AddonManager.PENDING_UPGRADE)) {
      needsRestart = true;
      mode = "update";
    } else if (aAddon.pendingOperations & AddonManager.PENDING_INSTALL) {
      needsRestart = true;
      mode = "normal";
    }

    
    
    if (needsRestart)
      ExtensionsView.showRestart(mode);
    this._showInstallCompleteAlert(true, needsRestart);

    
    if (!ExtensionsView._list)
      return;

    let element = ExtensionsView.getElementForAddon(aAddon.id);
    if (!element) {
      element = ExtensionsView._createLocalAddon(aAddon);
      ExtensionsView.addItem(element, "local");
    }

    if (needsRestart) {
      element.setAttribute("opType", "needs-restart");
    } else {
      if (element.getAttribute("typeName") == "search") {
        if (aAddon.permissions & AddonManager.PERM_CAN_UPGRADE)
          document.getElementById("addons-update-all").disabled = false;

        ExtensionsView.removeItem(element);
        element = ExtensionsView._createLocalAddon(aAddon);
        ExtensionsView.addItem(element, "local");
      }
    }

    element.setAttribute("status", "success");

    
    if (element.hasAttribute("updating")) {
      let strings = Services.strings.createBundle("chrome://browser/locale/browser.properties");
      element.setAttribute("updateStatus", strings.getFormattedString("addonUpdate.updated", [aAddon.version]));
      element.removeAttribute("updating");
    }
  },

  onInstallFailed: function(aInstall) {
    this._showInstallCompleteAlert(false);

    if (ExtensionsView.visible) {
      let element = ExtensionsView.getElementForAddon(aInstall.sourceURI.spec);
      if (!element)
        return;

      element.removeAttribute("opType");
      let strings = Services.strings.createBundle("chrome://global/locale/xpinstall/xpinstall.properties");

      let error = null;
      switch (aInstall.error) {
      case AddonManager.ERROR_NETWORK_FAILURE:
        error = "error-228";
        break;
      case AddonManager.ERROR_INCORRECT_HASH:
        error = "error-261";
        break;
      case AddonManager.ERROR_CORRUPT_FILE:
        error = "error-207";
        break;
      }

      try {
        var msg = strings.GetStringFromName(error);
      } catch (ex) {
        msg = strings.formatStringFromName("unknown.error", [aInstall.error], 1);
      }
      element.setAttribute("error", msg);
    }
  },

  onDownloadProgress: function xpidm_onDownloadProgress(aInstall) {
    let element = ExtensionsView.getElementForAddon(aInstall.sourceURI.spec);

#ifdef ANDROID
    let alertsService = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    let progressListener = alertsService.QueryInterface(Ci.nsIAlertsProgressListener);
    progressListener.onProgress(ADDONS_NOTIFICATION_NAME, aInstall.progress, aInstall.maxProgress);
#endif
    if (!element)
      return;

    let opType = element.getAttribute("opType");
    if (!opType)
      element.setAttribute("opType", "needs-install");

    let progress = Math.round((aInstall.progress / aInstall.maxProgress) * 100);
    element.setAttribute("progress", progress);
  },

  onDownloadFailed: function(aInstall) {
    this.onInstallFailed(aInstall);
  },

  onDownloadCancelled: function(aInstall) {
    let strings = Strings.browser;
    let brandBundle = Strings.brand;
    let brandShortName = brandBundle.GetStringFromName("brandShortName");
    let host = (aInstall.originatingURI instanceof Ci.nsIStandardURL) && aInstall.originatingURI.host;
    if (!host)
      host = (aInstall.sourceURI instanceof Ci.nsIStandardURL) && aInstall.sourceURI.host;

    let error = (host || aInstall.error == 0) ? "addonError" : "addonLocalError";
    if (aInstall.error != 0)
      error += aInstall.error;
    else if (aInstall.addon && aInstall.addon.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED)
      error += "Blocklisted";
    else if (aInstall.addon && (!aInstall.addon.isCompatible || !aInstall.addon.isPlatformCompatible))
      error += "Incompatible";
    else {
      ExtensionsView.hideAlerts();
      return; 
    }

    let messageString = strings.GetStringFromName(error);
    messageString = messageString.replace("#1", aInstall.name);
    if (host)
      messageString = messageString.replace("#2", host);
    messageString = messageString.replace("#3", brandShortName);
    messageString = messageString.replace("#4", Services.appinfo.version);

    ExtensionsView.showAlert(messageString);
  },

  _showInstallCompleteAlert: function xpidm_showAlert(aSucceeded, aNeedsRestart) {
    let strings = Strings.browser;
    let stringName = "alertAddonsFail";
    if (aSucceeded) {
      stringName = "alertAddonsInstalled";
      if (!aNeedsRestart)
        stringName += "NoRestart";
    }

    ExtensionsView.showAlert(strings.GetStringFromName(stringName), !aNeedsRestart);
  },
};
