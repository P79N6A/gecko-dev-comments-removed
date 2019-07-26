



"use strict";

this.EXPORTED_SYMBOLS = ["BrowserUITelemetry"];

const {interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "UITelemetry",
  "resource://gre/modules/UITelemetry.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
  "resource:///modules/RecentWindow.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
  "resource:///modules/CustomizableUI.jsm");

XPCOMUtils.defineLazyGetter(this, "DEFAULT_TOOLBAR_PLACEMENTS", function() {
  let result = {
    "PanelUI-contents": [
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
    ],
    "nav-bar": [
      "urlbar-container",
      "search-container",
      "webrtc-status-button",
      "bookmarks-menu-button",
      "downloads-button",
      "home-button",
      "social-share-button",
    ],
    
    
    
    "toolbar-menubar": [
      "menubar-items",
    ],
    "TabsToolbar": [
      "tabbrowser-tabs",
      "new-tab-button",
      "alltabs-button",
      "tabs-closebutton",
    ],
    "PersonalToolbar": [
      "personal-bookmarks",
    ],
  };

  let showCharacterEncoding = Services.prefs.getComplexValue(
    "browser.menu.showCharacterEncoding",
    Ci.nsIPrefLocalizedString
  ).data;
  if (showCharacterEncoding == "true") {
    result["PanelUI-contents"].push("characterencoding-button");
  }

  if (Services.sysinfo.getProperty("hasWindowsTouchInterface")) {
    result["PanelUI-contents"].push("switch-to-metro-button");
  }

  return result;
});

XPCOMUtils.defineLazyGetter(this, "PALETTE_ITEMS", function() {
  let result = [
    "open-file-button",
    "developer-button",
    "feed-button",
    "email-link-button",
    "sync-button",
    "tabview-button",
  ];

  let panelPlacements = DEFAULT_TOOLBAR_PLACEMENTS["PanelUI-contents"];
  if (panelPlacements.indexOf("characterencoding-button") == -1) {
    result.push("characterencoding-button");
  }

  return result;
});

XPCOMUtils.defineLazyGetter(this, "DEFAULT_ITEMS", function() {
  let result = [];
  for (let [, buttons] of Iterator(DEFAULT_TOOLBAR_PLACEMENTS)) {
    result = result.concat(buttons);
  }
  return result;
});

const ALL_BUILTIN_ITEMS = [
  "fullscreen-button",
  "switch-to-metro-button",
  "bookmarks-menu-button",
];

const OTHER_MOUSEUP_MONITORED_ITEMS = [
  "PlacesChevron",
  "PlacesToolbarItems",
];

