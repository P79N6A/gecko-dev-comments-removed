



"use strict";

this.EXPORTED_SYMBOLS = ["CustomizableUI"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PanelWideWidgetTracker",
  "resource:///modules/PanelWideWidgetTracker.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CustomizableWidgets",
  "resource:///modules/CustomizableWidgets.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DeferredTask",
  "resource://gre/modules/DeferredTask.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyGetter(this, "gWidgetsBundle", function() {
  const kUrl = "chrome://browser/locale/customizableui/customizableWidgets.properties";
  return Services.strings.createBundle(kUrl);
});
XPCOMUtils.defineLazyModuleGetter(this, "ShortcutUtils",
  "resource://gre/modules/ShortcutUtils.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "gELS",
  "@mozilla.org/eventlistenerservice;1", "nsIEventListenerService");

const kNSXUL = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const kSpecialWidgetPfx = "customizableui-special-";

const kPrefCustomizationState        = "browser.uiCustomization.state";
const kPrefCustomizationAutoAdd      = "browser.uiCustomization.autoAdd";
const kPrefCustomizationDebug        = "browser.uiCustomization.debug";
const kPrefDrawInTitlebar            = "browser.tabs.drawInTitlebar";
const kPrefWebIDEInNavbar            = "devtools.webide.widget.inNavbarByDefault";






const kSubviewEvents = [
  "ViewShowing",
  "ViewHiding"
];





let kVersion = 4;





let gPalette = new Map();





let gAreas = new Map();






let gPlacements = new Map();






let gFuturePlacements = new Map();



let gSupportedWidgetTypes = new Set(["button", "view", "custom"]);





let gPanelsForWindow = new WeakMap();







let gSeenWidgets = new Set();







let gDirtyAreaCache = new Set();






let gPendingBuildAreas = new Map();

let gSavedState = null;
let gRestoring = false;
let gDirty = false;
let gInBatchStack = 0;
let gResetting = false;
let gUndoResetting = false;




let gBuildAreas = new Map();





let gBuildWindows = new Map();

let gNewElementCount = 0;
let gGroupWrapperCache = new Map();
let gSingleWrapperCache = new WeakMap();
let gListeners = new Set();

let gUIStateBeforeReset = {
  uiCustomizationState: null,
  drawInTitlebar: null,
  gUIStateBeforeReset: null,
};

let gModuleName = "[CustomizableUI]";
#include logging.js

