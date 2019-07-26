



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


const UITOUR_PERMISSION   = "uitour";
const PREF_PERM_BRANCH    = "browser.uitour.";
const MAX_BUTTONS         = 4;


this.UITour = {
  originTabs: new WeakMap(),
  pinnedTabs: new WeakMap(),
  urlbarCapture: new WeakMap(),
  appMenuOpenForAnnotation: new Set(),

  highlightEffects: ["random", "wobble", "zoom", "color"],
  targets: new Map([
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
        if (!element || !_isElementVisible(element)) {
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

    switch (action) {
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

    let tab = window.gBrowser._getTabForContentWindow(contentDocument.defaultView);
    if (!this.originTabs.has(window))
      this.originTabs.set(window, new Set());
    this.originTabs.get(window).add(tab);

    tab.addEventListener("TabClose", this);
    window.gBrowser.tabContainer.addEventListener("TabSelect", this);
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

      case "TabClose": {
        let window = aEvent.target.ownerDocument.defaultView;
        this.teardownTour(window);
        break;
      }

      case "TabSelect": {
        let window = aEvent.target.ownerDocument.defaultView;
        let pinnedTab = this.pinnedTabs.get(window);
        if (pinnedTab && pinnedTab.tab == window.gBrowser.selectedTab)
          break;
        let originTabs = this.originTabs.get(window);
        if (originTabs && originTabs.has(window.gBrowser.selectedTab))
          break;

        this.teardownTour(window);
        break;
      }

      case "SSWindowClosing": {
        let window = aEvent.target;
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

  teardownTour: function(aWindow, aWindowClosing = false) {
    aWindow.gBrowser.tabContainer.removeEventListener("TabSelect", this);
    aWindow.removeEventListener("SSWindowClosing", this);

    let originTabs = this.originTabs.get(aWindow);
    if (originTabs) {
      for (let tab of originTabs)
        tab.removeEventListener("TabClose", this);
    }
    this.originTabs.delete(aWindow);

    if (!aWindowClosing) {
      this.hideHighlight(aWindow);
      this.hideInfo(aWindow);
      aWindow.PanelUI.panel.removeAttribute("noautohide");
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

  getTarget: function(aWindow, aTargetName, aSticky = false) {
    let deferred = Promise.defer();
    if (typeof aTargetName != "string" || !aTargetName) {
      deferred.reject("Invalid target name specified");
      return deferred.promise;
    }

    if (aTargetName == "pinnedTab") {
      deferred.resolve({node: this.ensurePinnedTab(aWindow, aSticky)});
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
          node: targetQuery(aWindow.document),
          widgetName: targetObject.widgetName,
        });
        return;
      }

      deferred.resolve({
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
      highlighter.parentElement.hidden = false;

      let targetRect = aTargetEl.getBoundingClientRect();

      highlighter.style.height = targetRect.height + "px";
      highlighter.style.width = targetRect.width + "px";

      
      if (highlighter.parentElement.state == "open") {
        highlighter.parentElement.hidePopup();
      }
      

      let highlightWindow = aTargetEl.ownerDocument.defaultView;
      let containerStyle = highlightWindow.getComputedStyle(highlighter.parentElement);
      let paddingTopPx = 0 - parseFloat(containerStyle.paddingTop);
      let paddingLeftPx = 0 - parseFloat(containerStyle.paddingLeft);
      let highlightStyle = highlightWindow.getComputedStyle(highlighter);
      let offsetX = paddingTopPx
                      - (Math.max(0, parseFloat(highlightStyle.minWidth) - targetRect.width) / 2);
      let offsetY = paddingLeftPx
                      - (Math.max(0, parseFloat(highlightStyle.minHeight) - targetRect.height) / 2);
      highlighter.parentElement.openPopup(aTargetEl, "overlap", offsetX, offsetY);
    }

    
    if (!_isElementVisible(aTarget.node))
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

        let callbackID = button.callbackID;
        el.addEventListener("command", event => {
          tooltip.hidePopup();
          this.sendPageCallback(aContentDocument, callbackID);
        });

        tooltipButtons.appendChild(el);
      }

      tooltipButtons.hidden = !aButtons.length;

      tooltip.hidden = false;
      let alignment = "bottomcenter topright";
      tooltip.openPopup(aAnchorEl, alignment);
    }

    
    if (!_isElementVisible(aAnchor.node))
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
    } else if (aMenuName == "bookmarks") {
      closeMenuButton("bookmarks-menu-button");
    }
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

function _isElementVisible(aElement) {
  let targetStyle = aElement.ownerDocument.defaultView.getComputedStyle(aElement);
  return (targetStyle.display != "none" && targetStyle.visibility == "visible");
}