this.BrowserUITelemetry = {
  init: function() {
    UITelemetry.addSimpleMeasureFunction("toolbars",
                                         this.getToolbarMeasures.bind(this));
    Services.obs.addObserver(this, "browser-delayed-startup-finished", false);
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "browser-delayed-startup-finished") {
      this._registerWindow(aSubject);
    }
  },

  































  _ensureObjectChain: function(aKeys, aEndWith) {
    let current = this._countableEvents;
    let parent = null;
    for (let [i, key] of Iterator(aKeys)) {
      if (!(key in current)) {
        if (i == aKeys.length - 1) {
          current[key] = aEndWith;
        } else {
          current[key] = {};
        }
      }
      parent = current;
      current = current[key];
    }
    return parent;
  },

  _countableEvents: {},
  _countMouseUpEvent: function(aCategory, aAction, aButton) {
    const BUTTONS = ["left", "middle", "right"];
    let buttonKey = BUTTONS[aButton];
    if (buttonKey) {
      let countObject =
        this._ensureObjectChain([aCategory, aAction, buttonKey], 0);
      countObject[buttonKey]++;
    }
  },

  _firstWindowMeasurements: null,
  _registerWindow: function(aWindow) {
    
    
    
    
    
    if (!this._firstWindowMeasurements && aWindow.toolbar.visible) {
      this._firstWindowMeasurements = this._getWindowMeasurements(aWindow);
    }

    aWindow.addEventListener("unload", this);
    let document = aWindow.document;

    for (let areaID of CustomizableUI.areas) {
      let areaNode = document.getElementById(areaID);
      if (areaNode) {
        (areaNode.customizationTarget || areaNode).addEventListener("mouseup", this);
      }
    }

    for (let itemID of OTHER_MOUSEUP_MONITORED_ITEMS) {
      let item = document.getElementById(itemID);
      if (item) {
        item.addEventListener("mouseup", this);
      }
    }
  },

  _unregisterWindow: function(aWindow) {
    aWindow.removeEventListener("unload", this);
    let document = aWindow.document;

    for (let areaID of CustomizableUI.areas) {
      let areaNode = document.getElementById(areaID);
      if (areaNode) {
        (areaNode.customizationTarget || areaNode).removeEventListener("mouseup", this);
      }
    }

    for (let itemID of OTHER_MOUSEUP_MONITORED_ITEMS) {
      let item = document.getElementById(itemID);
      if (item) {
        item.removeEventListener("mouseup", this);
      }
    }
  },

  handleEvent: function(aEvent) {
    switch(aEvent.type) {
      case "unload":
        this._unregisterWindow(aEvent.currentTarget);
        break;
      case "mouseup":
        this._handleMouseUp(aEvent);
        break;
    }
  },

  _handleMouseUp: function(aEvent) {
    let targetID = aEvent.currentTarget.id;

    switch (targetID) {
      case "PlacesToolbarItems":
        this._PlacesToolbarItemsMouseUp(aEvent);
        break;
      case "PlacesChevron":
        this._PlacesChevronMouseUp(aEvent);
        break;
      default:
        this._checkForBuiltinItem(aEvent);
    }
  },

  _PlacesChevronMouseUp: function(aEvent) {
    let target = aEvent.originalTarget;
    let result = target.id == "PlacesChevron" ? "chevron" : "overflowed-item";
    this._countMouseUpEvent("click-bookmarks-bar", result, aEvent.button);
  },

  _PlacesToolbarItemsMouseUp: function(aEvent) {
    let target = aEvent.originalTarget;
    
    if (!target.classList.contains("bookmark-item")) {
      return;
    }

    let result = target.hasAttribute("container") ? "container" : "item";
    this._countMouseUpEvent("click-bookmarks-bar", result, aEvent.button);
  },

  _bookmarksMenuButtonMouseUp: function(aEvent) {
    let bookmarksWidget = CustomizableUI.getWidget("bookmarks-menu-button");
    if (bookmarksWidget.areaType == CustomizableUI.TYPE_MENU_PANEL) {
      
      
      this._countMouseUpEvent("click-bookmarks-menu-button", "in-panel",
                              aEvent.button);
    } else {
      let clickedItem = aEvent.originalTarget;
      
      
      
      let action = "menu";
      if (clickedItem.getAttribute("anonid") == "button") {
        
        
        
        let bookmarksMenuNode =
          bookmarksWidget.forWindow(aEvent.target.ownerGlobal).node;
        action = bookmarksMenuNode.hasAttribute("starred") ? "edit" : "add";
      }
      this._countMouseUpEvent("click-bookmarks-menu-button", action,
                              aEvent.button);
    }
  },

  _checkForBuiltinItem: function(aEvent) {
    let item = aEvent.originalTarget;

    
    
    if (item.id == "bookmarks-menu-button" ||
        getIDBasedOnFirstIDedAncestor(item) == "bookmarks-menu-button") {
      this._bookmarksMenuButtonMouseUp(aEvent);
      return;
    }

    
    
    if (ALL_BUILTIN_ITEMS.indexOf(item.id) != -1) {
      
      
      this._countMouseUpEvent("click-builtin-item", item.id, aEvent.button);
    }
  },

  _getWindowMeasurements: function(aWindow) {
    let document = aWindow.document;
    let result = {};

    
    let bookmarksBar = document.getElementById("PersonalToolbar");
    result.bookmarksBarEnabled = bookmarksBar && !bookmarksBar.collapsed;

    
    
    let defaultKept = [];
    let defaultMoved = [];
    let nondefaultAdded = [];

    for (let areaID of CustomizableUI.areas) {
      let items = CustomizableUI.getWidgetIdsInArea(areaID);
      for (let item of items) {
        
        if (DEFAULT_ITEMS.indexOf(item) != -1) {
          
          
          
          
          if (Array.isArray(DEFAULT_TOOLBAR_PLACEMENTS[areaID]) &&
              DEFAULT_TOOLBAR_PLACEMENTS[areaID].indexOf(item) != -1) {
            
            defaultKept.push(item);
          } else {
            defaultMoved.push(item);
          }
        } else if (PALETTE_ITEMS.indexOf(item) != -1) {
          
          nondefaultAdded.push(item);
        }
        
      }
    }

    
    
    let paletteItems =
      CustomizableUI.getUnusedWidgets(aWindow.gNavToolbox.palette);
    let defaultRemoved = [item.id for (item of paletteItems)
                          if (DEFAULT_ITEMS.indexOf(item.id) != -1)];

    result.defaultKept = defaultKept;
    result.defaultMoved = defaultMoved;
    result.nondefaultAdded = nondefaultAdded;
    result.defaultRemoved = defaultRemoved;

    return result;
  },

  getToolbarMeasures: function() {
    let result = this._firstWindowMeasurements || {};
    result.countableEvents = this._countableEvents;
    return result;
  },
};







function getIDBasedOnFirstIDedAncestor(aNode) {
  while (!aNode.id) {
    aNode = aNode.parentNode;
    if (!aNode) {
      return null;
    }
  }

  return aNode.id;
}