let CustomizableUIInternal = {
  initialize: function() {
    LOG("Initializing");

    this.addListener(this);
    this._defineBuiltInWidgets();
    this.loadSavedState();
    this._introduceNewBuiltinWidgets();

    let panelPlacements = [
      "edit-controls",
      "zoom-controls",
      "new-window-button",
      "privatebrowsing-button",
      "save-page-button",
      "print-button",
      "history-panelmenu",
      "fullscreen-button",
      "find-button",
      "preferences-button",
      "add-ons-button",
#ifndef MOZ_DEV_EDITION
      "developer-button",
#endif
    ];

    if (gPalette.has("switch-to-metro-button")) {
      panelPlacements.push("switch-to-metro-button");
    }

#ifdef E10S_TESTING_ONLY
    if (gPalette.has("e10s-button")) {
      let newWindowIndex = panelPlacements.indexOf("new-window-button");
      if (newWindowIndex > -1) {
        panelPlacements.splice(newWindowIndex + 1, 0, "e10s-button");
      }
    }
#endif

    let showCharacterEncoding = Services.prefs.getComplexValue(
      "browser.menu.showCharacterEncoding",
      Ci.nsIPrefLocalizedString
    ).data;
    if (showCharacterEncoding == "true") {
      panelPlacements.push("characterencoding-button");
    }

    this.registerArea(CustomizableUI.AREA_PANEL, {
      anchor: "PanelUI-menu-button",
      type: CustomizableUI.TYPE_MENU_PANEL,
      defaultPlacements: panelPlacements
    }, true);
    PanelWideWidgetTracker.init();

    let navbarPlacements = [
      "urlbar-container",
      "search-container",
#ifdef MOZ_DEV_EDITION
      "developer-button",
#endif
      "bookmarks-menu-button",
      "downloads-button",
      "home-button",
      "loop-button",
    ];

    if (Services.prefs.getBoolPref(kPrefWebIDEInNavbar)) {
      navbarPlacements.push("webide-button");
    }

    this.registerArea(CustomizableUI.AREA_NAVBAR, {
      legacy: true,
      type: CustomizableUI.TYPE_TOOLBAR,
      overflowable: true,
      defaultPlacements: navbarPlacements,
      defaultCollapsed: false,
    }, true);
#ifndef XP_MACOSX
    this.registerArea(CustomizableUI.AREA_MENUBAR, {
      legacy: true,
      type: CustomizableUI.TYPE_TOOLBAR,
      defaultPlacements: [
        "menubar-items",
      ],
      get defaultCollapsed() {
#ifdef MENUBAR_CAN_AUTOHIDE
#if defined(MOZ_WIDGET_GTK) || defined(MOZ_WIDGET_QT)
        return true;
#else
        
        
        return Services.appinfo.OS == "WINNT" &&
               Services.sysinfo.getProperty("version") != "5.1";
#endif
#else
        return false;
#endif
      }
    }, true);
#endif
    this.registerArea(CustomizableUI.AREA_TABSTRIP, {
      legacy: true,
      type: CustomizableUI.TYPE_TOOLBAR,
      defaultPlacements: [
        "tabbrowser-tabs",
        "new-tab-button",
        "alltabs-button",
      ],
      defaultCollapsed: null,
    }, true);
    this.registerArea(CustomizableUI.AREA_BOOKMARKS, {
      legacy: true,
      type: CustomizableUI.TYPE_TOOLBAR,
      defaultPlacements: [
        "personal-bookmarks",
      ],
      defaultCollapsed: true,
    }, true);

    this.registerArea(CustomizableUI.AREA_ADDONBAR, {
      type: CustomizableUI.TYPE_TOOLBAR,
      legacy: true,
      defaultPlacements: ["addonbar-closebutton", "status-bar"],
      defaultCollapsed: false,
    }, true);
  },

  get _builtinToolbars() {
    return new Set([
      CustomizableUI.AREA_NAVBAR,
      CustomizableUI.AREA_BOOKMARKS,
      CustomizableUI.AREA_TABSTRIP,
      CustomizableUI.AREA_ADDONBAR,
#ifndef XP_MACOSX
      CustomizableUI.AREA_MENUBAR,
#endif
    ]);
  },

  _defineBuiltInWidgets: function() {
    for (let widgetDefinition of CustomizableWidgets) {
      this.createBuiltinWidget(widgetDefinition);
    }
  },

  _introduceNewBuiltinWidgets: function() {
    if (!gSavedState || gSavedState.currentVersion >= kVersion) {
      return;
    }

    let currentVersion = gSavedState.currentVersion;
    for (let [id, widget] of gPalette) {
      if (widget._introducedInVersion > currentVersion &&
          widget.defaultArea) {
        let futurePlacements = gFuturePlacements.get(widget.defaultArea);
        if (futurePlacements) {
          futurePlacements.add(id);
        } else {
          gFuturePlacements.set(widget.defaultArea, new Set([id]));
        }
      }
    }

    if (currentVersion < 2) {
      
      CustomizableUI.removeWidgetFromArea("loop-call-button");
    }

    if (currentVersion < 4) {
      CustomizableUI.removeWidgetFromArea("loop-button-throttled");
    }
  },

  wrapWidget: function(aWidgetId) {
    if (gGroupWrapperCache.has(aWidgetId)) {
      return gGroupWrapperCache.get(aWidgetId);
    }

    let provider = this.getWidgetProvider(aWidgetId);
    if (!provider) {
      return null;
    }

    if (provider == CustomizableUI.PROVIDER_API) {
      let widget = gPalette.get(aWidgetId);
      if (!widget.wrapper) {
        widget.wrapper = new WidgetGroupWrapper(widget);
        gGroupWrapperCache.set(aWidgetId, widget.wrapper);
      }
      return widget.wrapper;
    }

    
    let wrapper = new XULWidgetGroupWrapper(aWidgetId);
    gGroupWrapperCache.set(aWidgetId, wrapper);
    return wrapper;
  },

  registerArea: function(aName, aProperties, aInternalCaller) {
    if (typeof aName != "string" || !/^[a-z0-9-_]{1,}$/i.test(aName)) {
      throw new Error("Invalid area name");
    }

    let areaIsKnown = gAreas.has(aName);
    let props = areaIsKnown ? gAreas.get(aName) : new Map();
    const kImmutableProperties = new Set(["type", "legacy", "overflowable"]);
    for (let key in aProperties) {
      if (areaIsKnown && kImmutableProperties.has(key) &&
          props.get(key) != aProperties[key]) {
        throw new Error("An area cannot change the property for '" + key + "'");
      }
      
      
      if (key == "defaultPlacements" && Array.isArray(aProperties[key])) {
        props.set(key, aProperties[key].map(x => this.isSpecialWidget(x) ? this.ensureSpecialWidgetId(x) : x ));
      } else {
        props.set(key, aProperties[key]);
      }
    }
    
    if (!props.has("type")) {
      props.set("type", CustomizableUI.TYPE_TOOLBAR);
    }
    if (props.get("type") == CustomizableUI.TYPE_TOOLBAR) {
      
      
      if (!aInternalCaller && aProperties["defaultCollapsed"]) {
        throw new Error("defaultCollapsed is only allowed for default toolbars.")
      }
      if (!props.has("defaultCollapsed")) {
        props.set("defaultCollapsed", true);
      }
    } else if (props.has("defaultCollapsed")) {
      throw new Error("defaultCollapsed only applies for TYPE_TOOLBAR areas.");
    }
    
    let allTypes = [CustomizableUI.TYPE_TOOLBAR, CustomizableUI.TYPE_MENU_PANEL];
    if (allTypes.indexOf(props.get("type")) == -1) {
      throw new Error("Invalid area type " + props.get("type"));
    }

    
    if (!props.has("defaultPlacements")) {
      props.set("defaultPlacements", []);
    }
    
    if (!Array.isArray(props.get("defaultPlacements"))) {
      throw new Error("Should provide an array of default placements");
    }

    if (!areaIsKnown) {
      gAreas.set(aName, props);

      if (props.get("legacy") && !gPlacements.has(aName)) {
        
        
        if (!gFuturePlacements.has(aName)) {
          gFuturePlacements.set(aName, new Set());
        }
      } else {
        this.restoreStateForArea(aName);
      }

      
      if (gPendingBuildAreas.has(aName)) {
        let pendingNodes = gPendingBuildAreas.get(aName);
        for (let [pendingNode, existingChildren] of pendingNodes) {
          this.registerToolbarNode(pendingNode, existingChildren);
        }
        gPendingBuildAreas.delete(aName);
      }
    }
  },

  unregisterArea: function(aName, aDestroyPlacements) {
    if (typeof aName != "string" || !/^[a-z0-9-_]{1,}$/i.test(aName)) {
      throw new Error("Invalid area name");
    }
    if (!gAreas.has(aName) && !gPlacements.has(aName)) {
      throw new Error("Area not registered");
    }

    
    this.beginBatchUpdate();
    try {
      let placements = gPlacements.get(aName);
      if (placements) {
        
        placements = [...placements];
        placements.forEach(this.removeWidgetFromArea, this);
      }

      
      gAreas.delete(aName);
      
      if (aDestroyPlacements) {
        gPlacements.delete(aName);
      } else {
        
        
        gPlacements.set(aName, placements);
      }
      gFuturePlacements.delete(aName);
      let existingAreaNodes = gBuildAreas.get(aName);
      if (existingAreaNodes) {
        for (let areaNode of existingAreaNodes) {
          this.notifyListeners("onAreaNodeUnregistered", aName, areaNode.customizationTarget,
                               CustomizableUI.REASON_AREA_UNREGISTERED);
        }
      }
      gBuildAreas.delete(aName);
    } finally {
      this.endBatchUpdate(true);
    }
  },

  registerToolbarNode: function(aToolbar, aExistingChildren) {
    let area = aToolbar.id;
    if (gBuildAreas.has(area) && gBuildAreas.get(area).has(aToolbar)) {
      return;
    }
    let document = aToolbar.ownerDocument;
    let areaProperties = gAreas.get(area);

    
    if (!areaProperties) {
      
      
      if (!aToolbar.hasAttribute("defaultset") &&
          !aToolbar.hasAttribute("customindex")) {
        if (!gPendingBuildAreas.has(area)) {
          gPendingBuildAreas.set(area, new Map());
        }
        let pendingNodes = gPendingBuildAreas.get(area);
        pendingNodes.set(aToolbar, aExistingChildren);
        return;
      }
      let props = {type: CustomizableUI.TYPE_TOOLBAR, legacy: true};
      let defaultsetAttribute = aToolbar.getAttribute("defaultset") || "";
      props.defaultPlacements = defaultsetAttribute.split(',').filter(s => s);
      this.registerArea(area, props);
      areaProperties = gAreas.get(area);
    }

    this.beginBatchUpdate();
    try {
      let placements = gPlacements.get(area);
      if (!placements && areaProperties.has("legacy")) {
        let legacyState = aToolbar.getAttribute("currentset");
        if (legacyState) {
          legacyState = legacyState.split(",").filter(s => s);
        }

        
        this.restoreStateForArea(area, legacyState);
        placements = gPlacements.get(area);
      }

      
      
      if (aExistingChildren.length != placements.length ||
          aExistingChildren.every((id, i) => id == placements[i])) {
        gDirtyAreaCache.add(area);
      }

      if (areaProperties.has("overflowable")) {
        aToolbar.overflowable = new OverflowableToolbar(aToolbar);
      }

      this.registerBuildArea(area, aToolbar);

      
      
      
      
      
      
      
      
      if (gDirtyAreaCache.has(area)) {
        this.buildArea(area, placements, aToolbar);
      }
      this.notifyListeners("onAreaNodeRegistered", area, aToolbar.customizationTarget);
      aToolbar.setAttribute("currentset", placements.join(","));
    } finally {
      this.endBatchUpdate();
    }
  },

  buildArea: function(aArea, aPlacements, aAreaNode) {
    let document = aAreaNode.ownerDocument;
    let window = document.defaultView;
    let inPrivateWindow = PrivateBrowsingUtils.isWindowPrivate(window);
    let container = aAreaNode.customizationTarget;
    let areaIsPanel = gAreas.get(aArea).get("type") == CustomizableUI.TYPE_MENU_PANEL;

    if (!container) {
      throw new Error("Expected area " + aArea
                      + " to have a customizationTarget attribute.");
    }

    
    
    if (aArea == CustomizableUI.AREA_NAVBAR) {
      aAreaNode.collapsed = false;
    }

    this.beginBatchUpdate();

    try {
      let currentNode = container.firstChild;
      let placementsToRemove = new Set();
      for (let id of aPlacements) {
        while (currentNode && currentNode.getAttribute("skipintoolbarset") == "true") {
          currentNode = currentNode.nextSibling;
        }

        if (currentNode && currentNode.id == id) {
          currentNode = currentNode.nextSibling;
          continue;
        }

        if (this.isSpecialWidget(id) && areaIsPanel) {
          placementsToRemove.add(id);
          continue;
        }

        let [provider, node] = this.getWidgetNode(id, window);
        if (!node) {
          LOG("Unknown widget: " + id);
          continue;
        }

        
        
        if (provider == CustomizableUI.PROVIDER_API) {
          let widgetInfo = gPalette.get(id);
          if (!widgetInfo.removable && aArea != widgetInfo.defaultArea) {
            placementsToRemove.add(id);
            continue;
          }
        } else if (provider == CustomizableUI.PROVIDER_XUL &&
                   node.parentNode != container && !this.isWidgetRemovable(node)) {
          placementsToRemove.add(id);
          continue;
        } 

        if (inPrivateWindow && provider == CustomizableUI.PROVIDER_API) {
          let widget = gPalette.get(id);
          if (!widget.showInPrivateBrowsing && inPrivateWindow) {
            continue;
          }
        }

        this.ensureButtonContextMenu(node, aAreaNode);
        if (node.localName == "toolbarbutton") {
          if (areaIsPanel) {
            node.setAttribute("wrap", "true");
          } else {
            node.removeAttribute("wrap");
          }
        }

        this.insertWidgetBefore(node, currentNode, container, aArea);
        if (gResetting) {
          this.notifyListeners("onWidgetReset", node, container);
        } else if (gUndoResetting) {
          this.notifyListeners("onWidgetUndoMove", node, container);
        }
      }

      if (currentNode) {
        let palette = aAreaNode.toolbox ? aAreaNode.toolbox.palette : null;
        let limit = currentNode.previousSibling;
        let node = container.lastChild;
        while (node && node != limit) {
          let previousSibling = node.previousSibling;
          
          
          
          
          
          
          
          if (node.id && node.getAttribute("skipintoolbarset") != "true") {
            if (this.isWidgetRemovable(node)) {
              if (palette && !this.isSpecialWidget(node.id)) {
                palette.appendChild(node);
                this.removeLocationAttributes(node);
              } else {
                container.removeChild(node);
              }
            } else {
              node.setAttribute("removable", false);
              LOG("Adding non-removable widget to placements of " + aArea + ": " +
                  node.id);
              gPlacements.get(aArea).push(node.id);
              gDirty = true;
            }
          }
          node = previousSibling;
        }
      }

      
      
      
      if (placementsToRemove.size) {
        let placementAry = gPlacements.get(aArea);
        for (let id of placementsToRemove) {
          let index = placementAry.indexOf(id);
          placementAry.splice(index, 1);
        }
      }

      if (gResetting) {
        this.notifyListeners("onAreaReset", aArea, container);
      }
    } finally {
      this.endBatchUpdate();
    }
  },

  addPanelCloseListeners: function(aPanel) {
    gELS.addSystemEventListener(aPanel, "click", this, false);
    gELS.addSystemEventListener(aPanel, "keypress", this, false);
    let win = aPanel.ownerDocument.defaultView;
    if (!gPanelsForWindow.has(win)) {
      gPanelsForWindow.set(win, new Set());
    }
    gPanelsForWindow.get(win).add(this._getPanelForNode(aPanel));
  },

  removePanelCloseListeners: function(aPanel) {
    gELS.removeSystemEventListener(aPanel, "click", this, false);
    gELS.removeSystemEventListener(aPanel, "keypress", this, false);
    let win = aPanel.ownerDocument.defaultView;
    let panels = gPanelsForWindow.get(win);
    if (panels) {
      panels.delete(this._getPanelForNode(aPanel));
    }
  },

  ensureButtonContextMenu: function(aNode, aAreaNode) {
    const kPanelItemContextMenu = "customizationPanelItemContextMenu";

    let currentContextMenu = aNode.getAttribute("context") ||
                             aNode.getAttribute("contextmenu");
    let place = CustomizableUI.getPlaceForItem(aAreaNode);
    let contextMenuForPlace = place == "panel" ?
                                kPanelItemContextMenu :
                                null;
    if (contextMenuForPlace && !currentContextMenu) {
      aNode.setAttribute("context", contextMenuForPlace);
    } else if (currentContextMenu == kPanelItemContextMenu &&
               contextMenuForPlace != kPanelItemContextMenu) {
      aNode.removeAttribute("context");
      aNode.removeAttribute("contextmenu");
    }
  },

  getWidgetProvider: function(aWidgetId) {
    if (this.isSpecialWidget(aWidgetId)) {
      return CustomizableUI.PROVIDER_SPECIAL;
    }
    if (gPalette.has(aWidgetId)) {
      return CustomizableUI.PROVIDER_API;
    }
    
    if (gSeenWidgets.has(aWidgetId)) {
      return null;
    }

    
    
    
    
    
    return CustomizableUI.PROVIDER_XUL;
  },

  getWidgetNode: function(aWidgetId, aWindow) {
    let document = aWindow.document;

    if (this.isSpecialWidget(aWidgetId)) {
      let widgetNode = document.getElementById(aWidgetId) ||
                       this.createSpecialWidget(aWidgetId, document);
      return [ CustomizableUI.PROVIDER_SPECIAL, widgetNode];
    }

    let widget = gPalette.get(aWidgetId);
    if (widget) {
      
      if (widget.instances.has(document)) {
        LOG("An instance of widget " + aWidgetId + " already exists in this "
            + "document. Reusing.");
        return [ CustomizableUI.PROVIDER_API,
                 widget.instances.get(document) ];
      }

      return [ CustomizableUI.PROVIDER_API,
               this.buildWidget(document, widget) ];
    }

    LOG("Searching for " + aWidgetId + " in toolbox.");
    let node = this.findWidgetInWindow(aWidgetId, aWindow);
    if (node) {
      return [ CustomizableUI.PROVIDER_XUL, node ];
    }

    LOG("No node for " + aWidgetId + " found.");
    return [null, null];
  },

  registerMenuPanel: function(aPanelContents) {
    if (gBuildAreas.has(CustomizableUI.AREA_PANEL) &&
        gBuildAreas.get(CustomizableUI.AREA_PANEL).has(aPanelContents)) {
      return;
    }

    let document = aPanelContents.ownerDocument;

    aPanelContents.toolbox = document.getElementById("navigator-toolbox");
    aPanelContents.customizationTarget = aPanelContents;

    this.addPanelCloseListeners(this._getPanelForNode(aPanelContents));

    let placements = gPlacements.get(CustomizableUI.AREA_PANEL);
    this.buildArea(CustomizableUI.AREA_PANEL, placements, aPanelContents);
    this.notifyListeners("onAreaNodeRegistered", CustomizableUI.AREA_PANEL, aPanelContents);

    for (let child of aPanelContents.children) {
      if (child.localName != "toolbarbutton") {
        if (child.localName == "toolbaritem") {
          this.ensureButtonContextMenu(child, aPanelContents);
        }
        continue;
      }
      this.ensureButtonContextMenu(child, aPanelContents);
      child.setAttribute("wrap", "true");
    }

    this.registerBuildArea(CustomizableUI.AREA_PANEL, aPanelContents);
  },

  onWidgetAdded: function(aWidgetId, aArea, aPosition) {
    this.insertNode(aWidgetId, aArea, aPosition, true);

    if (!gResetting) {
      this._clearPreviousUIState();
    }
  },

  onWidgetRemoved: function(aWidgetId, aArea) {
    let areaNodes = gBuildAreas.get(aArea);
    if (!areaNodes) {
      return;
    }

    let area = gAreas.get(aArea);
    let isToolbar = area.get("type") == CustomizableUI.TYPE_TOOLBAR;
    let isOverflowable = isToolbar && area.get("overflowable");
    let showInPrivateBrowsing = gPalette.has(aWidgetId)
                              ? gPalette.get(aWidgetId).showInPrivateBrowsing
                              : true;

    for (let areaNode of areaNodes) {
      let window = areaNode.ownerDocument.defaultView;
      if (!showInPrivateBrowsing &&
          PrivateBrowsingUtils.isWindowPrivate(window)) {
        continue;
      }

      let container = areaNode.customizationTarget;
      let widgetNode = window.document.getElementById(aWidgetId);
      if (widgetNode && isOverflowable) {
        container = areaNode.overflowable.getContainerFor(widgetNode);
      }

      if (!widgetNode || !container.contains(widgetNode)) {
        INFO("Widget " + aWidgetId + " not found, unable to remove from " + aArea);
        continue;
      }

      this.notifyListeners("onWidgetBeforeDOMChange", widgetNode, null, container, true);

      
      
      this.removeLocationAttributes(widgetNode);
      
      this.ensureButtonContextMenu(widgetNode);
      widgetNode.removeAttribute("wrap");
      if (gPalette.has(aWidgetId) || this.isSpecialWidget(aWidgetId)) {
        container.removeChild(widgetNode);
      } else {
        areaNode.toolbox.palette.appendChild(widgetNode);
      }
      this.notifyListeners("onWidgetAfterDOMChange", widgetNode, null, container, true);

      if (isToolbar) {
        areaNode.setAttribute("currentset", gPlacements.get(aArea).join(','));
      }

      let windowCache = gSingleWrapperCache.get(window);
      if (windowCache) {
        windowCache.delete(aWidgetId);
      }
    }
    if (!gResetting) {
      this._clearPreviousUIState();
    }
  },

  onWidgetMoved: function(aWidgetId, aArea, aOldPosition, aNewPosition) {
    this.insertNode(aWidgetId, aArea, aNewPosition);
    if (!gResetting) {
      this._clearPreviousUIState();
    }
  },

  onCustomizeEnd: function(aWindow) {
    this._clearPreviousUIState();
  },

  registerBuildArea: function(aArea, aNode) {
    
    
    let window = aNode.ownerDocument.defaultView;
    if (window.closed) {
      return;
    }
    this.registerBuildWindow(window);

    
    if (aNode.toolbox) {
      gBuildWindows.get(window).add(aNode.toolbox);
    }

    if (!gBuildAreas.has(aArea)) {
      gBuildAreas.set(aArea, new Set());
    }

    gBuildAreas.get(aArea).add(aNode);

    
    let customizableNode = this.getCustomizeTargetForArea(aArea, window);
    customizableNode.classList.add("customization-target");
  },

  registerBuildWindow: function(aWindow) {
    if (!gBuildWindows.has(aWindow)) {
      gBuildWindows.set(aWindow, new Set());

      aWindow.addEventListener("unload", this);
      aWindow.addEventListener("command", this, true);

      this.notifyListeners("onWindowOpened", aWindow);
    }
  },

  unregisterBuildWindow: function(aWindow) {
    aWindow.removeEventListener("unload", this);
    aWindow.removeEventListener("command", this, true);
    gPanelsForWindow.delete(aWindow);
    gBuildWindows.delete(aWindow);
    gSingleWrapperCache.delete(aWindow);
    let document = aWindow.document;

    for (let [areaId, areaNodes] of gBuildAreas) {
      let areaProperties = gAreas.get(areaId);
      for (let node of areaNodes) {
        if (node.ownerDocument == document) {
          this.notifyListeners("onAreaNodeUnregistered", areaId, node.customizationTarget,
                               CustomizableUI.REASON_WINDOW_CLOSED);
          if (areaProperties.has("overflowable")) {
            node.overflowable.uninit();
            node.overflowable = null;
          }
          areaNodes.delete(node);
        }
      }
    }

    for (let [,widget] of gPalette) {
      widget.instances.delete(document);
      this.notifyListeners("onWidgetInstanceRemoved", widget.id, document);
    }

    for (let [area, areaMap] of gPendingBuildAreas) {
      let toDelete = [];
      for (let [areaNode, ] of areaMap) {
        if (areaNode.ownerDocument == document) {
          toDelete.push(areaNode);
        }
      }
      for (let areaNode of toDelete) {
        areaMap.delete(toDelete);
      }
    }

    this.notifyListeners("onWindowClosed", aWindow);
  },

  setLocationAttributes: function(aNode, aArea) {
    let props = gAreas.get(aArea);
    if (!props) {
      throw new Error("Expected area " + aArea + " to have a properties Map " +
                      "associated with it.");
    }

    aNode.setAttribute("cui-areatype", props.get("type") || "");
    let anchor = props.get("anchor");
    if (anchor) {
      aNode.setAttribute("cui-anchorid", anchor);
    } else {
      aNode.removeAttribute("cui-anchorid");
    }
  },

  removeLocationAttributes: function(aNode) {
    aNode.removeAttribute("cui-areatype");
    aNode.removeAttribute("cui-anchorid");
  },

  insertNode: function(aWidgetId, aArea, aPosition, isNew) {
    let areaNodes = gBuildAreas.get(aArea);
    if (!areaNodes) {
      return;
    }

    let placements = gPlacements.get(aArea);
    if (!placements) {
      ERROR("Could not find any placements for " + aArea +
            " when moving a widget.");
      return;
    }

    
    
    for (let areaNode of areaNodes) {
      this.insertNodeInWindow(aWidgetId, areaNode, isNew);
    }
  },

  insertNodeInWindow: function(aWidgetId, aAreaNode, isNew) {
    let window = aAreaNode.ownerDocument.defaultView;
    let showInPrivateBrowsing = gPalette.has(aWidgetId)
                              ? gPalette.get(aWidgetId).showInPrivateBrowsing
                              : true;

    if (!showInPrivateBrowsing && PrivateBrowsingUtils.isWindowPrivate(window)) {
      return;
    }

    let [, widgetNode] = this.getWidgetNode(aWidgetId, window);
    if (!widgetNode) {
      ERROR("Widget '" + aWidgetId + "' not found, unable to move");
      return;
    }

    let areaId = aAreaNode.id;
    if (isNew) {
      this.ensureButtonContextMenu(widgetNode, aAreaNode);
      if (widgetNode.localName == "toolbarbutton" && areaId == CustomizableUI.AREA_PANEL) {
        widgetNode.setAttribute("wrap", "true");
      }
    }

    let [insertionContainer, nextNode] = this.findInsertionPoints(widgetNode, aAreaNode);
    this.insertWidgetBefore(widgetNode, nextNode, insertionContainer, areaId);

    if (gAreas.get(areaId).get("type") == CustomizableUI.TYPE_TOOLBAR) {
      aAreaNode.setAttribute("currentset", gPlacements.get(areaId).join(','));
    }
  },

  findInsertionPoints: function(aNode, aAreaNode) {
    let areaId = aAreaNode.id;
    let props = gAreas.get(areaId);

    
    if (props.get("type") == CustomizableUI.TYPE_TOOLBAR && props.get("overflowable")) {
      return aAreaNode.overflowable.findOverflowedInsertionPoints(aNode);
    }

    let container = aAreaNode.customizationTarget;
    let placements = gPlacements.get(areaId);
    let nodeIndex = placements.indexOf(aNode.id);

    while (++nodeIndex < placements.length) {
      let nextNodeId = placements[nodeIndex];
      let nextNode = container.getElementsByAttribute("id", nextNodeId).item(0);

      if (nextNode) {
        return [container, nextNode];
      }
    }

    return [container, null];
  },

  insertWidgetBefore: function(aNode, aNextNode, aContainer, aArea) {
    this.notifyListeners("onWidgetBeforeDOMChange", aNode, aNextNode, aContainer);
    this.setLocationAttributes(aNode, aArea);
    aContainer.insertBefore(aNode, aNextNode);
    this.notifyListeners("onWidgetAfterDOMChange", aNode, aNextNode, aContainer);
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "command":
        if (!this._originalEventInPanel(aEvent)) {
          break;
        }
        aEvent = aEvent.sourceEvent;
        
      case "click":
      case "keypress":
        this.maybeAutoHidePanel(aEvent);
        break;
      case "unload":
        this.unregisterBuildWindow(aEvent.currentTarget);
        break;
    }
  },

  _originalEventInPanel: function(aEvent) {
    let e = aEvent.sourceEvent;
    if (!e) {
      return false;
    }
    let node = this._getPanelForNode(e.target);
    if (!node) {
      return false;
    }
    let win = e.view;
    let panels = gPanelsForWindow.get(win);
    return !!panels && panels.has(node);
  },

  isSpecialWidget: function(aId) {
    return (aId.startsWith(kSpecialWidgetPfx) ||
            aId.startsWith("separator") ||
            aId.startsWith("spring") ||
            aId.startsWith("spacer"));
  },

  ensureSpecialWidgetId: function(aId) {
    let nodeType = aId.match(/spring|spacer|separator/)[0];
    
    if (nodeType == aId) {
      
      return kSpecialWidgetPfx + aId + (++gNewElementCount);
    }
    return aId;
  },

  createSpecialWidget: function(aId, aDocument) {
    let nodeName = "toolbar" + aId.match(/spring|spacer|separator/)[0];
    let node = aDocument.createElementNS(kNSXUL, nodeName);
    node.id = this.ensureSpecialWidgetId(aId);
    if (nodeName == "toolbarspring") {
      node.flex = 1;
    }
    return node;
  },

  


  findWidgetInWindow: function(aId, aWindow) {
    if (!gBuildWindows.has(aWindow)) {
      throw new Error("Build window not registered");
    }

    if (!aId) {
      ERROR("findWidgetInWindow was passed an empty string.");
      return null;
    }

    let document = aWindow.document;

    
    
    let node = document.getElementById(aId);
    if (node) {
      let parent = node.parentNode;
      while (parent && !(parent.customizationTarget ||
                         parent == aWindow.gNavToolbox.palette)) {
        parent = parent.parentNode;
      }

      if (parent) {
        let nodeInArea = node.parentNode.localName == "toolbarpaletteitem" ?
                         node.parentNode : node;
        
        if ((parent.customizationTarget == nodeInArea.parentNode &&
             gBuildWindows.get(aWindow).has(parent.toolbox)) ||
            aWindow.gNavToolbox.palette == nodeInArea.parentNode) {
          
          
          
          if (!node.hasAttribute("removable")) {
            
            
            node.setAttribute("removable", !parent.customizationTarget);
          }
          return node;
        }
      }
    }

    let toolboxes = gBuildWindows.get(aWindow);
    for (let toolbox of toolboxes) {
      if (toolbox.palette) {
        
        
        let node = toolbox.palette.getElementsByAttribute("id", aId)[0];
        if (node) {
          
          
          
          if (!node.hasAttribute("removable")) {
            node.setAttribute("removable", true);
          }
          return node;
        }
      }
    }
    return null;
  },

  buildWidget: function(aDocument, aWidget) {
    if (typeof aWidget == "string") {
      aWidget = gPalette.get(aWidget);
    }
    if (!aWidget) {
      throw new Error("buildWidget was passed a non-widget to build.");
    }

    LOG("Building " + aWidget.id + " of type " + aWidget.type);

    let node;
    if (aWidget.type == "custom") {
      if (aWidget.onBuild) {
        node = aWidget.onBuild(aDocument);
      }
      if (!node || !(node instanceof aDocument.defaultView.XULElement))
        ERROR("Custom widget with id " + aWidget.id + " does not return a valid node");
    }
    else {
      if (aWidget.onBeforeCreated) {
        aWidget.onBeforeCreated(aDocument);
      }
      node = aDocument.createElementNS(kNSXUL, "toolbarbutton");

      node.setAttribute("id", aWidget.id);
      node.setAttribute("widget-id", aWidget.id);
      node.setAttribute("widget-type", aWidget.type);
      if (aWidget.disabled) {
        node.setAttribute("disabled", true);
      }
      node.setAttribute("removable", aWidget.removable);
      node.setAttribute("overflows", aWidget.overflows);
      node.setAttribute("label", this.getLocalizedProperty(aWidget, "label"));
      let additionalTooltipArguments = [];
      if (aWidget.shortcutId) {
        let keyEl = aDocument.getElementById(aWidget.shortcutId);
        if (keyEl) {
          additionalTooltipArguments.push(ShortcutUtils.prettifyShortcut(keyEl));
        } else {
          ERROR("Key element with id '" + aWidget.shortcutId + "' for widget '" + aWidget.id +
                "' not found!");
        }
      }

      let tooltip = this.getLocalizedProperty(aWidget, "tooltiptext", additionalTooltipArguments);
      node.setAttribute("tooltiptext", tooltip);
      node.setAttribute("class", "toolbarbutton-1 chromeclass-toolbar-additional");

      let commandHandler = this.handleWidgetCommand.bind(this, aWidget, node);
      node.addEventListener("command", commandHandler, false);
      let clickHandler = this.handleWidgetClick.bind(this, aWidget, node);
      node.addEventListener("click", clickHandler, false);

      
      
      if (aWidget.type == "view") {
        LOG("Widget " + aWidget.id + " has a view. Auto-registering event handlers.");
        let viewNode = aDocument.getElementById(aWidget.viewId);

        if (viewNode) {
          
          
          viewNode.classList.add("PanelUI-subView");

          for (let eventName of kSubviewEvents) {
            let handler = "on" + eventName;
            if (typeof aWidget[handler] == "function") {
              viewNode.addEventListener(eventName, aWidget[handler], false);
            }
          }

          LOG("Widget " + aWidget.id + " showing and hiding event handlers set.");
        } else {
          ERROR("Could not find the view node with id: " + aWidget.viewId +
                ", for widget: " + aWidget.id + ".");
        }
      }

      if (aWidget.onCreated) {
        aWidget.onCreated(node);
      }
    }

    aWidget.instances.set(aDocument, node);
    return node;
  },

  getLocalizedProperty: function(aWidget, aProp, aFormatArgs, aDef) {
    if (typeof aWidget == "string") {
      aWidget = gPalette.get(aWidget);
    }
    if (!aWidget) {
      throw new Error("getLocalizedProperty was passed a non-widget to work with.");
    }
    let def, name;
    
    
    
    if (aWidget[aProp]) {
      name = aWidget[aProp];
      
      
      
      def = aDef || name;
    } else {
      name = aWidget.id + "." + aProp;
      def = aDef || "";
    }
    try {
      if (Array.isArray(aFormatArgs) && aFormatArgs.length) {
        return gWidgetsBundle.formatStringFromName(name, aFormatArgs,
          aFormatArgs.length) || def;
      }
      return gWidgetsBundle.GetStringFromName(name) || def;
    } catch(ex) {
      if (!def) {
        ERROR("Could not localize property '" + name + "'.");
      }
    }
    return def;
  },

  addShortcut: function(aShortcutNode, aTargetNode) {
    if (!aTargetNode)
      aTargetNode = aShortcutNode;
    let document = aShortcutNode.ownerDocument;

    
    if (!aTargetNode || aTargetNode.hasAttribute("shortcut"))
      return;

    let shortcutId = aShortcutNode.getAttribute("key");
    let shortcut;
    if (shortcutId) {
      shortcut = document.getElementById(shortcutId);
    } else {
      let commandId = aShortcutNode.getAttribute("command");
      if (commandId)
        shortcut = ShortcutUtils.findShortcut(document.getElementById(commandId));
    }
    if (!shortcut) {
      return;
    }

    aTargetNode.setAttribute("shortcut", ShortcutUtils.prettifyShortcut(shortcut));
  },

  handleWidgetCommand: function(aWidget, aNode, aEvent) {
    LOG("handleWidgetCommand");

    if (aWidget.type == "button") {
      if (aWidget.onCommand) {
        try {
          aWidget.onCommand.call(null, aEvent);
        } catch (e) {
          ERROR(e);
        }
      } else {
        
        Services.obs.notifyObservers(aNode,
                                     "customizedui-widget-command",
                                     aWidget.id);
      }
    } else if (aWidget.type == "view") {
      let ownerWindow = aNode.ownerDocument.defaultView;
      let area = this.getPlacementOfWidget(aNode.id).area;
      let anchor = aNode;
      if (area != CustomizableUI.AREA_PANEL) {
        let wrapper = this.wrapWidget(aWidget.id).forWindow(ownerWindow);
        if (wrapper && wrapper.anchor) {
          this.hidePanelForNode(aNode);
          anchor = wrapper.anchor;
        }
      }
      ownerWindow.PanelUI.showSubView(aWidget.viewId, anchor, area);
    }
  },

  handleWidgetClick: function(aWidget, aNode, aEvent) {
    LOG("handleWidgetClick");
    if (aWidget.onClick) {
      try {
        aWidget.onClick.call(null, aEvent);
      } catch(e) {
        Cu.reportError(e);
      }
    } else {
      
      Services.obs.notifyObservers(aNode, "customizedui-widget-click", aWidget.id);
    }
  },

  _getPanelForNode: function(aNode) {
    let panel = aNode;
    while (panel && panel.localName != "panel")
      panel = panel.parentNode;
    return panel;
  },

  





  _isOnInteractiveElement: function(aEvent) {
    function getMenuPopupForDescendant(aNode) {
      let lastPopup = null;
      while (aNode && aNode.parentNode &&
             aNode.parentNode.localName.startsWith("menu")) {
        lastPopup = aNode.localName == "menupopup" ? aNode : lastPopup;
        aNode = aNode.parentNode;
      }
      return lastPopup;
    }

    let target = aEvent.originalTarget;
    let panel = this._getPanelForNode(aEvent.currentTarget);
    
    
    if (!panel) {
      return true;
    }
    
    
    let inInput = false;
    
    let inMenu = false;
    
    let inItem = false;
    
    let menuitemCloseMenu = "auto";
    
    let closemenu = "auto";

    
    
    
    while (true && target) {
      let tagName = target.localName;
      inInput = tagName == "input" || tagName == "textbox";
      inItem = tagName == "toolbaritem" || tagName == "toolbarbutton";
      let isMenuItem = tagName == "menuitem";
      inMenu = inMenu || isMenuItem;
      if (inItem && target.hasAttribute("closemenu")) {
        let closemenuVal = target.getAttribute("closemenu");
        closemenu = (closemenuVal == "single" || closemenuVal == "none") ?
                    closemenuVal : "auto";
      }

      if (isMenuItem && target.hasAttribute("closemenu")) {
        let closemenuVal = target.getAttribute("closemenu");
        menuitemCloseMenu = (closemenuVal == "single" || closemenuVal == "none") ?
                            closemenuVal : "auto";
      }
      
      
      if (target.getAttribute("disabled") == "true") {
        return true;
      }

      
      
      if (inInput || inItem || target == panel) {
        break;
      }
      
      
      if (isMenuItem) {
        let topmostMenuPopup = getMenuPopupForDescendant(target);
        target = (topmostMenuPopup && topmostMenuPopup.triggerNode) ||
                 target.parentNode;
      } else {
        target = target.parentNode;
      }
    }

    
    if (inMenu) {
      
      
      if (inInput || menuitemCloseMenu != "auto") {
        return true;
      }
      
      return false;
    }
    
    
    if (inItem && target.getAttribute("type") == "menu") {
      return true;
    }
    
    
    if (inItem && target.getAttribute("type") == "menu-button") {
      
      if (target.getAttribute("anonid") == "button") {
        return closemenu != "none";
      }
      
      
      return true;
    }
    return inInput || !inItem;
  },

  hidePanelForNode: function(aNode) {
    let panel = this._getPanelForNode(aNode);
    if (panel) {
      panel.hidePopup();
    }
  },

  maybeAutoHidePanel: function(aEvent) {
    if (aEvent.type == "keypress") {
      if (aEvent.keyCode != aEvent.DOM_VK_RETURN) {
        return;
      }
      
      
      
      

    } else if (aEvent.type != "command") { 
      if (aEvent.defaultPrevented || aEvent.button != 0) {
        return;
      }
      let isInteractive = this._isOnInteractiveElement(aEvent);
      LOG("maybeAutoHidePanel: interactive ? " + isInteractive);
      if (isInteractive) {
        return;
      }
    }

    
    
    
    
    
    let target = aEvent.originalTarget;
    let closemenu = "auto";
    let widgetType = "button";
    while (target.parentNode && target.localName != "panel") {
      closemenu = target.getAttribute("closemenu");
      widgetType = target.getAttribute("widget-type");
      if (closemenu == "none" || closemenu == "single" ||
          widgetType == "view") {
        break;
      }
      target = target.parentNode;
    }
    if (closemenu == "none" || widgetType == "view") {
      return;
    }

    if (closemenu == "single") {
      let panel = this._getPanelForNode(target);
      let multiview = panel.querySelector("panelmultiview");
      if (multiview.showingSubView) {
        multiview.showMainView();
        return;
      }
    }

    
    this.hidePanelForNode(aEvent.target);
  },

  getUnusedWidgets: function(aWindowPalette) {
    let window = aWindowPalette.ownerDocument.defaultView;
    let isWindowPrivate = PrivateBrowsingUtils.isWindowPrivate(window);
    
    
    
    
    let widgets = new Set();

    
    
    
    for (let [id, widget] of gPalette) {
      if (!widget.currentArea) {
        if (widget.showInPrivateBrowsing || !isWindowPrivate) {
          widgets.add(id);
        }
      }
    }

    LOG("Iterating the actual nodes of the window palette");
    for (let node of aWindowPalette.children) {
      LOG("In palette children: " + node.id);
      if (node.id && !this.getPlacementOfWidget(node.id)) {
        widgets.add(node.id);
      }
    }

    return [...widgets];
  },

  getPlacementOfWidget: function(aWidgetId, aOnlyRegistered, aDeadAreas) {
    if (aOnlyRegistered && !this.widgetExists(aWidgetId)) {
      return null;
    }

    for (let [area, placements] of gPlacements) {
      if (!gAreas.has(area) && !aDeadAreas) {
        continue;
      }
      let index = placements.indexOf(aWidgetId);
      if (index != -1) {
        return { area: area, position: index };
      }
    }

    return null;
  },

  widgetExists: function(aWidgetId) {
    if (gPalette.has(aWidgetId) || this.isSpecialWidget(aWidgetId)) {
      return true;
    }

    
    if (gSeenWidgets.has(aWidgetId)) {
      return false;
    }

    
    
    return true;
  },

  addWidgetToArea: function(aWidgetId, aArea, aPosition, aInitialAdd) {
    if (!gAreas.has(aArea)) {
      throw new Error("Unknown customization area: " + aArea);
    }

    
    
    if (gAreas.get(aArea).get("type") == CustomizableUI.TYPE_MENU_PANEL &&
        this.isSpecialWidget(aWidgetId)) {
      return;
    }

    
    
    
    
    
    
    if (this.isAreaLazy(aArea)) {
      gFuturePlacements.get(aArea).add(aWidgetId);
      return;
    }

    if (this.isSpecialWidget(aWidgetId)) {
      aWidgetId = this.ensureSpecialWidgetId(aWidgetId);
    }

    let oldPlacement = this.getPlacementOfWidget(aWidgetId, false, true);
    if (oldPlacement && oldPlacement.area == aArea) {
      this.moveWidgetWithinArea(aWidgetId, aPosition);
      return;
    }

    
    if (!this.canWidgetMoveToArea(aWidgetId, aArea)) {
      return;
    }

    if (oldPlacement) {
      this.removeWidgetFromArea(aWidgetId);
    }

    if (!gPlacements.has(aArea)) {
      gPlacements.set(aArea, [aWidgetId]);
      aPosition = 0;
    } else {
      let placements = gPlacements.get(aArea);
      if (typeof aPosition != "number") {
        aPosition = placements.length;
      }
      if (aPosition < 0) {
        aPosition = 0;
      }
      placements.splice(aPosition, 0, aWidgetId);
    }

    let widget = gPalette.get(aWidgetId);
    if (widget) {
      widget.currentArea = aArea;
      widget.currentPosition = aPosition;
    }

    
    
    if (!aInitialAdd) {
      gDirtyAreaCache.add(aArea);
    }

    gDirty = true;
    this.saveState();

    this.notifyListeners("onWidgetAdded", aWidgetId, aArea, aPosition);
  },

  removeWidgetFromArea: function(aWidgetId) {
    let oldPlacement = this.getPlacementOfWidget(aWidgetId, false, true);
    if (!oldPlacement) {
      return;
    }

    if (!this.isWidgetRemovable(aWidgetId)) {
      return;
    }

    let placements = gPlacements.get(oldPlacement.area);
    let position = placements.indexOf(aWidgetId);
    if (position != -1) {
      placements.splice(position, 1);
    }

    let widget = gPalette.get(aWidgetId);
    if (widget) {
      widget.currentArea = null;
      widget.currentPosition = null;
    }

    gDirty = true;
    this.saveState();
    gDirtyAreaCache.add(oldPlacement.area);

    this.notifyListeners("onWidgetRemoved", aWidgetId, oldPlacement.area);
  },

  moveWidgetWithinArea: function(aWidgetId, aPosition) {
    let oldPlacement = this.getPlacementOfWidget(aWidgetId);
    if (!oldPlacement) {
      return;
    }

    let placements = gPlacements.get(oldPlacement.area);
    if (typeof aPosition != "number") {
      aPosition = placements.length;
    } else if (aPosition < 0) {
      aPosition = 0;
    } else if (aPosition > placements.length) {
      aPosition = placements.length;
    }

    let widget = gPalette.get(aWidgetId);
    if (widget) {
      widget.currentPosition = aPosition;
      widget.currentArea = oldPlacement.area;
    }

    if (aPosition == oldPlacement.position) {
      return;
    }

    placements.splice(oldPlacement.position, 1);
    
    
    if (oldPlacement.position < aPosition) {
      aPosition--;
    }
    placements.splice(aPosition, 0, aWidgetId);

    gDirty = true;
    gDirtyAreaCache.add(oldPlacement.area);

    this.saveState();

    this.notifyListeners("onWidgetMoved", aWidgetId, oldPlacement.area,
                         oldPlacement.position, aPosition);
  },

  
  
  
  
  
  
  
  loadSavedState: function() {
    let state = null;
    try {
      state = Services.prefs.getCharPref(kPrefCustomizationState);
    } catch (e) {
      LOG("No saved state found");
      
      
    }

    if (!state) {
      return;
    }
    try {
      gSavedState = JSON.parse(state);
      if (typeof gSavedState != "object" || gSavedState === null) {
        throw "Invalid saved state";
      }
    } catch(e) {
      Services.prefs.clearUserPref(kPrefCustomizationState);
      gSavedState = {};
      LOG("Error loading saved UI customization state, falling back to defaults.");
    }

    if (!("placements" in gSavedState)) {
      gSavedState.placements = {};
    }

    if (!("currentVersion" in gSavedState)) {
      gSavedState.currentVersion = 0;
    }

    gSeenWidgets = new Set(gSavedState.seen || []);
    gDirtyAreaCache = new Set(gSavedState.dirtyAreaCache || []);
    gNewElementCount = gSavedState.newElementCount || 0;
  },

  restoreStateForArea: function(aArea, aLegacyState) {
    let placementsPreexisted = gPlacements.has(aArea);

    this.beginBatchUpdate();
    try {
      gRestoring = true;

      let restored = false;
      if (placementsPreexisted) {
        LOG("Restoring " + aArea + " from pre-existing placements");
        for (let [position, id] in Iterator(gPlacements.get(aArea))) {
          this.moveWidgetWithinArea(id, position);
        }
        gDirty = false;
        restored = true;
      } else {
        gPlacements.set(aArea, []);
      }

      if (!restored && gSavedState && aArea in gSavedState.placements) {
        LOG("Restoring " + aArea + " from saved state");
        let placements = gSavedState.placements[aArea];
        for (let id of placements)
          this.addWidgetToArea(id, aArea);
        gDirty = false;
        restored = true;
      }

      if (!restored && aLegacyState) {
        LOG("Restoring " + aArea + " from legacy state");
        for (let id of aLegacyState)
          this.addWidgetToArea(id, aArea);
        
        
        restored = true;
      }

      if (!restored) {
        LOG("Restoring " + aArea + " from default state");
        let defaults = gAreas.get(aArea).get("defaultPlacements");
        if (defaults) {
          for (let id of defaults)
            this.addWidgetToArea(id, aArea, null, true);
        }
        gDirty = false;
      }

      
      
      
      if (gFuturePlacements.has(aArea)) {
        for (let id of gFuturePlacements.get(aArea))
          this.addWidgetToArea(id, aArea);
        gFuturePlacements.delete(aArea);
      }

      LOG("Placements for " + aArea + ":\n\t" + gPlacements.get(aArea).join("\n\t"));

      gRestoring = false;
    } finally {
      this.endBatchUpdate();
    }
  },

  saveState: function() {
    if (gInBatchStack || !gDirty) {
      return;
    }
    
    let state = { placements: new Map(gPlacements),
                  seen: gSeenWidgets,
                  dirtyAreaCache: gDirtyAreaCache,
                  currentVersion: kVersion,
                  newElementCount: gNewElementCount };

    
    
    
    if (gSavedState && gSavedState.placements) {
      for (let area of Object.keys(gSavedState.placements)) {
        if (!state.placements.has(area)) {
          let placements = gSavedState.placements[area];
          state.placements.set(area, placements);
        }
      }
    }

    LOG("Saving state.");
    let serialized = JSON.stringify(state, this.serializerHelper);
    LOG("State saved as: " + serialized);
    Services.prefs.setCharPref(kPrefCustomizationState, serialized);
    gDirty = false;
  },

  serializerHelper: function(aKey, aValue) {
    if (typeof aValue == "object" && aValue.constructor.name == "Map") {
      let result = {};
      for (let [mapKey, mapValue] of aValue)
        result[mapKey] = mapValue;
      return result;
    }

    if (typeof aValue == "object" && aValue.constructor.name == "Set") {
      return [...aValue];
    }

    return aValue;
  },

  beginBatchUpdate: function() {
    gInBatchStack++;
  },

  endBatchUpdate: function(aForceDirty) {
    gInBatchStack--;
    if (aForceDirty === true) {
      gDirty = true;
    }
    if (gInBatchStack == 0) {
      this.saveState();
    } else if (gInBatchStack < 0) {
      throw new Error("The batch editing stack should never reach a negative number.");
    }
  },

  addListener: function(aListener) {
    gListeners.add(aListener);
  },

  removeListener: function(aListener) {
    if (aListener == this) {
      return;
    }

    gListeners.delete(aListener);
  },

  notifyListeners: function(aEvent, ...aArgs) {
    if (gRestoring) {
      return;
    }

    for (let listener of gListeners) {
      try {
        if (typeof listener[aEvent] == "function") {
          listener[aEvent].apply(listener, aArgs);
        }
      } catch (e) {
        ERROR(e + " -- " + e.fileName + ":" + e.lineNumber);
      }
    }
  },

  _dispatchToolboxEventToWindow: function(aEventType, aDetails, aWindow) {
    let evt = new aWindow.CustomEvent(aEventType, {
      bubbles: true,
      cancelable: true,
      detail: aDetails
    });
    aWindow.gNavToolbox.dispatchEvent(evt);
  },

  dispatchToolboxEvent: function(aEventType, aDetails={}, aWindow=null) {
    if (aWindow) {
      return this._dispatchToolboxEventToWindow(aEventType, aDetails, aWindow);
    }
    for (let [win, ] of gBuildWindows) {
      this._dispatchToolboxEventToWindow(aEventType, aDetails, win);
    }
  },

  createWidget: function(aProperties) {
    let widget = this.normalizeWidget(aProperties, CustomizableUI.SOURCE_EXTERNAL);
    
    if (!widget) {
      return;
    }

    gPalette.set(widget.id, widget);

    
    gGroupWrapperCache.delete(widget.id);
    for (let [win, ] of gBuildWindows) {
      let cache = gSingleWrapperCache.get(win);
      if (cache) {
        cache.delete(widget.id);
      }
    }

    this.notifyListeners("onWidgetCreated", widget.id);

    if (widget.defaultArea) {
      let addToDefaultPlacements = false;
      let area = gAreas.get(widget.defaultArea);
      if (!CustomizableUI.isBuiltinToolbar(widget.defaultArea) &&
          widget.defaultArea != CustomizableUI.AREA_PANEL) {
        addToDefaultPlacements = true;
      }

      if (addToDefaultPlacements) {
        if (area.has("defaultPlacements")) {
          area.get("defaultPlacements").push(widget.id);
        } else {
          area.set("defaultPlacements", [widget.id]);
        }
      }
    }

    
    let seenAreas = new Set();
    let widgetMightNeedAutoAdding = true;
    for (let [area, placements] of gPlacements) {
      seenAreas.add(area);
      let areaIsRegistered = gAreas.has(area);
      let index = gPlacements.get(area).indexOf(widget.id);
      if (index != -1) {
        widgetMightNeedAutoAdding = false;
        if (areaIsRegistered) {
          widget.currentArea = area;
          widget.currentPosition = index;
        }
        break;
      }
    }

    
    
    
    if (widgetMightNeedAutoAdding && gSavedState) {
      for (let area of Object.keys(gSavedState.placements)) {
        if (seenAreas.has(area)) {
          continue;
        }

        let areaIsRegistered = gAreas.has(area);
        let index = gSavedState.placements[area].indexOf(widget.id);
        if (index != -1) {
          widgetMightNeedAutoAdding = false;
          if (areaIsRegistered) {
            widget.currentArea = area;
            widget.currentPosition = index;
          }
          break;
        }
      }
    }

    
    
    
    this.beginBatchUpdate();
    try {
      if (widget.currentArea) {
        this.notifyListeners("onWidgetAdded", widget.id, widget.currentArea,
                             widget.currentPosition);
      } else if (widgetMightNeedAutoAdding) {
        let autoAdd = true;
        try {
          autoAdd = Services.prefs.getBoolPref(kPrefCustomizationAutoAdd);
        } catch (e) {}

        
        
        
        
        let canBeAutoAdded = autoAdd && !gSeenWidgets.has(widget.id);
        if (!widget.currentArea && (!widget.removable || canBeAutoAdded)) {
          if (widget.defaultArea) {
            if (this.isAreaLazy(widget.defaultArea)) {
              gFuturePlacements.get(widget.defaultArea).add(widget.id);
            } else {
              this.addWidgetToArea(widget.id, widget.defaultArea);
            }
          }
        }
      }
    } finally {
      
      
      gSeenWidgets.add(widget.id);
      this.endBatchUpdate(true);
    }

    this.notifyListeners("onWidgetAfterCreation", widget.id, widget.currentArea);
    return widget.id;
  },

  createBuiltinWidget: function(aData) {
    
    
    
    
    let widget = this.normalizeWidget(aData, CustomizableUI.SOURCE_BUILTIN);
    if (!widget) {
      ERROR("Error creating builtin widget: " + aData.id);
      return;
    }

    LOG("Creating built-in widget with id: " + widget.id);
    gPalette.set(widget.id, widget);
  },

  
  isAreaLazy: function(aArea) {
    if (gPlacements.has(aArea)) {
      return false;
    }
    return gAreas.get(aArea).has("legacy");
  },

  
  normalizeWidget: function(aData, aSource) {
    let widget = {
      implementation: aData,
      source: aSource || CustomizableUI.SOURCE_EXTERNAL,
      instances: new Map(),
      currentArea: null,
      removable: true,
      overflows: true,
      defaultArea: null,
      shortcutId: null,
      tooltiptext: null,
      showInPrivateBrowsing: true,
      _introducedInVersion: -1,
    };

    if (typeof aData.id != "string" || !/^[a-z0-9-_]{1,}$/i.test(aData.id)) {
      ERROR("Given an illegal id in normalizeWidget: " + aData.id);
      return null;
    }

    delete widget.implementation.currentArea;
    widget.implementation.__defineGetter__("currentArea", function() widget.currentArea);

    const kReqStringProps = ["id"];
    for (let prop of kReqStringProps) {
      if (typeof aData[prop] != "string") {
        ERROR("Missing required property '" + prop + "' in normalizeWidget: "
              + aData.id);
        return null;
      }
      widget[prop] = aData[prop];
    }

    const kOptStringProps = ["label", "tooltiptext", "shortcutId"];
    for (let prop of kOptStringProps) {
      if (typeof aData[prop] == "string") {
        widget[prop] = aData[prop];
      }
    }

    const kOptBoolProps = ["removable", "showInPrivateBrowsing", "overflows"];
    for (let prop of kOptBoolProps) {
      if (typeof aData[prop] == "boolean") {
        widget[prop] = aData[prop];
      }
    }

    
    if (aData.defaultArea &&
        (aSource == CustomizableUI.SOURCE_BUILTIN || gAreas.has(aData.defaultArea))) {
      widget.defaultArea = aData.defaultArea;
    } else if (!widget.removable) {
      ERROR("Widget '" + widget.id + "' is not removable but does not specify " +
            "a valid defaultArea. That's not possible; it must specify a " +
            "valid defaultArea as well.");
      return null;
    }

    if ("type" in aData && gSupportedWidgetTypes.has(aData.type)) {
      widget.type = aData.type;
    } else {
      widget.type = "button";
    }

    widget.disabled = aData.disabled === true;

    if (aSource == CustomizableUI.SOURCE_BUILTIN) {
      widget._introducedInVersion = aData.introducedInVersion || 0;
    }

    this.wrapWidgetEventHandler("onBeforeCreated", widget);
    this.wrapWidgetEventHandler("onClick", widget);
    this.wrapWidgetEventHandler("onCreated", widget);

    if (widget.type == "button") {
      widget.onCommand = typeof aData.onCommand == "function" ?
                           aData.onCommand :
                           null;
    } else if (widget.type == "view") {
      if (typeof aData.viewId != "string") {
        ERROR("Expected a string for widget " + widget.id + " viewId, but got "
              + aData.viewId);
        return null;
      }
      widget.viewId = aData.viewId;

      this.wrapWidgetEventHandler("onViewShowing", widget);
      this.wrapWidgetEventHandler("onViewHiding", widget);
    } else if (widget.type == "custom") {
      this.wrapWidgetEventHandler("onBuild", widget);
    }

    if (gPalette.has(widget.id)) {
      return null;
    }

    return widget;
  },

  wrapWidgetEventHandler: function(aEventName, aWidget) {
    if (typeof aWidget.implementation[aEventName] != "function") {
      aWidget[aEventName] = null;
      return;
    }
    aWidget[aEventName] = function(...aArgs) {
      
      
      try {
        
        
        
        
        return aWidget.implementation[aEventName].apply(aWidget.implementation,
                                                        aArgs);
      } catch (e) {
        Cu.reportError(e);
      }
    };
  },

  destroyWidget: function(aWidgetId) {
    let widget = gPalette.get(aWidgetId);
    if (!widget) {
      gGroupWrapperCache.delete(aWidgetId);
      for (let [window, ] of gBuildWindows) {
        let windowCache = gSingleWrapperCache.get(window);
        if (windowCache) {
          windowCache.delete(aWidgetId);
        }
      }
      return;
    }

    
    if (widget.defaultArea) {
      let area = gAreas.get(widget.defaultArea);
      if (area) {
        let defaultPlacements = area.get("defaultPlacements");
        
        
        let widgetIndex = defaultPlacements.indexOf(aWidgetId);
        if (widgetIndex != -1) {
          defaultPlacements.splice(widgetIndex, 1);
        }
      }
    }

    
    
    
    for (let [window, ] of gBuildWindows) {
      let windowCache = gSingleWrapperCache.get(window);
      if (windowCache) {
        windowCache.delete(aWidgetId);
      }
      let widgetNode = window.document.getElementById(aWidgetId) ||
                       window.gNavToolbox.palette.getElementsByAttribute("id", aWidgetId)[0];
      if (widgetNode) {
        let container = widgetNode.parentNode
        this.notifyListeners("onWidgetBeforeDOMChange", widgetNode, null,
                             container, true);
        widgetNode.remove();
        this.notifyListeners("onWidgetAfterDOMChange", widgetNode, null,
                             container, true);
      }
      if (widget.type == "view") {
        let viewNode = window.document.getElementById(widget.viewId);
        if (viewNode) {
          for (let eventName of kSubviewEvents) {
            let handler = "on" + eventName;
            if (typeof widget[handler] == "function") {
              viewNode.removeEventListener(eventName, widget[handler], false);
            }
          }
        }
      }
    }

    gPalette.delete(aWidgetId);
    gGroupWrapperCache.delete(aWidgetId);

    this.notifyListeners("onWidgetDestroyed", aWidgetId);
  },

  getCustomizeTargetForArea: function(aArea, aWindow) {
    let buildAreaNodes = gBuildAreas.get(aArea);
    if (!buildAreaNodes) {
      return null;
    }

    for (let node of buildAreaNodes) {
      if (node.ownerDocument.defaultView === aWindow) {
        return node.customizationTarget ? node.customizationTarget : node;
      }
    }

    return null;
  },

  reset: function() {
    gResetting = true;
    this._resetUIState();

    
    
    this._rebuildRegisteredAreas();

    for (let [widgetId, widget] of gPalette) {
      if (widget.source == CustomizableUI.SOURCE_EXTERNAL) {
        gSeenWidgets.add(widgetId);
      }
    }
    if (gSeenWidgets.size) {
      gDirty = true;
    }

    gResetting = false;
  },

  _resetUIState: function() {
    try {
      gUIStateBeforeReset.drawInTitlebar = Services.prefs.getBoolPref(kPrefDrawInTitlebar);
      gUIStateBeforeReset.uiCustomizationState = Services.prefs.getCharPref(kPrefCustomizationState);
    } catch(e) { }

    this._resetExtraToolbars();

    Services.prefs.clearUserPref(kPrefCustomizationState);
    Services.prefs.clearUserPref(kPrefDrawInTitlebar);
    LOG("State reset");

    
    gPlacements = new Map();
    gDirtyAreaCache = new Set();
    gSeenWidgets = new Set();
    
    gSavedState = null;
    
    for (let [areaId,] of gAreas) {
      this.restoreStateForArea(areaId);
    }
  },

  _resetExtraToolbars: function(aFilter = null) {
    let firstWindow = true; 
    for (let [win, ] of gBuildWindows) {
      let toolbox = win.gNavToolbox;
      for (let child of toolbox.children) {
        let matchesFilter = !aFilter || aFilter == child.id;
        if (child.hasAttribute("customindex") && matchesFilter) {
          let toolbarId = "toolbar" + child.getAttribute("customindex");
          toolbox.toolbarset.removeAttribute(toolbarId);
          if (firstWindow) {
            win.document.persist(toolbox.toolbarset.id, toolbarId);
            
            
            this.unregisterArea(child.id, true);
          }
          child.remove();
        }
      }
      firstWindow = false;
    }
  },

  _rebuildRegisteredAreas: function() {
    for (let [areaId, areaNodes] of gBuildAreas) {
      let placements = gPlacements.get(areaId);
      let isFirstChangedToolbar = true;
      for (let areaNode of areaNodes) {
        this.buildArea(areaId, placements, areaNode);

        let area = gAreas.get(areaId);
        if (area.get("type") == CustomizableUI.TYPE_TOOLBAR) {
          let defaultCollapsed = area.get("defaultCollapsed");
          let win = areaNode.ownerDocument.defaultView;
          if (defaultCollapsed !== null) {
            win.setToolbarVisibility(areaNode, !defaultCollapsed, isFirstChangedToolbar);
          }
        }
        isFirstChangedToolbar = false;
      }
    }
  },

  


  undoReset: function() {
    if (gUIStateBeforeReset.uiCustomizationState == null ||
        gUIStateBeforeReset.drawInTitlebar == null) {
      return;
    }
    gUndoResetting = true;

    let uiCustomizationState = gUIStateBeforeReset.uiCustomizationState;
    let drawInTitlebar = gUIStateBeforeReset.drawInTitlebar;

    
    
    this._clearPreviousUIState();

    Services.prefs.setCharPref(kPrefCustomizationState, uiCustomizationState);
    Services.prefs.setBoolPref(kPrefDrawInTitlebar, drawInTitlebar);
    this.loadSavedState();
    
    
    if (gSavedState) {
      for (let areaId of Object.keys(gSavedState.placements)) {
        let placements = gSavedState.placements[areaId];
        gPlacements.set(areaId, placements);
      }
      this._rebuildRegisteredAreas();
    }

    gUndoResetting = false;
  },

  _clearPreviousUIState: function() {
    Object.getOwnPropertyNames(gUIStateBeforeReset).forEach((prop) => {
      gUIStateBeforeReset[prop] = null;
    });
  },

  removeExtraToolbar: function(aToolbarId) {
    this._resetExtraToolbars(aToolbarId);
  },

  



  isWidgetRemovable: function(aWidget) {
    let widgetId;
    let widgetNode;
    if (typeof aWidget == "string") {
      widgetId = aWidget;
    } else {
      widgetId = aWidget.id;
      widgetNode = aWidget;
    }
    let provider = this.getWidgetProvider(widgetId);

    if (provider == CustomizableUI.PROVIDER_API) {
      return gPalette.get(widgetId).removable;
    }

    if (provider == CustomizableUI.PROVIDER_XUL) {
      if (gBuildWindows.size == 0) {
        
        
        return true;
      }

      if (!widgetNode) {
        
        let [window,] = [...gBuildWindows][0];
        [, widgetNode] = this.getWidgetNode(widgetId, window);
      }
      
      
      
      if (!widgetNode) {
        return true;
      }
      return widgetNode.getAttribute("removable") == "true";
    }

    
    
    
    
    return true;
  },

  canWidgetMoveToArea: function(aWidgetId, aArea) {
    let placement = this.getPlacementOfWidget(aWidgetId);
    if (placement && placement.area != aArea) {
      
      if (this.isSpecialWidget(aWidgetId) && gAreas.has(aArea) &&
          gAreas.get(aArea).get("type") == CustomizableUI.TYPE_MENU_PANEL) {
        return false;
      }
      
      return this.isWidgetRemovable(aWidgetId);
    }

    return true;
  },

  ensureWidgetPlacedInWindow: function(aWidgetId, aWindow) {
    let placement = this.getPlacementOfWidget(aWidgetId);
    if (!placement) {
      return false;
    }
    let areaNodes = gBuildAreas.get(placement.area);
    if (!areaNodes) {
      return false;
    }
    let container = [...areaNodes].filter((n) => n.ownerDocument.defaultView == aWindow);
    if (!container.length) {
      return false;
    }
    let existingNode = container[0].getElementsByAttribute("id", aWidgetId)[0];
    if (existingNode) {
      return true;
    }

    this.insertNodeInWindow(aWidgetId, container[0], true);
    return true;
  },

  get inDefaultState() {
    for (let [areaId, props] of gAreas) {
      let defaultPlacements = props.get("defaultPlacements");
      
      if (!defaultPlacements) {
        continue;
      }

      let currentPlacements = gPlacements.get(areaId);
      
      
      
      
      
      
      let buildAreaNodes = gBuildAreas.get(areaId);
      if (buildAreaNodes && buildAreaNodes.size) {
        let container = [...buildAreaNodes][0];
        let removableOrDefault = (itemNodeOrItem) => {
          let item = (itemNodeOrItem && itemNodeOrItem.id) || itemNodeOrItem;
          let isRemovable = this.isWidgetRemovable(itemNodeOrItem);
          let isInDefault = defaultPlacements.indexOf(item) != -1;
          return isRemovable || isInDefault;
        };
        
        
        if (props.get("type") == CustomizableUI.TYPE_TOOLBAR) {
          let currentSet = container.currentSet;
          currentPlacements = currentSet ? currentSet.split(',') : [];
          currentPlacements = currentPlacements.filter(removableOrDefault);
        } else {
          
          currentPlacements = [...currentPlacements];
          currentPlacements = currentPlacements.filter((item) => {
            let itemNode = container.getElementsByAttribute("id", item)[0];
            return itemNode && removableOrDefault(itemNode || item);
          });
        }

        if (props.get("type") == CustomizableUI.TYPE_TOOLBAR) {
          let attribute = container.getAttribute("type") == "menubar" ? "autohide" : "collapsed";
          let collapsed = container.getAttribute(attribute) == "true";
          let defaultCollapsed = props.get("defaultCollapsed");
          if (defaultCollapsed !== null && collapsed != defaultCollapsed) {
            LOG("Found " + areaId + " had non-default toolbar visibility (expected " + defaultCollapsed + ", was " + collapsed + ")");
            return false;
          }
        }
      }
      LOG("Checking default state for " + areaId + ":\n" + currentPlacements.join(",") +
          "\nvs.\n" + defaultPlacements.join(","));

      if (currentPlacements.length != defaultPlacements.length) {
        return false;
      }

      for (let i = 0; i < currentPlacements.length; ++i) {
        if (currentPlacements[i] != defaultPlacements[i]) {
          LOG("Found " + currentPlacements[i] + " in " + areaId + " where " +
              defaultPlacements[i] + " was expected!");
          return false;
        }
      }
    }

    if (Services.prefs.prefHasUserValue(kPrefDrawInTitlebar)) {
      LOG(kPrefDrawInTitlebar + " pref is non-default");
      return false;
    }

    return true;
  },

  setToolbarVisibility: function(aToolbarId, aIsVisible) {
    
    let isFirstChangedToolbar = true;
    for (let window of CustomizableUI.windows) {
      let toolbar = window.document.getElementById(aToolbarId);
      if (toolbar) {
        window.setToolbarVisibility(toolbar, aIsVisible, isFirstChangedToolbar);
        isFirstChangedToolbar = false;
      }
    }
  },
};
Object.freeze(CustomizableUIInternal);

