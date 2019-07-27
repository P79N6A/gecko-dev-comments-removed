



"use strict";

this.EXPORTED_SYMBOLS = ["UITour"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LightweightThemeManager",
  "resource://gre/modules/LightweightThemeManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ResetProfile",
  "resource://gre/modules/ResetProfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
  "resource:///modules/CustomizableUI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UITelemetry",
  "resource://gre/modules/UITelemetry.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BrowserUITelemetry",
  "resource:///modules/BrowserUITelemetry.jsm");



const PREF_LOG_LEVEL      = "browser.uitour.loglevel";
const PREF_SEENPAGEIDS    = "browser.uitour.seenPageIDs";
const MAX_BUTTONS         = 4;

const BUCKET_NAME         = "UITour";
const BUCKET_TIMESTEPS    = [
  1 * 60 * 1000, 
  3 * 60 * 1000, 
  10 * 60 * 1000, 
  60 * 60 * 1000, 
];


const SEENPAGEID_EXPIRY  = 8 * 7 * 24 * 60 * 60 * 1000; 


const TARGET_SEARCHENGINE_PREFIX = "searchEngine-";


XPCOMUtils.defineLazyGetter(this, "log", () => {
  let ConsoleAPI = Cu.import("resource://gre/modules/devtools/Console.jsm", {}).ConsoleAPI;
  let consoleOptions = {
    
    maxLogLevel: Services.prefs.getCharPref(PREF_LOG_LEVEL).toLowerCase(),
    prefix: "UITour",
  };
  return new ConsoleAPI(consoleOptions);
});

