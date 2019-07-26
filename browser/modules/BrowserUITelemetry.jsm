



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

XPCOMUtils.defineLazyGetter(this, "DEFAULT_AREA_PLACEMENTS", function() {
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
      "developer-button",
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

XPCOMUtils.defineLazyGetter(this, "DEFAULT_AREAS", function() {
  return Object.keys(DEFAULT_AREA_PLACEMENTS);
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

  let panelPlacements = DEFAULT_AREA_PLACEMENTS["PanelUI-contents"];
  if (panelPlacements.indexOf("characterencoding-button") == -1) {
    result.push("characterencoding-button");
  }

  return result;
});

XPCOMUtils.defineLazyGetter(this, "DEFAULT_ITEMS", function() {
  let result = [];
  for (let [, buttons] of Iterator(DEFAULT_AREA_PLACEMENTS)) {
    result = result.concat(buttons);
  }
  return result;
});

XPCOMUtils.defineLazyGetter(this, "ALL_BUILTIN_ITEMS", function() {
  
  
  const SPECIAL_CASES = [
    "back-button",
    "forward-button",
    "urlbar-stop-button",
    "urlbar-go-button",
    "urlbar-reload-button",
    "searchbar",
    "cut-button",
    "copy-button",
    "paste-button",
    "zoom-out-button",
    "zoom-reset-button",
    "zoom-in-button",
    "BMB_bookmarksPopup",
    "BMB_unsortedBookmarksPopup",
    "BMB_bookmarksToolbarPopup",
  ]
  return DEFAULT_ITEMS.concat(PALETTE_ITEMS)
                      .concat(SPECIAL_CASES);
});

const OTHER_MOUSEUP_MONITORED_ITEMS = [
  "PlacesChevron",
  "PlacesToolbarItems",
];






const WINDOW_DURATION_MAP = new WeakMap();

this.BrowserUITelemetry = {
  init: function() {
    UITelemetry.addSimpleMeasureFunction("toolbars",
                                         this.getToolbarMeasures.bind(this));
    Services.obs.addObserver(this, "sessionstore-windows-restored", false);
    Services.obs.addObserver(this, "browser-delayed-startup-finished", false);
    CustomizableUI.addListener(this);
  },

  observe: function(aSubject, aTopic, aData) {
    switch(aTopic) {
      case "sessionstore-windows-restored":
        this._gatherFirstWindowMeasurements();
        break;
      case "browser-delayed-startup-finished":
        this._registerWindow(aSubject);
        break;
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
  _countEvent: function(aKeyArray) {
    let countObject = this._ensureObjectChain(aKeyArray, 0);
    let lastItemKey = aKeyArray[aKeyArray.length - 1];
    countObject[lastItemKey]++;
  },

  _countMouseUpEvent: function(aCategory, aAction, aButton) {
    const BUTTONS = ["left", "middle", "right"];
    let buttonKey = BUTTONS[aButton];
    if (buttonKey) {
      this._countEvent([aCategory, aAction, buttonKey]);
    }
  },

  _firstWindowMeasurements: null,
  _gatherFirstWindowMeasurements: function() {
    
    
    
    
    
    let win = RecentWindow.getMostRecentBrowserWindow({
      private: false,
      allowPopups: false,
    });

    
    this._firstWindowMeasurements = win ? this._getWindowMeasurements(win)
                                        : {};
  },

  _registerWindow: function(aWindow) {
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

    WINDOW_DURATION_MAP.set(aWindow, {});
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
      return;
    }

    
    
    let candidate = getIDBasedOnFirstIDedAncestor(item);
    if (ALL_BUILTIN_ITEMS.indexOf(candidate) != -1) {
      this._countMouseUpEvent("click-builtin-item", candidate, aEvent.button);
    }
  },

  _getWindowMeasurements: function(aWindow) {
    let document = aWindow.document;
    let result = {};

    
    
    result.sizemode = document.documentElement.getAttribute("sizemode");

    
    let bookmarksBar = document.getElementById("PersonalToolbar");
    result.bookmarksBarEnabled = bookmarksBar && !bookmarksBar.collapsed;

    
    
    let defaultKept = [];
    let defaultMoved = [];
    let nondefaultAdded = [];

    for (let areaID of CustomizableUI.areas) {
      let items = CustomizableUI.getWidgetIdsInArea(areaID);
      for (let item of items) {
        
        if (DEFAULT_ITEMS.indexOf(item) != -1) {
          
          
          
          
          if (Array.isArray(DEFAULT_AREA_PLACEMENTS[areaID]) &&
              DEFAULT_AREA_PLACEMENTS[areaID].indexOf(item) != -1) {
            
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

    
    let addonToolbars = 0;
    let toolbars = document.querySelectorAll("toolbar[customizable=true]");
    for (let toolbar of toolbars) {
      if (DEFAULT_AREAS.indexOf(toolbar.id) == -1) {
        addonToolbars++;
      }
    }
    result.addonToolbars = addonToolbars;

    
    let winEnumerator = Services.wm.getEnumerator("navigator:browser");
    let visibleTabs = [];
    let hiddenTabs = [];
    while (winEnumerator.hasMoreElements()) {
      let someWin = winEnumerator.getNext();
      if (someWin.gBrowser) {
        let visibleTabsNum = someWin.gBrowser.visibleTabs.length;
        visibleTabs.push(visibleTabsNum);
        hiddenTabs.push(someWin.gBrowser.tabs.length - visibleTabsNum);
      }
    }
    result.visibleTabs = visibleTabs;
    result.hiddenTabs = hiddenTabs;

    return result;
  },

  getToolbarMeasures: function() {
    let result = this._firstWindowMeasurements || {};
    result.countableEvents = this._countableEvents;
    result.durations = this._durations;
    return result;
  },

  countCustomizationEvent: function(aEventType) {
    this._countEvent(["customize", aEventType]);
  },

  _durations: {
    customization: [],
  },

  onCustomizeStart: function(aWindow) {
    this._countEvent(["customize", "start"]);
    let durationMap = WINDOW_DURATION_MAP.get(aWindow);
    if (!durationMap) {
      durationMap = {};
      WINDOW_DURATION_MAP.set(aWindow, durationMap);
    }
    durationMap.customization = aWindow.performance.now();
  },

  onCustomizeEnd: function(aWindow) {
    let durationMap = WINDOW_DURATION_MAP.get(aWindow);
    if (durationMap && "customization" in durationMap) {
      let duration = aWindow.performance.now() - durationMap.customization;
      this._durations.customization.push(duration);
      delete durationMap.customization;
    }
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