this.CustomizableUI = {
  


  get AREA_PANEL() "PanelUI-contents",
  


  get AREA_NAVBAR() "nav-bar",
  


  get AREA_MENUBAR() "toolbar-menubar",
  


  get AREA_TABSTRIP() "TabsToolbar",
  


  get AREA_BOOKMARKS() "PersonalToolbar",
  




  get AREA_ADDONBAR() "addon-bar",
  


  get TYPE_MENU_PANEL() "menu-panel",
  


  get TYPE_TOOLBAR() "toolbar",

  


  get PROVIDER_XUL() "xul",
  


  get PROVIDER_API() "api",
  


  get PROVIDER_SPECIAL() "special",

  


  get SOURCE_BUILTIN() "builtin",
  



  get SOURCE_EXTERNAL() "external",

  


  get WIDE_PANEL_CLASS() "panel-wide-item",
  


  get PANEL_COLUMN_COUNT() 3,

  


  get REASON_WINDOW_CLOSED() "window-closed",
  



  get REASON_AREA_UNREGISTERED() "area-unregistered",


  




  windows: {
    *[Symbol.iterator]() {
      for (let [window,] of gBuildWindows)
        yield window;
    }
  },

  


































































































  addListener: function(aListener) {
    CustomizableUIInternal.addListener(aListener);
  },
  



  removeListener: function(aListener) {
    CustomizableUIInternal.removeListener(aListener);
  },

  





















  registerArea: function(aName, aProperties) {
    CustomizableUIInternal.registerArea(aName, aProperties);
  },
  















  registerToolbarNode: function(aToolbar, aExistingChildren) {
    CustomizableUIInternal.registerToolbarNode(aToolbar, aExistingChildren);
  },
  




  registerMenuPanel: function(aPanel) {
    CustomizableUIInternal.registerMenuPanel(aPanel);
  },
  




















  unregisterArea: function(aName, aDestroyPlacements) {
    CustomizableUIInternal.unregisterArea(aName, aDestroyPlacements);
  },
  





















  addWidgetToArea: function(aWidgetId, aArea, aPosition) {
    CustomizableUIInternal.addWidgetToArea(aWidgetId, aArea, aPosition);
  },
  








  removeWidgetFromArea: function(aWidgetId) {
    CustomizableUIInternal.removeWidgetFromArea(aWidgetId);
  },
  














  moveWidgetWithinArea: function(aWidgetId, aPosition) {
    CustomizableUIInternal.moveWidgetWithinArea(aWidgetId, aPosition);
  },
  















  ensureWidgetPlacedInWindow: function(aWidgetId, aWindow) {
    return CustomizableUIInternal.ensureWidgetPlacedInWindow(aWidgetId, aWindow);
  },
  











  beginBatchUpdate: function() {
    CustomizableUIInternal.beginBatchUpdate();
  },
  











  endBatchUpdate: function(aForceDirty) {
    CustomizableUIInternal.endBatchUpdate(aForceDirty);
  },
  

























































  createWidget: function(aProperties) {
    return CustomizableUIInternal.wrapWidget(
      CustomizableUIInternal.createWidget(aProperties)
    );
  },
  










  destroyWidget: function(aWidgetId) {
    CustomizableUIInternal.destroyWidget(aWidgetId);
  },
  



























































  getWidget: function(aWidgetId) {
    return CustomizableUIInternal.wrapWidget(aWidgetId);
  },
  













  getUnusedWidgets: function(aWindowPalette) {
    return CustomizableUIInternal.getUnusedWidgets(aWindowPalette).map(
      CustomizableUIInternal.wrapWidget,
      CustomizableUIInternal
    );
  },
  










  getWidgetIdsInArea: function(aArea) {
    if (!gAreas.has(aArea)) {
      throw new Error("Unknown customization area: " + aArea);
    }
    if (!gPlacements.has(aArea)) {
      throw new Error("Area not yet restored");
    }

    
    return [...gPlacements.get(aArea)];
  },
  














  getWidgetsInArea: function(aArea) {
    return this.getWidgetIdsInArea(aArea).map(
      CustomizableUIInternal.wrapWidget,
      CustomizableUIInternal
    );
  },
  




  get areas() {
    return [area for ([area, props] of gAreas)];
  },
  









  getAreaType: function(aArea) {
    let area = gAreas.get(aArea);
    return area ? area.get("type") : null;
  },
  






  isToolbarDefaultCollapsed: function(aArea) {
    let area = gAreas.get(aArea);
    return area ? area.get("defaultCollapsed") : null;
  },
  























  getCustomizeTargetForArea: function(aArea, aWindow) {
    return CustomizableUIInternal.getCustomizeTargetForArea(aArea, aWindow);
  },
  






  reset: function() {
    CustomizableUIInternal.reset();
  },

  



  undoReset: function() {
    CustomizableUIInternal.undoReset();
  },

  






  removeExtraToolbar: function(aToolbarId) {
    CustomizableUIInternal.removeExtraToolbar(aToolbarId);
  },

  





  get canUndoReset() {
    return gUIStateBeforeReset.uiCustomizationState != null ||
           gUIStateBeforeReset.drawInTitlebar != null;
  },

  

















  getPlacementOfWidget: function(aWidgetId) {
    return CustomizableUIInternal.getPlacementOfWidget(aWidgetId, true);
  },
  

















  isWidgetRemovable: function(aWidgetId) {
    return CustomizableUIInternal.isWidgetRemovable(aWidgetId);
  },
  









  canWidgetMoveToArea: function(aWidgetId, aArea) {
    return CustomizableUIInternal.canWidgetMoveToArea(aWidgetId, aArea);
  },
  








  get inDefaultState() {
    return CustomizableUIInternal.inDefaultState;
  },

  




  setToolbarVisibility: function(aToolbarId, aIsVisible) {
    CustomizableUIInternal.setToolbarVisibility(aToolbarId, aIsVisible);
  },

  






















  getLocalizedProperty: function(aWidget, aProp, aFormatArgs, aDef) {
    return CustomizableUIInternal.getLocalizedProperty(aWidget, aProp,
      aFormatArgs, aDef);
  },
  








  addShortcut: function(aShortcutNode, aTargetNode) {
    return CustomizableUIInternal.addShortcut(aShortcutNode, aTargetNode);
  },
  





  hidePanelForNode: function(aNode) {
    CustomizableUIInternal.hidePanelForNode(aNode);
  },
  





  isSpecialWidget: function(aWidgetId) {
    return CustomizableUIInternal.isSpecialWidget(aWidgetId);
  },
  






  addPanelCloseListeners: function(aPanel) {
    CustomizableUIInternal.addPanelCloseListeners(aPanel);
  },
  






  removePanelCloseListeners: function(aPanel) {
    CustomizableUIInternal.removePanelCloseListeners(aPanel);
  },
  






  onWidgetDrag: function(aWidgetId, aArea) {
    CustomizableUIInternal.notifyListeners("onWidgetDrag", aWidgetId, aArea);
  },
  




  notifyStartCustomizing: function(aWindow) {
    CustomizableUIInternal.notifyListeners("onCustomizeStart", aWindow);
  },
  




  notifyEndCustomizing: function(aWindow) {
    CustomizableUIInternal.notifyListeners("onCustomizeEnd", aWindow);
  },

  







  dispatchToolboxEvent: function(aEvent, aDetails={}, aWindow=null) {
    CustomizableUIInternal.dispatchToolboxEvent(aEvent, aDetails, aWindow);
  },

  





  isAreaOverflowable: function(aAreaId) {
    let area = gAreas.get(aAreaId);
    return area ? area.get("type") == this.TYPE_TOOLBAR && area.get("overflowable")
                : false;
  },
  









  getPlaceForItem: function(aElement) {
    let place;
    let node = aElement;
    while (node && !place) {
      if (node.localName == "toolbar")
        place = "toolbar";
      else if (node.id == CustomizableUI.AREA_PANEL)
        place = "panel";
      else if (node.id == "customization-palette")
        place = "palette";

      node = node.parentNode;
    }
    return place;
  },

  



  isBuiltinToolbar: function(aToolbarId) {
    return CustomizableUIInternal._builtinToolbars.has(aToolbarId);
  },
};
Object.freeze(this.CustomizableUI);
Object.freeze(this.CustomizableUI.windows);












