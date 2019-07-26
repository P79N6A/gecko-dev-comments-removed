



"use strict";

this.EXPORTED_SYMBOLS = ["UITour"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LightweightThemeManager",
  "resource://gre/modules/LightweightThemeManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PermissionsUtils",
  "resource://gre/modules/PermissionsUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
  "resource:///modules/CustomizableUI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UITelemetry",
  "resource://gre/modules/UITelemetry.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BrowserUITelemetry",
  "resource:///modules/BrowserUITelemetry.jsm");


const UITOUR_PERMISSION   = "uitour";
const PREF_PERM_BRANCH    = "browser.uitour.";
const PREF_SEENPAGEIDS    = "browser.uitour.seenPageIDs";
const MAX_BUTTONS         = 4;

const BUCKET_NAME         = "UITour";
const BUCKET_TIMESTEPS    = [
  1 * 60 * 1000, 
  3 * 60 * 1000, 
  10 * 60 * 1000, 
  60 * 60 * 1000, 
];


const SEENPAGEID_EXPIRY  = 2 * 7 * 24 * 60 * 60 * 1000; 


this.UITour = {
  seenPageIDs: null,
  pageIDSourceTabs: new WeakMap(),
  pageIDSourceWindows: new WeakMap(),
  
  originTabs: new WeakMap(),
  
  pinnedTabs: new WeakMap(),
  urlbarCapture: new WeakMap(),
  appMenuOpenForAnnotation: new Set(),

  _detachingTab: false,
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
    ["appMenu",     {query: "#PanelUI-menu-button"}],
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
        if (!element || !this.isElementVisible(element)) {
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
    
    
    delete this.seenPageIDs;
    Object.defineProperty(this, "seenPageIDs", {
      get: this.restoreSeenPageIDs.bind(this),
      configurable: true,
    });

    UITelemetry.addSimpleMeasureFunction("UITour",
                                         this.getTelemetry.bind(this));
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

  onPageEvent: function(aEvent) {
    let contentDocument = null;
    if (aEvent.target instanceof Ci.nsIDOMHTMLDocument)
      contentDocument = aEvent.target;
    else if (aEvent.target instanceof Ci.nsIDOMHTMLElement)
      contentDocument = aEvent.target.ownerDocument;
    else
      return false;

    
    if (!this.ensureTrustedOrigin(contentDocument))
      return false;

    if (typeof aEvent.detail != "object")
      return false;

    let action = aEvent.detail.action;
    if (typeof action != "string" || !action)
      return false;

    let data = aEvent.detail.data;
    if (typeof data != "object")
      return false;

    let window = this.getChromeWindow(contentDocument);
    
    window.gBrowser.tabContainer.addEventListener("TabSelect", this);
    let tab = window.gBrowser._getTabForContentWindow(contentDocument.defaultView);
    if (!tab) {
      
      if (this._detachingTab) {
        this._queuedEvents.push(aEvent);
        this._pendingDoc = Cu.getWeakReference(contentDocument);
        return;
      }
      Cu.reportError("Discarding tabless UITour event (" + action + ") while not detaching a tab." +
                     "This shouldn't happen!");
      return;
    }

    switch (action) {
      case "registerPageID": {
        
        if (!UITelemetry.enabled)
          break;

        
        
        if (typeof data.pageID == "string" &&
            !data.pageID.contains(BrowserUITelemetry.BUCKET_SEPARATOR)) {
          this.addSeenPageID(data.pageID);

          
          
          this.pageIDSourceTabs.set(tab, data.pageID);
          this.pageIDSourceWindows.set(window, data.pageID);

          this.setTelemetryBucket(data.pageID);
        }
        break;
      }

      case "showHighlight": {
        let targetPromise = this.getTarget(window, data.target);
        targetPromise.then(target => {
          if (!target.node) {
            Cu.reportError("UITour: Target could not be resolved: " + data.target);
            return;
          }
          let effect = undefined;
          if (this.highlightEffects.indexOf(data.effect) !== -1) {
            effect = data.effect;
          }
          this.showHighlight(target, effect);
        }).then(null, Cu.reportError);
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
            Cu.reportError("UITour: Target could not be resolved: " + data.target);
            return;
          }

          let iconURL = null;
          if (typeof data.icon == "string")
            iconURL = this.resolveURL(contentDocument, data.icon);

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
                  button.iconURL = this.resolveURL(contentDocument, buttonData.icon);

                if (typeof buttonData.style == "string")
                  button.style = buttonData.style;

                buttons.push(button);

                if (buttons.length == MAX_BUTTONS)
                  break;
              }
            }
          }

          this.showInfo(contentDocument, target, data.title, data.text, iconURL, buttons);
        }).then(null, Cu.reportError);
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
        this.showMenu(window, data.name);
        break;
      }

      case "hideMenu": {
        this.hideMenu(window, data.name);
        break;
      }

      case "startUrlbarCapture": {
        if (typeof data.text != "string" || !data.text ||
            typeof data.url != "string" || !data.url) {
          return false;
        }

        let uri = null;
        try {
          uri = Services.io.newURI(data.url, null, null);
        } catch (e) {
          return false;
        }

        let secman = Services.scriptSecurityManager;
        let principal = contentDocument.nodePrincipal;
        let flags = secman.DISALLOW_INHERIT_PRINCIPAL;
        try {
          secman.checkLoadURIWithPrincipal(principal, uri, flags);
        } catch (e) {
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
          return false;
        }

        this.getConfiguration(contentDocument, data.configuration, data.callbackID);
        break;
      }
    }

    if (!this.originTabs.has(window))
      this.originTabs.set(window, new Set());

    this.originTabs.get(window).add(tab);
    tab.addEventListener("TabClose", this);
    tab.addEventListener("TabBecomingWindow", this);
    window.addEventListener("SSWindowClosing", this);

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
                Cu.reportError(ex);
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

      case "command": {
        if (aEvent.target.id == "UITourTooltipClose") {
          let window = aEvent.target.ownerDocument.defaultView;
          this.hideInfo(window);
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
      aWindow.PanelUI.panel.removeAttribute("noautohide");
      this.recreatePopup(aWindow.PanelUI.panel);
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

  importPermissions: function() {
    try {
      PermissionsUtils.importFromPrefs(PREF_PERM_BRANCH, UITOUR_PERMISSION);
    } catch (e) {
      Cu.reportError(e);
    }
  },

  ensureTrustedOrigin: function(aDocument) {
    if (aDocument.defaultView.top != aDocument.defaultView)
      return false;

    let uri = aDocument.documentURIObject;

    if (uri.schemeIs("chrome"))
      return true;

    if (!this.isSafeScheme(uri))
      return false;

    this.importPermissions();
    let permission = Services.perms.testPermission(uri, UITOUR_PERMISSION);
    return permission == Services.perms.ALLOW_ACTION;
  },

  isSafeScheme: function(aURI) {
    let allowedSchemes = new Set(["https"]);
    if (!Services.prefs.getBoolPref("browser.uitour.requireSecure"))
      allowedSchemes.add("http");

    if (!allowedSchemes.has(aURI.scheme))
      return false;

    return true;
  },

  resolveURL: function(aDocument, aURL) {
    try {
      let uri = Services.io.newURI(aURL, null, aDocument.documentURIObject);

      if (!this.isSafeScheme(uri))
        return null;

      return uri.spec;
    } catch (e) {}

    return null;
  },

  sendPageCallback: function(aDocument, aCallbackID, aData = {}) {
    let detail = Cu.createObjectIn(aDocument.defaultView);
    detail.data = Cu.createObjectIn(detail);

    for (let key of Object.keys(aData))
      detail.data[key] = aData[key];

    Cu.makeObjectPropsNormal(detail.data);
    Cu.makeObjectPropsNormal(detail);

    detail.callbackID = aCallbackID;

    let event = new aDocument.defaultView.CustomEvent("mozUITourResponse", {
      bubbles: true,
      detail: detail
    });

    aDocument.dispatchEvent(event);
  },

  isElementVisible: function(aElement) {
    let targetStyle = aElement.ownerDocument.defaultView.getComputedStyle(aElement);
    return (targetStyle.display != "none" && targetStyle.visibility == "visible");
  },

  getTarget: function(aWindow, aTargetName, aSticky = false) {
    let deferred = Promise.defer();
    if (typeof aTargetName != "string" || !aTargetName) {
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

    let targetObject = this.targets.get(aTargetName);
    if (!targetObject) {
      deferred.reject("The specified target name is not in the allowed set");
      return deferred.promise;
    }

    let targetQuery = targetObject.query;
    aWindow.PanelUI.ensureReady().then(() => {
      if (typeof targetQuery == "function") {
        deferred.resolve({
          targetName: aTargetName,
          node: targetQuery(aWindow.document),
          widgetName: targetObject.widgetName,
        });
        return;
      }

      deferred.resolve({
        targetName: aTargetName,
        node: aWindow.document.querySelector(targetQuery),
        widgetName: targetObject.widgetName,
      });
    }).then(null, Cu.reportError);
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
             && targetElement.id != "PanelUI-menu-button";
  },

  



  _setAppMenuStateForAnnotation: function(aWindow, aAnnotationType, aShouldOpenForHighlight, aCallback = null) {
    
    let panelIsOpen = aWindow.PanelUI.panel.state != "closed";
    if (aShouldOpenForHighlight == panelIsOpen) {
      if (aCallback)
        aCallback();
      return;
    }

    
    if (!aShouldOpenForHighlight && !this.appMenuOpenForAnnotation.has(aAnnotationType)) {
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
      this.showMenu(aWindow, "appMenu", aCallback);
    } else {
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
    function showHighlightPanel(aTargetEl) {
      let highlighter = aTargetEl.ownerDocument.getElementById("UITourHighlight");

      let effect = aEffect;
      if (effect == "random") {
        
        let randomEffect = 1 + Math.floor(Math.random() * (this.highlightEffects.length - 1));
        if (randomEffect == this.highlightEffects.length)
          randomEffect--; 
        effect = this.highlightEffects[randomEffect];
      }
      highlighter.setAttribute("active", effect);
      highlighter.parentElement.setAttribute("targetName", aTarget.targetName);
      highlighter.parentElement.hidden = false;

      let targetRect = aTargetEl.getBoundingClientRect();
      let highlightHeight = targetRect.height;
      let highlightWidth = targetRect.width;
      let minDimension = Math.min(highlightHeight, highlightWidth);
      let maxDimension = Math.max(highlightHeight, highlightWidth);

      
      
      if (maxDimension / minDimension <= 1.4) {
        highlightHeight = highlightWidth = maxDimension;
        highlighter.style.borderRadius = "100%";
      } else {
        highlighter.style.borderRadius = "";
      }

      highlighter.style.height = highlightHeight + "px";
      highlighter.style.width = highlightWidth + "px";

      
      if (highlighter.parentElement.state == "open") {
        highlighter.parentElement.hidePopup();
      }
      

      let highlightWindow = aTargetEl.ownerDocument.defaultView;
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
      highlighter.parentElement.openPopup(aTargetEl, "overlap", offsetX, offsetY);
    }

    
    if (!this.isElementVisible(aTarget.node))
      return;

    this._setAppMenuStateForAnnotation(aTarget.node.ownerDocument.defaultView, "highlight",
                                       this.targetIsInAppMenu(aTarget),
                                       showHighlightPanel.bind(this, aTarget.node));
  },

  hideHighlight: function(aWindow) {
    let tabData = this.pinnedTabs.get(aWindow);
    if (tabData && !tabData.sticky)
      this.removePinnedTab(aWindow);

    let highlighter = aWindow.document.getElementById("UITourHighlight");
    highlighter.parentElement.hidePopup();
    highlighter.removeAttribute("active");

    this._setAppMenuStateForAnnotation(aWindow, "highlight", false);
  },

  showInfo: function(aContentDocument, aAnchor, aTitle = "", aDescription = "", aIconURL = "", aButtons = []) {
    function showInfoPanel(aAnchorEl) {
      aAnchorEl.focus();

      let document = aAnchorEl.ownerDocument;
      let tooltip = document.getElementById("UITourTooltip");
      let tooltipTitle = document.getElementById("UITourTooltipTitle");
      let tooltipDesc = document.getElementById("UITourTooltipDescription");
      let tooltipIcon = document.getElementById("UITourTooltipIcon");
      let tooltipButtons = document.getElementById("UITourTooltipButtons");

      if (tooltip.state == "open") {
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

        let callbackID = button.callbackID;
        el.addEventListener("command", event => {
          tooltip.hidePopup();
          this.sendPageCallback(aContentDocument, callbackID);
        });

        tooltipButtons.appendChild(el);
      }

      tooltipButtons.hidden = !aButtons.length;

      let tooltipClose = document.getElementById("UITourTooltipClose");
      tooltipClose.addEventListener("command", this);

      tooltip.setAttribute("targetName", aAnchor.targetName);
      tooltip.hidden = false;
      let alignment = "bottomcenter topright";
      tooltip.openPopup(aAnchorEl, alignment);
    }

    
    if (!this.isElementVisible(aAnchor.node))
      return;

    this._setAppMenuStateForAnnotation(aAnchor.node.ownerDocument.defaultView, "info",
                                       this.targetIsInAppMenu(aAnchor),
                                       showInfoPanel.bind(this, aAnchor.node));
  },

  hideInfo: function(aWindow) {
    let document = aWindow.document;

    let tooltip = document.getElementById("UITourTooltip");
    tooltip.hidePopup();
    this._setAppMenuStateForAnnotation(aWindow, "info", false);

    let tooltipButtons = document.getElementById("UITourTooltipButtons");
    while (tooltipButtons.firstChild)
      tooltipButtons.firstChild.remove();
  },

  showMenu: function(aWindow, aMenuName, aOpenCallback = null) {
    function openMenuButton(aId) {
      let menuBtn = aWindow.document.getElementById(aId);
      if (!menuBtn || !menuBtn.boxObject) {
        aOpenCallback();
        return;
      }
      if (aOpenCallback)
        menuBtn.addEventListener("popupshown", onPopupShown);
      menuBtn.boxObject.QueryInterface(Ci.nsIMenuBoxObject).openMenu(true);
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
      openMenuButton("bookmarks-menu-button");
    }
  },

  hideMenu: function(aWindow, aMenuName) {
    function closeMenuButton(aId) {
      let menuBtn = aWindow.document.getElementById(aId);
      if (menuBtn && menuBtn.boxObject)
        menuBtn.boxObject.QueryInterface(Ci.nsIMenuBoxObject).openMenu(false);
    }

    if (aMenuName == "appMenu") {
      aWindow.PanelUI.panel.removeAttribute("noautohide");
      aWindow.PanelUI.hide();
      this.recreatePopup(aWindow.PanelUI.panel);
    } else if (aMenuName == "bookmarks") {
      closeMenuButton("bookmarks-menu-button");
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
        }).then(null, Cu.reportError);
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

  getConfiguration: function(aContentDocument, aConfiguration, aCallbackId) {
    let config = null;
    switch (aConfiguration) {
      case "sync":
        config = {
          setup: Services.prefs.prefHasUserValue("services.sync.username"),
        };
        break;
      default:
        Cu.reportError("getConfiguration: Unknown configuration requested: " + aConfiguration);
        break;
    }
    this.sendPageCallback(aContentDocument, aCallbackId, config);
  },
};

this.UITour.init();