this.UITour = {
  url: null,
  seenPageIDs: null,
  pageIDSourceTabs: new WeakMap(),
  pageIDSourceWindows: new WeakMap(),
  
  originTabs: new WeakMap(),
  
  pinnedTabs: new WeakMap(),
  urlbarCapture: new WeakMap(),
  appMenuOpenForAnnotation: new Set(),
  availableTargetsCache: new WeakMap(),

  _detachingTab: false,
  _annotationPanelMutationObservers: new WeakMap(),
  _queuedEvents: [],
  _pendingDoc: null,

  highlightEffects: ["random", "wobble", "zoom", "color"],
  targets: new Map([
    ["accountStatus", {
      query: (aDocument) => {
        let statusButton = aDocument.getElementById("PanelUI-fxa-status");
        return aDocument.getAnonymousElementByAttribute(statusButton,
                                                        "class",
                                                        "toolbarbutton-icon");
      },
      widgetName: "PanelUI-fxa-status",
    }],
    ["addons",      {query: "#add-ons-button"}],
    ["appMenu",     {
      addTargetListener: (aDocument, aCallback) => {
        let panelPopup = aDocument.getElementById("PanelUI-popup");
        panelPopup.addEventListener("popupshown", aCallback);
      },
      query: "#PanelUI-button",
      removeTargetListener: (aDocument, aCallback) => {
        let panelPopup = aDocument.getElementById("PanelUI-popup");
        panelPopup.removeEventListener("popupshown", aCallback);
      },
    }],
    ["backForward", {
      query: "#back-button",
      widgetName: "urlbar-container",
    }],
    ["bookmarks",   {query: "#bookmarks-menu-button"}],
    ["customize",   {
      query: (aDocument) => {
        let customizeButton = aDocument.getElementById("PanelUI-customize");
        return aDocument.getAnonymousElementByAttribute(customizeButton,
                                                        "class",
                                                        "toolbarbutton-icon");
      },
      widgetName: "PanelUI-customize",
    }],
    ["help",        {query: "#PanelUI-help"}],
    ["home",        {query: "#home-button"}],
    ["loop",        {query: "#loop-button"}],
    ["devtools",    {query: "#developer-button"}],
    ["webide",      {query: "#webide-button"}],
    ["forget", {
      query: "#panic-button",
      widgetName: "panic-button",
      allowAdd: true }],
    ["privateWindow",  {query: "#privatebrowsing-button"}],
    ["quit",        {query: "#PanelUI-quit"}],
    ["search",      {
      query: "#searchbar",
      widgetName: "search-container",
    }],
    ["searchProvider", {
      query: (aDocument) => {
        let searchbar = aDocument.getElementById("searchbar");
        return aDocument.getAnonymousElementByAttribute(searchbar,
                                                        "anonid",
                                                        "searchbar-engine-button");
      },
      widgetName: "search-container",
    }],
    ["selectedTabIcon", {
      query: (aDocument) => {
        let selectedtab = aDocument.defaultView.gBrowser.selectedTab;
        let element = aDocument.getAnonymousElementByAttribute(selectedtab,
                                                               "anonid",
                                                               "tab-icon-image");
        if (!element || !UITour.isElementVisible(element)) {
          return null;
        }
        return element;
      },
    }],
    ["urlbar",      {
      query: "#urlbar",
      widgetName: "urlbar-container",
    }],
  ]),

  init: function() {
    log.debug("Initializing UITour");
    
    
    delete this.seenPageIDs;
    Object.defineProperty(this, "seenPageIDs", {
      get: this.restoreSeenPageIDs.bind(this),
      configurable: true,
    });

    delete this.url;
    XPCOMUtils.defineLazyGetter(this, "url", function () {
      return Services.urlFormatter.formatURLPref("browser.uitour.url");
    });

    
    let listenerMethods = [
      "onWidgetAdded",
      "onWidgetMoved",
      "onWidgetRemoved",
      "onWidgetReset",
      "onAreaReset",
    ];
    CustomizableUI.addListener(listenerMethods.reduce((listener, method) => {
      listener[method] = () => this.availableTargetsCache.clear();
      return listener;
    }, {}));
  },

  restoreSeenPageIDs: function() {
    delete this.seenPageIDs;

    if (UITelemetry.enabled) {
      let dateThreshold = Date.now() - SEENPAGEID_EXPIRY;

      try {
        let data = Services.prefs.getCharPref(PREF_SEENPAGEIDS);
        data = new Map(JSON.parse(data));

        for (let [pageID, details] of data) {

          if (typeof pageID != "string" ||
              typeof details != "object" ||
              typeof details.lastSeen != "number" ||
              details.lastSeen < dateThreshold) {

            data.delete(pageID);
          }
        }

        this.seenPageIDs = data;
      } catch (e) {}
    }

    if (!this.seenPageIDs)
      this.seenPageIDs = new Map();

    this.persistSeenIDs();

    return this.seenPageIDs;
  },

  addSeenPageID: function(aPageID) {
    if (!UITelemetry.enabled)
      return;

    this.seenPageIDs.set(aPageID, {
      lastSeen: Date.now(),
    });

    this.persistSeenIDs();
  },

  persistSeenIDs: function() {
    if (this.seenPageIDs.size === 0) {
      Services.prefs.clearUserPref(PREF_SEENPAGEIDS);
      return;
    }

    Services.prefs.setCharPref(PREF_SEENPAGEIDS,
                               JSON.stringify([...this.seenPageIDs]));
  },

  onPageEvent: function(aMessage, aEvent) {
    let contentDocument = null;
    let browser = aMessage.target;
    let window = browser.ownerDocument.defaultView;
    let tab = window.gBrowser.getTabForBrowser(browser);
    let messageManager = browser.messageManager;

    log.debug("onPageEvent:", aEvent.detail);

    if (typeof aEvent.detail != "object") {
      log.warn("Malformed event - detail not an object");
      return false;
    }

    let action = aEvent.detail.action;
    if (typeof action != "string" || !action) {
      log.warn("Action not defined");
      return false;
    }

    let data = aEvent.detail.data;
    if (typeof data != "object") {
      log.warn("Malformed event - data not an object");
      return false;
    }

    
    window.gBrowser.tabContainer.addEventListener("TabSelect", this);

    if (!window.gMultiProcessBrowser) { 
      contentDocument = browser.contentWindow.document;
      if (!tab) {
        
        if (this._detachingTab) {
          log.debug("Got event while detatching a tab");
          this._queuedEvents.push(aEvent);
          this._pendingDoc = Cu.getWeakReference(contentDocument);
          return;
        }
        log.error("Discarding tabless UITour event (" + action + ") while not detaching a tab." +
                       "This shouldn't happen!");
        return;
      }
    }

    switch (action) {
      case "registerPageID": {
        
        if (!UITelemetry.enabled) {
          log.debug("registerPageID: Telemery disabled, not doing anything");
          break;
        }

        
        
        if (typeof data.pageID != "string" ||
            data.pageID.contains(BrowserUITelemetry.BUCKET_SEPARATOR)) {
          log.warn("registerPageID: Invalid page ID specified");
          break;
        }

        this.addSeenPageID(data.pageID);

        
        
        this.pageIDSourceTabs.set(tab, data.pageID);
        this.pageIDSourceWindows.set(window, data.pageID);

        this.setTelemetryBucket(data.pageID);

        break;
      }

      case "showHighlight": {
        let targetPromise = this.getTarget(window, data.target);
        targetPromise.then(target => {
          if (!target.node) {
            log.error("UITour: Target could not be resolved: " + data.target);
            return;
          }
          let effect = undefined;
          if (this.highlightEffects.indexOf(data.effect) !== -1) {
            effect = data.effect;
          }
          this.showHighlight(target, effect);
        }).catch(log.error);
        break;
      }

      case "hideHighlight": {
        this.hideHighlight(window);
        break;
      }

      case "showInfo": {
        let targetPromise = this.getTarget(window, data.target, true);
        targetPromise.then(target => {
          if (!target.node) {
            log.error("UITour: Target could not be resolved: " + data.target);
            return;
          }

          let iconURL = null;
          if (typeof data.icon == "string")
            iconURL = this.resolveURL(browser, data.icon);

          let buttons = [];
          if (Array.isArray(data.buttons) && data.buttons.length > 0) {
            for (let buttonData of data.buttons) {
              if (typeof buttonData == "object" &&
                  typeof buttonData.label == "string" &&
                  typeof buttonData.callbackID == "string") {
                let button = {
                  label: buttonData.label,
                  callbackID: buttonData.callbackID,
                };

                if (typeof buttonData.icon == "string")
                  button.iconURL = this.resolveURL(browser, buttonData.icon);

                if (typeof buttonData.style == "string")
                  button.style = buttonData.style;

                buttons.push(button);

                if (buttons.length == MAX_BUTTONS) {
                  log.warn("showInfo: Reached limit of allowed number of buttons");
                  break;
                }
              }
            }
          }

          let infoOptions = {};

          if (typeof data.closeButtonCallbackID == "string")
            infoOptions.closeButtonCallbackID = data.closeButtonCallbackID;
          if (typeof data.targetCallbackID == "string")
            infoOptions.targetCallbackID = data.targetCallbackID;

          this.showInfo(messageManager, target, data.title, data.text, iconURL, buttons, infoOptions);
        }).catch(log.error);
        break;
      }

      case "hideInfo": {
        this.hideInfo(window);
        break;
      }

      case "previewTheme": {
        this.previewTheme(data.theme);
        break;
      }

      case "resetTheme": {
        this.resetTheme();
        break;
      }

      case "addPinnedTab": {
        this.ensurePinnedTab(window, true);
        break;
      }

      case "removePinnedTab": {
        this.removePinnedTab(window);
        break;
      }

      case "showMenu": {
        this.showMenu(window, data.name, () => {
          if (typeof data.showCallbackID == "string")
            this.sendPageCallback(messageManager, data.showCallbackID);
        });
        break;
      }

      case "hideMenu": {
        this.hideMenu(window, data.name);
        break;
      }

      case "startUrlbarCapture": {
        if (typeof data.text != "string" || !data.text ||
            typeof data.url != "string" || !data.url) {
          log.warn("startUrlbarCapture: Text or URL not specified");
          return false;
        }

        let uri = null;
        try {
          uri = Services.io.newURI(data.url, null, null);
        } catch (e) {
          log.warn("startUrlbarCapture: Malformed URL specified");
          return false;
        }

        let secman = Services.scriptSecurityManager;
        let principal = contentDocument.nodePrincipal;
        let flags = secman.DISALLOW_INHERIT_PRINCIPAL;
        try {
          secman.checkLoadURIWithPrincipal(principal, uri, flags);
        } catch (e) {
          log.warn("startUrlbarCapture: Orginating page doesn't have permission to open specified URL");
          return false;
        }

        this.startUrlbarCapture(window, data.text, data.url);
        break;
      }

      case "endUrlbarCapture": {
        this.endUrlbarCapture(window);
        break;
      }

      case "getConfiguration": {
        if (typeof data.configuration != "string") {
          log.warn("getConfiguration: No configuration option specified");
          return false;
        }

        this.getConfiguration(messageManager, window, data.configuration, data.callbackID);
        break;
      }

      case "showFirefoxAccounts": {
        
        
        
        contentDocument.location.href = "about:accounts?action=signup&entrypoint=uitour";
        break;
      }

      case "resetFirefox": {
        
        ResetProfile.openConfirmationDialog(window);
        break;
      }

      case "addNavBarWidget": {
        
        let targetPromise = this.getTarget(window, data.name);
        targetPromise.then(target => {
          this.addNavBarWidget(target, messageManager, data.callbackID);
        }).catch(log.error);
        break;
      }
    }

    if (!window.gMultiProcessBrowser) { 
      if (!this.originTabs.has(window)) {
        this.originTabs.set(window, new Set());
      }

      this.originTabs.get(window).add(tab);
      tab.addEventListener("TabClose", this);
      tab.addEventListener("TabBecomingWindow", this);
      window.addEventListener("SSWindowClosing", this);
    }

    return true;
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "pagehide": {
        let window = this.getChromeWindow(aEvent.target);
        this.teardownTour(window);
        break;
      }

      case "TabBecomingWindow":
        this._detachingTab = true;
        
      case "TabClose": {
        let tab = aEvent.target;
        if (this.pageIDSourceTabs.has(tab)) {
          let pageID = this.pageIDSourceTabs.get(tab);

          
          
          let window = tab.ownerDocument.defaultView;
          if (this.pageIDSourceWindows.get(window) == pageID)
            this.pageIDSourceWindows.delete(window);

          this.setExpiringTelemetryBucket(pageID, "closed");
        }

        let window = tab.ownerDocument.defaultView;
        this.teardownTour(window);
        break;
      }

      case "TabSelect": {
        if (aEvent.detail && aEvent.detail.previousTab) {
          let previousTab = aEvent.detail.previousTab;

          if (this.pageIDSourceTabs.has(previousTab)) {
            let pageID = this.pageIDSourceTabs.get(previousTab);
            this.setExpiringTelemetryBucket(pageID, "inactive");
          }
        }

        let window = aEvent.target.ownerDocument.defaultView;
        let selectedTab = window.gBrowser.selectedTab;
        let pinnedTab = this.pinnedTabs.get(window);
        if (pinnedTab && pinnedTab.tab == selectedTab)
          break;
        let originTabs = this.originTabs.get(window);
        if (originTabs && originTabs.has(selectedTab))
          break;

        let pendingDoc;
        if (this._detachingTab && this._pendingDoc && (pendingDoc = this._pendingDoc.get())) {
          if (selectedTab.linkedBrowser.contentDocument == pendingDoc) {
            if (!this.originTabs.get(window)) {
              this.originTabs.set(window, new Set());
            }
            this.originTabs.get(window).add(selectedTab);
            this.pendingDoc = null;
            this._detachingTab = false;
            while (this._queuedEvents.length) {
              try {
                this.onPageEvent(this._queuedEvents.shift());
              } catch (ex) {
                log.error(ex);
              }
            }
            break;
          }
        }

        this.teardownTour(window);
        break;
      }

      case "SSWindowClosing": {
        let window = aEvent.target;
        if (this.pageIDSourceWindows.has(window)) {
          let pageID = this.pageIDSourceWindows.get(window);
          this.setExpiringTelemetryBucket(pageID, "closed");
        }

        this.teardownTour(window, true);
        break;
      }

      case "input": {
        if (aEvent.target.id == "urlbar") {
          let window = aEvent.target.ownerDocument.defaultView;
          this.handleUrlbarInput(window);
        }
        break;
      }
    }
  },

  setTelemetryBucket: function(aPageID) {
    let bucket = BUCKET_NAME + BrowserUITelemetry.BUCKET_SEPARATOR + aPageID;
    BrowserUITelemetry.setBucket(bucket);
  },

  setExpiringTelemetryBucket: function(aPageID, aType) {
    let bucket = BUCKET_NAME + BrowserUITelemetry.BUCKET_SEPARATOR + aPageID +
                 BrowserUITelemetry.BUCKET_SEPARATOR + aType;

    BrowserUITelemetry.setExpiringBucket(bucket,
                                         BUCKET_TIMESTEPS);
  },

  
  
  getTelemetry: function() {
    return {
      seenPageIDs: [...this.seenPageIDs.keys()],
    };
  },

  teardownTour: function(aWindow, aWindowClosing = false) {
    log.debug("teardownTour: aWindowClosing = " + aWindowClosing);
    aWindow.gBrowser.tabContainer.removeEventListener("TabSelect", this);
    aWindow.PanelUI.panel.removeEventListener("popuphiding", this.hidePanelAnnotations);
    aWindow.PanelUI.panel.removeEventListener("ViewShowing", this.hidePanelAnnotations);
    aWindow.removeEventListener("SSWindowClosing", this);

    let originTabs = this.originTabs.get(aWindow);
    if (originTabs) {
      for (let tab of originTabs) {
        tab.removeEventListener("TabClose", this);
        tab.removeEventListener("TabBecomingWindow", this);
      }
    }
    this.originTabs.delete(aWindow);

    if (!aWindowClosing) {
      this.hideHighlight(aWindow);
      this.hideInfo(aWindow);
      
      this.hideMenu(aWindow, "appMenu");
    }

    this.endUrlbarCapture(aWindow);
    this.removePinnedTab(aWindow);
    this.resetTheme();
  },

  getChromeWindow: function(aContentDocument) {
    return aContentDocument.defaultView
                           .window
                           .QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIWebNavigation)
                           .QueryInterface(Ci.nsIDocShellTreeItem)
                           .rootTreeItem
                           .QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindow)
                           .wrappedJSObject;
  },

  
  isSafeScheme: function(aURI) {
    let allowedSchemes = new Set(["https", "about"]);
    if (!Services.prefs.getBoolPref("browser.uitour.requireSecure"))
      allowedSchemes.add("http");

    if (!allowedSchemes.has(aURI.scheme)) {
      log.error("Unsafe scheme:", aURI.scheme);
      return false;
    }

    return true;
  },

  resolveURL: function(aBrowser, aURL) {
    try {
      let uri = Services.io.newURI(aURL, null, aBrowser.currentURI);

      if (!this.isSafeScheme(uri))
        return null;

      return uri.spec;
    } catch (e) {}

    return null;
  },

  sendPageCallback: function(aMessageManager, aCallbackID, aData = {}) {
    let detail = {data: aData, callbackID: aCallbackID};
    log.debug("sendPageCallback", detail);
    aMessageManager.sendAsyncMessage("UITour:SendPageCallback", detail);
  },

  isElementVisible: function(aElement) {
    let targetStyle = aElement.ownerDocument.defaultView.getComputedStyle(aElement);
    return (targetStyle.display != "none" && targetStyle.visibility == "visible");
  },

  getTarget: function(aWindow, aTargetName, aSticky = false) {
    log.debug("getTarget:", aTargetName);
    let deferred = Promise.defer();
    if (typeof aTargetName != "string" || !aTargetName) {
      log.warn("getTarget: Invalid target name specified");
      deferred.reject("Invalid target name specified");
      return deferred.promise;
    }

    if (aTargetName == "pinnedTab") {
      deferred.resolve({
          targetName: aTargetName,
          node: this.ensurePinnedTab(aWindow, aSticky)
      });
      return deferred.promise;
    }

    if (aTargetName.startsWith(TARGET_SEARCHENGINE_PREFIX)) {
      let engineID = aTargetName.slice(TARGET_SEARCHENGINE_PREFIX.length);
      return this.getSearchEngineTarget(aWindow, engineID);
    }

    let targetObject = this.targets.get(aTargetName);
    if (!targetObject) {
      log.warn("getTarget: The specified target name is not in the allowed set");
      deferred.reject("The specified target name is not in the allowed set");
      return deferred.promise;
    }

    let targetQuery = targetObject.query;
    aWindow.PanelUI.ensureReady().then(() => {
      let node;
      if (typeof targetQuery == "function") {
        try {
          node = targetQuery(aWindow.document);
        } catch (ex) {
          log.warn("getTarget: Error running target query:", ex);
          node = null;
        }
      } else {
        node = aWindow.document.querySelector(targetQuery);
      }

      deferred.resolve({
        addTargetListener: targetObject.addTargetListener,
        node: node,
        removeTargetListener: targetObject.removeTargetListener,
        targetName: aTargetName,
        widgetName: targetObject.widgetName,
        allowAdd: targetObject.allowAdd,
      });
    }).catch(log.error);
    return deferred.promise;
  },

  targetIsInAppMenu: function(aTarget) {
    let placement = CustomizableUI.getPlacementOfWidget(aTarget.widgetName || aTarget.node.id);
    if (placement && placement.area == CustomizableUI.AREA_PANEL) {
      return true;
    }

    let targetElement = aTarget.node;
    
    if (aTarget.widgetName) {
      targetElement = aTarget.node.ownerDocument.getElementById(aTarget.widgetName);
    }

    
    return targetElement.id.startsWith("PanelUI-")
             && targetElement.id != "PanelUI-button";
  },

  



  _setAppMenuStateForAnnotation: function(aWindow, aAnnotationType, aShouldOpenForHighlight, aCallback = null) {
    log.debug("_setAppMenuStateForAnnotation:", aAnnotationType);
    log.debug("_setAppMenuStateForAnnotation: Menu is exptected to be:", aShouldOpenForHighlight ? "open" : "closed");

    
    let panelIsOpen = aWindow.PanelUI.panel.state != "closed";
    if (aShouldOpenForHighlight == panelIsOpen) {
      log.debug("_setAppMenuStateForAnnotation: Panel already in expected state");
      if (aCallback)
        aCallback();
      return;
    }

    
    if (!aShouldOpenForHighlight && !this.appMenuOpenForAnnotation.has(aAnnotationType)) {
      log.debug("_setAppMenuStateForAnnotation: Menu not opened by us, not closing");
      if (aCallback)
        aCallback();
      return;
    }

    if (aShouldOpenForHighlight) {
      this.appMenuOpenForAnnotation.add(aAnnotationType);
    } else {
      this.appMenuOpenForAnnotation.delete(aAnnotationType);
    }

    
    if (this.appMenuOpenForAnnotation.size) {
      log.debug("_setAppMenuStateForAnnotation: Opening the menu");
      this.showMenu(aWindow, "appMenu", aCallback);
    } else {
      log.debug("_setAppMenuStateForAnnotation: Closing the menu");
      this.hideMenu(aWindow, "appMenu");
      if (aCallback)
        aCallback();
    }

  },

  previewTheme: function(aTheme) {
    let origin = Services.prefs.getCharPref("browser.uitour.themeOrigin");
    let data = LightweightThemeManager.parseTheme(aTheme, origin);
    if (data)
      LightweightThemeManager.previewTheme(data);
  },

  resetTheme: function() {
    LightweightThemeManager.resetPreview();
  },

  ensurePinnedTab: function(aWindow, aSticky = false) {
    let tabInfo = this.pinnedTabs.get(aWindow);

    if (tabInfo) {
      tabInfo.sticky = tabInfo.sticky || aSticky;
    } else {
      let url = Services.urlFormatter.formatURLPref("browser.uitour.pinnedTabUrl");

      let tab = aWindow.gBrowser.addTab(url);
      aWindow.gBrowser.pinTab(tab);
      tab.addEventListener("TabClose", () => {
        this.pinnedTabs.delete(aWindow);
      });

      tabInfo = {
        tab: tab,
        sticky: aSticky
      };
      this.pinnedTabs.set(aWindow, tabInfo);
    }

    return tabInfo.tab;
  },

  removePinnedTab: function(aWindow) {
    let tabInfo = this.pinnedTabs.get(aWindow);
    if (tabInfo)
      aWindow.gBrowser.removeTab(tabInfo.tab);
  },

  




  showHighlight: function(aTarget, aEffect = "none") {
    let window = aTarget.node.ownerDocument.defaultView;

    function showHighlightPanel() {
      if (aTarget.targetName.startsWith(TARGET_SEARCHENGINE_PREFIX)) {
        
        
        this.hideHighlight(window);
        aTarget.node.setAttribute("_moz-menuactive", true);
        return;
      }

      
      
      this._hideSearchEngineHighlight(window);

      let highlighter = aTarget.node.ownerDocument.getElementById("UITourHighlight");

      let effect = aEffect;
      if (effect == "random") {
        
        let randomEffect = 1 + Math.floor(Math.random() * (this.highlightEffects.length - 1));
        if (randomEffect == this.highlightEffects.length)
          randomEffect--; 
        effect = this.highlightEffects[randomEffect];
      }
      
      highlighter.setAttribute("active", "none");
      aTarget.node.ownerDocument.defaultView.getComputedStyle(highlighter).animationName;
      highlighter.setAttribute("active", effect);
      highlighter.parentElement.setAttribute("targetName", aTarget.targetName);
      highlighter.parentElement.hidden = false;

      let highlightAnchor;
      
      if (aTarget.node.getAttribute("overflowedItem")) {
        let doc = aTarget.node.ownerDocument;
        let placement = CustomizableUI.getPlacementOfWidget(aTarget.widgetName || aTarget.node.id);
        let areaNode = doc.getElementById(placement.area);
        highlightAnchor = areaNode.overflowable._chevron;
      } else {
        highlightAnchor = aTarget.node;
      }
      let targetRect = highlightAnchor.getBoundingClientRect();
      let highlightHeight = targetRect.height;
      let highlightWidth = targetRect.width;
      let minDimension = Math.min(highlightHeight, highlightWidth);
      let maxDimension = Math.max(highlightHeight, highlightWidth);

      
      
      if (maxDimension / minDimension <= 3.0) {
        highlightHeight = highlightWidth = maxDimension;
        highlighter.style.borderRadius = "100%";
      } else {
        highlighter.style.borderRadius = "";
      }

      highlighter.style.height = highlightHeight + "px";
      highlighter.style.width = highlightWidth + "px";

      
      if (highlighter.parentElement.state == "showing" || highlighter.parentElement.state == "open") {
        log.debug("showHighlight: Closing previous highlight first");
        highlighter.parentElement.hidePopup();
      }
      

      let highlightWindow = aTarget.node.ownerDocument.defaultView;
      let containerStyle = highlightWindow.getComputedStyle(highlighter.parentElement);
      let paddingTopPx = 0 - parseFloat(containerStyle.paddingTop);
      let paddingLeftPx = 0 - parseFloat(containerStyle.paddingLeft);
      let highlightStyle = highlightWindow.getComputedStyle(highlighter);
      let highlightHeightWithMin = Math.max(highlightHeight, parseFloat(highlightStyle.minHeight));
      let highlightWidthWithMin = Math.max(highlightWidth, parseFloat(highlightStyle.minWidth));
      let offsetX = paddingTopPx
                      - (Math.max(0, highlightWidthWithMin - targetRect.width) / 2);
      let offsetY = paddingLeftPx
                      - (Math.max(0, highlightHeightWithMin - targetRect.height) / 2);
      this._addAnnotationPanelMutationObserver(highlighter.parentElement);
      highlighter.parentElement.openPopup(highlightAnchor, "overlap", offsetX, offsetY);
    }

    
    if (!this.isElementVisible(aTarget.node)) {
      log.warn("showHighlight: Not showing a highlight since the target isn't visible", aTarget);
      return;
    }

    this._setAppMenuStateForAnnotation(aTarget.node.ownerDocument.defaultView, "highlight",
                                       this.targetIsInAppMenu(aTarget),
                                       showHighlightPanel.bind(this));
  },

  hideHighlight: function(aWindow) {
    let tabData = this.pinnedTabs.get(aWindow);
    if (tabData && !tabData.sticky)
      this.removePinnedTab(aWindow);

    let highlighter = aWindow.document.getElementById("UITourHighlight");
    this._removeAnnotationPanelMutationObserver(highlighter.parentElement);
    highlighter.parentElement.hidePopup();
    highlighter.removeAttribute("active");

    this._setAppMenuStateForAnnotation(aWindow, "highlight", false);
    this._hideSearchEngineHighlight(aWindow);
  },

  _hideSearchEngineHighlight: function(aWindow) {
    
    
    let searchMenuBtn = null;
    try {
      searchMenuBtn = this.targets.get("searchProvider").query(aWindow.document);
    } catch (e) {  }
    if (searchMenuBtn) {
      let searchPopup = aWindow.document
                               .getAnonymousElementByAttribute(searchMenuBtn,
                                                               "anonid",
                                                               "searchbar-popup");
      for (let menuItem of searchPopup.children)
        menuItem.removeAttribute("_moz-menuactive");
    }
  },

  











  showInfo: function(aMessageManager, aAnchor, aTitle = "", aDescription = "", aIconURL = "",
                     aButtons = [], aOptions = {}) {
    function showInfoPanel(aAnchorEl) {
      aAnchorEl.focus();

      let document = aAnchorEl.ownerDocument;
      let tooltip = document.getElementById("UITourTooltip");
      let tooltipTitle = document.getElementById("UITourTooltipTitle");
      let tooltipDesc = document.getElementById("UITourTooltipDescription");
      let tooltipIcon = document.getElementById("UITourTooltipIcon");
      let tooltipButtons = document.getElementById("UITourTooltipButtons");

      if (tooltip.state == "showing" || tooltip.state == "open") {
        tooltip.hidePopup();
      }

      tooltipTitle.textContent = aTitle || "";
      tooltipDesc.textContent = aDescription || "";
      tooltipIcon.src = aIconURL || "";
      tooltipIcon.hidden = !aIconURL;

      while (tooltipButtons.firstChild)
        tooltipButtons.firstChild.remove();

      for (let button of aButtons) {
        let el = document.createElement("button");
        el.setAttribute("label", button.label);
        if (button.iconURL)
          el.setAttribute("image", button.iconURL);

        if (button.style == "link")
          el.setAttribute("class", "button-link");

        if (button.style == "primary")
          el.setAttribute("class", "button-primary");

        let callbackID = button.callbackID;
        el.addEventListener("command", event => {
          tooltip.hidePopup();
          this.sendPageCallback(aMessageManager, callbackID);
        });

        tooltipButtons.appendChild(el);
      }

      tooltipButtons.hidden = !aButtons.length;

      let tooltipClose = document.getElementById("UITourTooltipClose");
      let closeButtonCallback = (event) => {
        this.hideInfo(document.defaultView);
        if (aOptions && aOptions.closeButtonCallbackID)
          this.sendPageCallback(aMessageManager, aOptions.closeButtonCallbackID);
      };
      tooltipClose.addEventListener("command", closeButtonCallback);

      let targetCallback = (event) => {
        let details = {
          target: aAnchor.targetName,
          type: event.type,
        };
        this.sendPageCallback(aMessageManager, aOptions.targetCallbackID, details);
      };
      if (aOptions.targetCallbackID && aAnchor.addTargetListener) {
        aAnchor.addTargetListener(document, targetCallback);
      }

      tooltip.addEventListener("popuphiding", function tooltipHiding(event) {
        tooltip.removeEventListener("popuphiding", tooltipHiding);
        tooltipClose.removeEventListener("command", closeButtonCallback);
        if (aOptions.targetCallbackID && aAnchor.removeTargetListener) {
          aAnchor.removeTargetListener(document, targetCallback);
        }
      });

      tooltip.setAttribute("targetName", aAnchor.targetName);
      tooltip.hidden = false;
      let alignment = "bottomcenter topright";
      this._addAnnotationPanelMutationObserver(tooltip);
      tooltip.openPopup(aAnchorEl, alignment);
      if (tooltip.state == "closed") {
        document.defaultView.addEventListener("endmodalstate", function endModalStateHandler() {
          document.defaultView.removeEventListener("endmodalstate", endModalStateHandler);
          tooltip.openPopup(aAnchorEl, alignment);
        }, false);
      }
    }

    
    if (!this.isElementVisible(aAnchor.node))
      return;

    
    
    if (aAnchor.targetName.startsWith(TARGET_SEARCHENGINE_PREFIX))
      return;

    this._setAppMenuStateForAnnotation(aAnchor.node.ownerDocument.defaultView, "info",
                                       this.targetIsInAppMenu(aAnchor),
                                       showInfoPanel.bind(this, aAnchor.node));
  },

  hideInfo: function(aWindow) {
    let document = aWindow.document;

    let tooltip = document.getElementById("UITourTooltip");
    this._removeAnnotationPanelMutationObserver(tooltip);
    tooltip.hidePopup();
    this._setAppMenuStateForAnnotation(aWindow, "info", false);

    let tooltipButtons = document.getElementById("UITourTooltipButtons");
    while (tooltipButtons.firstChild)
      tooltipButtons.firstChild.remove();
  },

  showMenu: function(aWindow, aMenuName, aOpenCallback = null) {
    function openMenuButton(aMenuBtn) {
      if (!aMenuBtn || !aMenuBtn.boxObject || aMenuBtn.open) {
        if (aOpenCallback)
          aOpenCallback();
        return;
      }
      if (aOpenCallback)
        aMenuBtn.addEventListener("popupshown", onPopupShown);
      aMenuBtn.boxObject.openMenu(true);
    }
    function onPopupShown(event) {
      this.removeEventListener("popupshown", onPopupShown);
      aOpenCallback(event);
    }

    if (aMenuName == "appMenu") {
      aWindow.PanelUI.panel.setAttribute("noautohide", "true");
      
      if (aWindow.PanelUI.panel.state != "open") {
        this.recreatePopup(aWindow.PanelUI.panel);
      }
      aWindow.PanelUI.panel.addEventListener("popuphiding", this.hidePanelAnnotations);
      aWindow.PanelUI.panel.addEventListener("ViewShowing", this.hidePanelAnnotations);
      if (aOpenCallback) {
        aWindow.PanelUI.panel.addEventListener("popupshown", onPopupShown);
      }
      aWindow.PanelUI.show();
    } else if (aMenuName == "bookmarks") {
      let menuBtn = aWindow.document.getElementById("bookmarks-menu-button");
      openMenuButton(menuBtn);
    } else if (aMenuName == "searchEngines") {
      this.getTarget(aWindow, "searchProvider").then(target => {
        openMenuButton(target.node);
      }).catch(log.error);
    }
  },

  hideMenu: function(aWindow, aMenuName) {
    function closeMenuButton(aMenuBtn) {
      if (aMenuBtn && aMenuBtn.boxObject)
        aMenuBtn.boxObject.openMenu(false);
    }

    if (aMenuName == "appMenu") {
      aWindow.PanelUI.panel.removeAttribute("noautohide");
      aWindow.PanelUI.hide();
      this.recreatePopup(aWindow.PanelUI.panel);
    } else if (aMenuName == "bookmarks") {
      let menuBtn = aWindow.document.getElementById("bookmarks-menu-button");
      closeMenuButton(menuBtn);
    } else if (aMenuName == "searchEngines") {
      let menuBtn = this.targets.get("searchProvider").query(aWindow.document);
      closeMenuButton(menuBtn);
    }
  },

  hidePanelAnnotations: function(aEvent) {
    let win = aEvent.target.ownerDocument.defaultView;
    let annotationElements = new Map([
      
      [win.document.getElementById("UITourHighlightContainer"), UITour.hideHighlight.bind(UITour)],
      [win.document.getElementById("UITourTooltip"), UITour.hideInfo.bind(UITour)],
    ]);
    annotationElements.forEach((hideMethod, annotationElement) => {
      if (annotationElement.state != "closed") {
        let targetName = annotationElement.getAttribute("targetName");
        UITour.getTarget(win, targetName).then((aTarget) => {
          
          
          if (annotationElement.getAttribute("targetName") != aTarget.targetName ||
              annotationElement.state == "closed" ||
              !UITour.targetIsInAppMenu(aTarget)) {
            return;
          }
          hideMethod(win);
        }).catch(log.error);
      }
    });
    UITour.appMenuOpenForAnnotation.clear();
  },

  recreatePopup: function(aPanel) {
    
    
    if (aPanel.hidden) {
      
      
      aPanel.clientWidth; 
      return;
    }
    aPanel.hidden = true;
    aPanel.clientWidth; 
    aPanel.hidden = false;
  },

  startUrlbarCapture: function(aWindow, aExpectedText, aUrl) {
    let urlbar = aWindow.document.getElementById("urlbar");
    this.urlbarCapture.set(aWindow, {
      expected: aExpectedText.toLocaleLowerCase(),
      url: aUrl
    });
    urlbar.addEventListener("input", this);
  },

  endUrlbarCapture: function(aWindow) {
    let urlbar = aWindow.document.getElementById("urlbar");
    urlbar.removeEventListener("input", this);
    this.urlbarCapture.delete(aWindow);
  },

  handleUrlbarInput: function(aWindow) {
    if (!this.urlbarCapture.has(aWindow))
      return;

    let urlbar = aWindow.document.getElementById("urlbar");

    let {expected, url} = this.urlbarCapture.get(aWindow);

    if (urlbar.value.toLocaleLowerCase().localeCompare(expected) != 0)
      return;

    urlbar.handleRevert();

    let tab = aWindow.gBrowser.addTab(url, {
      owner: aWindow.gBrowser.selectedTab,
      relatedToCurrent: true
    });
    aWindow.gBrowser.selectedTab = tab;
  },

  getConfiguration: function(aMessageManager, aWindow, aConfiguration, aCallbackID) {
    switch (aConfiguration) {
      case "availableTargets":
        this.getAvailableTargets(aMessageManager, aWindow, aCallbackID);
        break;
      case "sync":
        this.sendPageCallback(aMessageManager, aCallbackID, {
          setup: Services.prefs.prefHasUserValue("services.sync.username"),
        });
        break;
      case "appinfo":
        let props = ["defaultUpdateChannel", "version"];
        let appinfo = {};
        props.forEach(property => appinfo[property] = Services.appinfo[property]);
        this.sendPageCallback(aMessageManager, aCallbackID, appinfo);
        break;
      default:
        log.error("getConfiguration: Unknown configuration requested: " + aConfiguration);
        break;
    }
  },

  getAvailableTargets: function(aMessageManager, aChromeWindow, aCallbackID) {
    Task.spawn(function*() {
      let window = aChromeWindow;
      let data = this.availableTargetsCache.get(window);
      if (data) {
        log.debug("getAvailableTargets: Using cached targets list", data.targets.join(","));
        this.sendPageCallback(aMessageManager, aCallbackID, data);
        return;
      }

      let promises = [];
      for (let targetName of this.targets.keys()) {
        promises.push(this.getTarget(window, targetName));
      }
      let targetObjects = yield Promise.all(promises);

      let targetNames = [
        "pinnedTab",
      ];

      for (let targetObject of targetObjects) {
        if (targetObject.node)
          targetNames.push(targetObject.targetName);
      }

      targetNames = targetNames.concat(
        yield this.getAvailableSearchEngineTargets(window)
      );

      data = {
        targets: targetNames,
      };
      this.availableTargetsCache.set(window, data);
      this.sendPageCallback(aMessageManager, aCallbackID, data);
    }.bind(this)).catch(err => {
      log.error(err);
      this.sendPageCallback(aMessageManager, aCallbackID, {
        targets: [],
      });
    });
  },

  addNavBarWidget: function (aTarget, aMessageManager, aCallbackID) {
    if (aTarget.node) {
      log.error("UITour: can't add a widget already present: " + data.target);
      return;
    }
    if (!aTarget.allowAdd) {
      log.error("UITour: not allowed to add this widget: " + data.target);
      return;
    }
    if (!aTarget.widgetName) {
      log.error("UITour: can't add a widget without a widgetName property: " + data.target);
      return;
    }

    CustomizableUI.addWidgetToArea(aTarget.widgetName, CustomizableUI.AREA_NAVBAR);
    this.sendPageCallback(aMessageManager, aCallbackID);
  },

  _addAnnotationPanelMutationObserver: function(aPanelEl) {
#ifdef XP_LINUX
    let observer = this._annotationPanelMutationObservers.get(aPanelEl);
    if (observer) {
      return;
    }
    let win = aPanelEl.ownerDocument.defaultView;
    observer = new win.MutationObserver(this._annotationMutationCallback);
    this._annotationPanelMutationObservers.set(aPanelEl, observer);
    let observerOptions = {
      attributeFilter: ["height", "width"],
      attributes: true,
    };
    observer.observe(aPanelEl, observerOptions);
#endif
  },

  _removeAnnotationPanelMutationObserver: function(aPanelEl) {
#ifdef XP_LINUX
    let observer = this._annotationPanelMutationObservers.get(aPanelEl);
    if (observer) {
      observer.disconnect();
      this._annotationPanelMutationObservers.delete(aPanelEl);
    }
#endif
  },






  _annotationMutationCallback: function(aMutations) {
    for (let mutation of aMutations) {
      
      mutation.target.removeAttribute("width");
      mutation.target.removeAttribute("height");
      return;
    }
  },

  getAvailableSearchEngineTargets(aWindow) {
    return new Promise(resolve => {
      this.getTarget(aWindow, "search").then(searchTarget => {
        if (!searchTarget.node || this.targetIsInAppMenu(searchTarget))
          return resolve([]);

        Services.search.init(() => {
          let engines = Services.search.getVisibleEngines();
          resolve([TARGET_SEARCHENGINE_PREFIX + engine.identifier
                   for (engine of engines)
                   if (engine.identifier)]);
        });
      }).catch(() => resolve([]));
    });
  },

  
  
  
  getSearchEngineTarget(aWindow, aIdentifier) {
    return new Promise((resolve, reject) => {
      Task.spawn(function*() {
        let searchTarget = yield this.getTarget(aWindow, "search");
        
        
        
        
        if (!searchTarget.node || this.targetIsInAppMenu(searchTarget))
          return reject("Search engine not available");

        yield Services.search.init();

        let searchPopup = searchTarget.node._popup;
        for (let engineNode of searchPopup.children) {
          let engine = engineNode.engine;
          if (engine && engine.identifier == aIdentifier) {
            return resolve({
              targetName: TARGET_SEARCHENGINE_PREFIX + engine.identifier,
              node: engineNode,
            });
          }
        }
        reject("Search engine not available");
      }.bind(this)).catch(() => {
        reject("Search engine not available");
      });
    });
  }
};

this.UITour.init();