function WidgetGroupWrapper(aWidget) {
  this.isGroup = true;

  const kBareProps = ["id", "source", "type", "disabled", "label", "tooltiptext",
                      "showInPrivateBrowsing"];
  for (let prop of kBareProps) {
    let propertyName = prop;
    this.__defineGetter__(propertyName, function() aWidget[propertyName]);
  }

  this.__defineGetter__("provider", function() CustomizableUI.PROVIDER_API);

  this.__defineSetter__("disabled", function(aValue) {
    aValue = !!aValue;
    aWidget.disabled = aValue;
    for (let [,instance] of aWidget.instances) {
      instance.disabled = aValue;
    }
  });

  this.forWindow = function WidgetGroupWrapper_forWindow(aWindow) {
    let wrapperMap;
    if (!gSingleWrapperCache.has(aWindow)) {
      wrapperMap = new Map();
      gSingleWrapperCache.set(aWindow, wrapperMap);
    } else {
      wrapperMap = gSingleWrapperCache.get(aWindow);
    }
    if (wrapperMap.has(aWidget.id)) {
      return wrapperMap.get(aWidget.id);
    }

    let instance = aWidget.instances.get(aWindow.document);
    if (!instance &&
        (aWidget.showInPrivateBrowsing || !PrivateBrowsingUtils.isWindowPrivate(aWindow))) {
      instance = CustomizableUIInternal.buildWidget(aWindow.document,
                                                    aWidget);
    }

    let wrapper = new WidgetSingleWrapper(aWidget, instance);
    wrapperMap.set(aWidget.id, wrapper);
    return wrapper;
  };

  this.__defineGetter__("instances", function() {
    
    let placement = CustomizableUIInternal.getPlacementOfWidget(aWidget.id);
    if (!placement) {
      return [];
    }
    let area = placement.area;
    let buildAreas = gBuildAreas.get(area);
    if (!buildAreas) {
      return [];
    }
    return [this.forWindow(node.ownerDocument.defaultView) for (node of buildAreas)];
  });

  this.__defineGetter__("areaType", function() {
    let areaProps = gAreas.get(aWidget.currentArea);
    return areaProps && areaProps.get("type");
  });

  Object.freeze(this);
}





function WidgetSingleWrapper(aWidget, aNode) {
  this.isGroup = false;

  this.node = aNode;
  this.provider = CustomizableUI.PROVIDER_API;

  const kGlobalProps = ["id", "type"];
  for (let prop of kGlobalProps) {
    this[prop] = aWidget[prop];
  }

  const kNodeProps = ["label", "tooltiptext"];
  for (let prop of kNodeProps) {
    let propertyName = prop;
    
    
    this.__defineGetter__(propertyName,
                          function() aNode.getAttribute(propertyName));
  }

  this.__defineGetter__("disabled", function() aNode.disabled);
  this.__defineSetter__("disabled", function(aValue) {
    aNode.disabled = !!aValue;
  });

  this.__defineGetter__("anchor", function() {
    let anchorId;
    
    let placement = CustomizableUIInternal.getPlacementOfWidget(aWidget.id);
    if (placement) {
      anchorId = gAreas.get(placement.area).get("anchor");
    }
    if (!anchorId) {
      anchorId = aNode.getAttribute("cui-anchorid");
    }

    return anchorId ? aNode.ownerDocument.getElementById(anchorId)
                    : aNode;
  });

  this.__defineGetter__("overflowed", function() {
    return aNode.getAttribute("overflowedItem") == "true";
  });

  Object.freeze(this);
}









function XULWidgetGroupWrapper(aWidgetId) {
  this.isGroup = true;
  this.id = aWidgetId;
  this.type = "custom";
  this.provider = CustomizableUI.PROVIDER_XUL;

  this.forWindow = function XULWidgetGroupWrapper_forWindow(aWindow) {
    let wrapperMap;
    if (!gSingleWrapperCache.has(aWindow)) {
      wrapperMap = new Map();
      gSingleWrapperCache.set(aWindow, wrapperMap);
    } else {
      wrapperMap = gSingleWrapperCache.get(aWindow);
    }
    if (wrapperMap.has(aWidgetId)) {
      return wrapperMap.get(aWidgetId);
    }

    let instance = aWindow.document.getElementById(aWidgetId);
    if (!instance) {
      
      
      instance = aWindow.gNavToolbox.palette.getElementsByAttribute("id", aWidgetId)[0];
    }

    let wrapper = new XULWidgetSingleWrapper(aWidgetId, instance, aWindow.document);
    wrapperMap.set(aWidgetId, wrapper);
    return wrapper;
  };

  this.__defineGetter__("areaType", function() {
    let placement = CustomizableUIInternal.getPlacementOfWidget(aWidgetId);
    if (!placement) {
      return null;
    }

    let areaProps = gAreas.get(placement.area);
    return areaProps && areaProps.get("type");
  });

  this.__defineGetter__("instances", function() {
    return [this.forWindow(win) for ([win,] of gBuildWindows)];
  });

  Object.freeze(this);
}





function XULWidgetSingleWrapper(aWidgetId, aNode, aDocument) {
  this.isGroup = false;

  this.id = aWidgetId;
  this.type = "custom";
  this.provider = CustomizableUI.PROVIDER_XUL;

  let weakDoc = Cu.getWeakReference(aDocument);
  
  aDocument = null;

  this.__defineGetter__("node", function() {
    
    
    if (!weakDoc) {
      return null;
    }
    if (aNode) {
      
      if (aNode.ownerDocument.contains(aNode)) {
        return aNode;
      }
      
      let toolbox = aNode.ownerDocument.defaultView.gNavToolbox;
      if (toolbox && toolbox.palette && aNode.parentNode == toolbox.palette) {
        return aNode;
      }
      
      aNode = null;
    }

    let doc = weakDoc.get();
    if (doc) {
      
      aNode = CustomizableUIInternal.findWidgetInWindow(aWidgetId, doc.defaultView);
      return aNode;
    }
    
    weakDoc = null;
    return null;
  });

  this.__defineGetter__("anchor", function() {
    let anchorId;
    
    let placement = CustomizableUIInternal.getPlacementOfWidget(aWidgetId);
    if (placement) {
      anchorId = gAreas.get(placement.area).get("anchor");
    }

    let node = this.node;
    if (!anchorId && node) {
      anchorId = node.getAttribute("cui-anchorid");
    }

    return (anchorId && node) ? node.ownerDocument.getElementById(anchorId) : node;
  });

  this.__defineGetter__("overflowed", function() {
    let node = this.node;
    if (!node) {
      return false;
    }
    return node.getAttribute("overflowedItem") == "true";
  });

  Object.freeze(this);
}

const LAZY_RESIZE_INTERVAL_MS = 200;
const OVERFLOW_PANEL_HIDE_DELAY_MS = 500;

function OverflowableToolbar(aToolbarNode) {
  this._toolbar = aToolbarNode;
  this._collapsed = new Map();
  this._enabled = true;

  this._toolbar.setAttribute("overflowable", "true");
  let doc = this._toolbar.ownerDocument;
  this._target = this._toolbar.customizationTarget;
  this._list = doc.getElementById(this._toolbar.getAttribute("overflowtarget"));
  this._list.toolbox = this._toolbar.toolbox;
  this._list.customizationTarget = this._list;

  let window = this._toolbar.ownerDocument.defaultView;
  if (window.gBrowserInit.delayedStartupFinished) {
    this.init();
  } else {
    Services.obs.addObserver(this, "browser-delayed-startup-finished", false);
  }
}

OverflowableToolbar.prototype = {
  initialized: false,
  _forceOnOverflow: false,

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "browser-delayed-startup-finished" &&
        aSubject == this._toolbar.ownerDocument.defaultView) {
      Services.obs.removeObserver(this, "browser-delayed-startup-finished");
      this.init();
    }
  },

  init: function() {
    let doc = this._toolbar.ownerDocument;
    let window = doc.defaultView;
    window.addEventListener("resize", this);
    window.gNavToolbox.addEventListener("customizationstarting", this);
    window.gNavToolbox.addEventListener("aftercustomization", this);

    let chevronId = this._toolbar.getAttribute("overflowbutton");
    this._chevron = doc.getElementById(chevronId);
    this._chevron.addEventListener("command", this);
    this._chevron.addEventListener("dragover", this);
    this._chevron.addEventListener("dragend", this);

    let panelId = this._toolbar.getAttribute("overflowpanel");
    this._panel = doc.getElementById(panelId);
    this._panel.addEventListener("popuphiding", this);
    CustomizableUIInternal.addPanelCloseListeners(this._panel);

    CustomizableUI.addListener(this);

    
    if (this._toolbar.overflowedDuringConstruction) {
      this.onOverflow(this._toolbar.overflowedDuringConstruction);
      this._toolbar.overflowedDuringConstruction = null;
    }

    this.initialized = true;
  },

  uninit: function() {
    this._toolbar.removeEventListener("overflow", this._toolbar);
    this._toolbar.removeEventListener("underflow", this._toolbar);
    this._toolbar.removeAttribute("overflowable");

    if (!this.initialized) {
      Services.obs.removeObserver(this, "browser-delayed-startup-finished");
      return;
    }

    this._disable();

    let window = this._toolbar.ownerDocument.defaultView;
    window.removeEventListener("resize", this);
    window.gNavToolbox.removeEventListener("customizationstarting", this);
    window.gNavToolbox.removeEventListener("aftercustomization", this);
    this._chevron.removeEventListener("command", this);
    this._chevron.removeEventListener("dragover", this);
    this._chevron.removeEventListener("dragend", this);
    this._panel.removeEventListener("popuphiding", this);
    CustomizableUI.removeListener(this);
    CustomizableUIInternal.removePanelCloseListeners(this._panel);
  },

  handleEvent: function(aEvent) {
    switch(aEvent.type) {
      case "aftercustomization":
        this._enable();
        break;
      case "command":
        if (aEvent.target == this._chevron) {
          this._onClickChevron(aEvent);
        } else {
          this._panel.hidePopup();
        }
        break;
      case "customizationstarting":
        this._disable();
        break;
      case "dragover":
        this._showWithTimeout();
        break;
      case "dragend":
        this._panel.hidePopup();
        break;
      case "popuphiding":
        this._onPanelHiding(aEvent);
        break;
      case "resize":
        this._onResize(aEvent);
    }
  },

  show: function() {
    let deferred = Promise.defer();
    if (this._panel.state == "open") {
      deferred.resolve();
      return deferred.promise;
    }
    let doc = this._panel.ownerDocument;
    this._panel.hidden = false;
    let contextMenu = doc.getElementById(this._panel.getAttribute("context"));
    gELS.addSystemEventListener(contextMenu, 'command', this, true);
    let anchor = doc.getAnonymousElementByAttribute(this._chevron, "class", "toolbarbutton-icon");
    this._panel.openPopup(anchor || this._chevron);
    this._chevron.open = true;

    let overflowableToolbarInstance = this;
    this._panel.addEventListener("popupshown", function onPopupShown(aEvent) {
      this.removeEventListener("popupshown", onPopupShown);
      this.addEventListener("dragover", overflowableToolbarInstance);
      this.addEventListener("dragend", overflowableToolbarInstance);
      deferred.resolve();
    });

    return deferred.promise;
  },

  _onClickChevron: function(aEvent) {
    if (this._chevron.open) {
      this._panel.hidePopup();
      this._chevron.open = false;
    } else {
      this.show();
    }
  },

  _onPanelHiding: function(aEvent) {
    this._chevron.open = false;
    this._panel.removeEventListener("dragover", this);
    this._panel.removeEventListener("dragend", this);
    let doc = aEvent.target.ownerDocument;
    let contextMenu = doc.getElementById(this._panel.getAttribute("context"));
    gELS.removeSystemEventListener(contextMenu, 'command', this, true);
  },

  onOverflow: function(aEvent) {
    
    
    
    if (!this._enabled ||
        (aEvent && aEvent.target != this._toolbar.customizationTarget) ||
        (aEvent && aEvent.rangeParent))
      return;

    let child = this._target.lastChild;

    while (child && this._target.scrollLeftMax > 0) {
      let prevChild = child.previousSibling;

      if (child.getAttribute("overflows") != "false") {
        this._collapsed.set(child.id, this._target.clientWidth);
        child.setAttribute("overflowedItem", true);
        child.setAttribute("cui-anchorid", this._chevron.id);
        CustomizableUIInternal.notifyListeners("onWidgetOverflow", child, this._target);

        this._list.insertBefore(child, this._list.firstChild);
        if (!this._toolbar.hasAttribute("overflowing")) {
          CustomizableUI.addListener(this);
        }
        this._toolbar.setAttribute("overflowing", "true");
      }
      child = prevChild;
    };

    let win = this._target.ownerDocument.defaultView;
    win.UpdateUrlbarSearchSplitterState();
  },

  _onResize: function(aEvent) {
    if (!this._lazyResizeHandler) {
      this._lazyResizeHandler = new DeferredTask(this._onLazyResize.bind(this),
                                                 LAZY_RESIZE_INTERVAL_MS);
    }
    this._lazyResizeHandler.arm();
  },

  _moveItemsBackToTheirOrigin: function(shouldMoveAllItems) {
    let placements = gPlacements.get(this._toolbar.id);
    while (this._list.firstChild) {
      let child = this._list.firstChild;
      let minSize = this._collapsed.get(child.id);

      if (!shouldMoveAllItems &&
          minSize &&
          this._target.clientWidth <= minSize) {
        return;
      }

      this._collapsed.delete(child.id);
      let beforeNodeIndex = placements.indexOf(child.id) + 1;
      
      
      
      if (beforeNodeIndex == 0) {
        beforeNodeIndex = placements.length;
      }
      let inserted = false;
      for (; beforeNodeIndex < placements.length; beforeNodeIndex++) {
        let beforeNode = this._target.getElementsByAttribute("id", placements[beforeNodeIndex])[0];
        if (beforeNode) {
          this._target.insertBefore(child, beforeNode);
          inserted = true;
          break;
        }
      }
      if (!inserted) {
        this._target.appendChild(child);
      }
      child.removeAttribute("cui-anchorid");
      child.removeAttribute("overflowedItem");
      CustomizableUIInternal.notifyListeners("onWidgetUnderflow", child, this._target);
    }

    let win = this._target.ownerDocument.defaultView;
    win.UpdateUrlbarSearchSplitterState();

    if (!this._collapsed.size) {
      this._toolbar.removeAttribute("overflowing");
      CustomizableUI.removeListener(this);
    }
  },

  _onLazyResize: function() {
    if (!this._enabled)
      return;

    if (this._target.scrollLeftMax > 0) {
      this.onOverflow();
    } else {
      this._moveItemsBackToTheirOrigin();
    }
  },

  _disable: function() {
    this._enabled = false;
    this._moveItemsBackToTheirOrigin(true);
    if (this._lazyResizeHandler) {
      this._lazyResizeHandler.disarm();
    }
  },

  _enable: function() {
    this._enabled = true;
    this.onOverflow();
  },

  onWidgetBeforeDOMChange: function(aNode, aNextNode, aContainer) {
    if (aContainer != this._target && aContainer != this._list) {
      return;
    }
    
    
    
    if (aNode.parentNode == this._list) {
      let updatedMinSize;
      if (aNode.previousSibling) {
        updatedMinSize = this._collapsed.get(aNode.previousSibling.id);
      } else {
        
        updatedMinSize = 1;
      }
      let nextItem = aNode.nextSibling;
      while (nextItem) {
        this._collapsed.set(nextItem.id, updatedMinSize);
        nextItem = nextItem.nextSibling;
      }
    }
  },

  onWidgetAfterDOMChange: function(aNode, aNextNode, aContainer) {
    if (aContainer != this._target && aContainer != this._list) {
      return;
    }

    let nowInBar = aNode.parentNode == aContainer;
    let nowOverflowed = aNode.parentNode == this._list;
    let wasOverflowed = this._collapsed.has(aNode.id);

    
    if (!wasOverflowed) {
      
      if (nowOverflowed) {
        
        
        let prevId = aNode.previousSibling.id;
        let minSize = this._collapsed.get(prevId);
        this._collapsed.set(aNode.id, minSize);
        aNode.setAttribute("cui-anchorid", this._chevron.id);
        aNode.setAttribute("overflowedItem", true);
        CustomizableUIInternal.notifyListeners("onWidgetOverflow", aNode, this._target);
      }
      
      
      
      else if (!nowInBar) {
        this._moveItemsBackToTheirOrigin(true);
      }
      
      
    }
    
    else {
      
      if (!nowOverflowed) {
        this._collapsed.delete(aNode.id);
        aNode.removeAttribute("cui-anchorid");
        aNode.removeAttribute("overflowedItem");
        CustomizableUIInternal.notifyListeners("onWidgetUnderflow", aNode, this._target);

        if (!this._collapsed.size) {
          this._toolbar.removeAttribute("overflowing");
          CustomizableUI.removeListener(this);
        }
      }
      
      else {
        if (aNode.previousSibling) {
          let prevId = aNode.previousSibling.id;
          let minSize = this._collapsed.get(prevId);
          this._collapsed.set(aNode.id, minSize);
        } else {
          
          
          this._moveItemsBackToTheirOrigin();
        }
      }
    }
  },

  findOverflowedInsertionPoints: function(aNode) {
    let newNodeCanOverflow = aNode.getAttribute("overflows") != "false";
    let areaId = this._toolbar.id;
    let placements = gPlacements.get(areaId);
    let nodeIndex = placements.indexOf(aNode.id);
    let nodeBeforeNewNodeIsOverflown = false;

    let loopIndex = -1;
    while (++loopIndex < placements.length) {
      let nextNodeId = placements[loopIndex];
      if (loopIndex > nodeIndex) {
        if (newNodeCanOverflow && this._collapsed.has(nextNodeId)) {
          let nextNode = this._list.getElementsByAttribute("id", nextNodeId).item(0);
          if (nextNode) {
            return [this._list, nextNode];
          }
        }
        if (!nodeBeforeNewNodeIsOverflown || !newNodeCanOverflow) {
          let nextNode = this._target.getElementsByAttribute("id", nextNodeId).item(0);
          if (nextNode) {
            return [this._target, nextNode];
          }
        }
      } else if (loopIndex < nodeIndex && this._collapsed.has(nextNodeId)) {
        nodeBeforeNewNodeIsOverflown = true;
      }
    }

    let containerForAppending = (this._collapsed.size && newNodeCanOverflow) ?
                                this._list : this._target;
    return [containerForAppending, null];
  },

  getContainerFor: function(aNode) {
    if (aNode.getAttribute("overflowedItem") == "true") {
      return this._list;
    }
    return this._target;
  },

  _hideTimeoutId: null,
  _showWithTimeout: function() {
    this.show().then(function () {
      let window = this._toolbar.ownerDocument.defaultView;
      if (this._hideTimeoutId) {
        window.clearTimeout(this._hideTimeoutId);
      }
      this._hideTimeoutId = window.setTimeout(() => {
        if (!this._panel.firstChild.matches(":hover")) {
          this._panel.hidePopup();
        }
      }, OVERFLOW_PANEL_HIDE_DELAY_MS);
    }.bind(this));
  },
};

CustomizableUIInternal.initialize();
