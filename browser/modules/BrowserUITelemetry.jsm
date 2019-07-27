



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
XPCOMUtils.defineLazyModuleGetter(this, "UITour",
  "resource:///modules/UITour.jsm");
XPCOMUtils.defineLazyGetter(this, "Timer", function() {
  let timer = {};
  Cu.import("resource://gre/modules/Timer.jsm", timer);
  return timer;
});

const MS_SECOND = 1000;
const MS_MINUTE = MS_SECOND * 60;
const MS_HOUR = MS_MINUTE * 60;

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

  if (Services.metro && Services.metro.supported) {
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
    "web-apps-button",
  ];

  let panelPlacements = DEFAULT_AREA_PLACEMENTS["PanelUI-contents"];
  if (panelPlacements.indexOf("characterencoding-button") == -1) {
    result.push("characterencoding-button");
  }

  if (Services.prefs.getBoolPref("privacy.panicButton.enabled")) {
    result.push("panic-button");
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
    "search-go-button",
  ]
  return DEFAULT_ITEMS.concat(PALETTE_ITEMS)
                      .concat(SPECIAL_CASES);
});

const OTHER_MOUSEUP_MONITORED_ITEMS = [
  "PlacesChevron",
  "PlacesToolbarItems",
  "menubar-items",
];




const MOUSEDOWN_MONITORED_ITEMS = [
  "PanelUI-menu-button",
];






const WINDOW_DURATION_MAP = new WeakMap();


const BUCKET_DEFAULT = "__DEFAULT__";

const BUCKET_PREFIX = "bucket_";


const BUCKET_SEPARATOR = "|";

this.BrowserUITelemetry = {
  init: function() {
    UITelemetry.addSimpleMeasureFunction("toolbars",
                                         this.getToolbarMeasures.bind(this));
    UITelemetry.addSimpleMeasureFunction("contextmenu",
                                         this.getContextMenuInfo.bind(this));
    
    
    UITelemetry.addSimpleMeasureFunction("UITour",
                                         () => UITour.getTelemetry());

    Services.obs.addObserver(this, "sessionstore-windows-restored", false);
    Services.obs.addObserver(this, "browser-delayed-startup-finished", false);
    Services.obs.addObserver(this, "autocomplete-did-enter-text", false);
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
      case "autocomplete-did-enter-text":
        let input = aSubject.QueryInterface(Ci.nsIAutoCompleteInput);
        if (input && input.id == "urlbar" && !input.inPrivateContext &&
            input.popup.selectedIndex != -1) {
          this._logAwesomeBarSearchResult(input.textValue);
        }
        break;
    }
  },

  

































  _ensureObjectChain: function(aKeys, aEndWith, aRoot) {
    let current = aRoot;
    let parent = null;
    aKeys.unshift(this._bucket);
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
  _countEvent: function(aKeyArray, root=this._countableEvents) {
    let countObject = this._ensureObjectChain(aKeyArray, 0, root);
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

    Services.search.init(rv => {
      
      
      let hasWindow = win && !win.closed;
      this._firstWindowMeasurements = hasWindow ? this._getWindowMeasurements(win, rv)
                                                : {};
    });
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

    for (let itemID of MOUSEDOWN_MONITORED_ITEMS) {
      let item = document.getElementById(itemID);
      if (item) {
        item.addEventListener("mousedown", this);
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

    for (let itemID of MOUSEDOWN_MONITORED_ITEMS) {
      let item = document.getElementById(itemID);
      if (item) {
        item.removeEventListener("mousedown", this);
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
      case "mousedown":
        this._handleMouseDown(aEvent);
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
      case "menubar-items":
        this._menubarMouseUp(aEvent);
        break;
      default:
        this._checkForBuiltinItem(aEvent);
    }
  },

  _handleMouseDown: function(aEvent) {
    if (aEvent.currentTarget.id == "PanelUI-menu-button") {
      
      
      
      
      this._countMouseUpEvent("click-menu-button", "button", aEvent.button);
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

  _menubarMouseUp: function(aEvent) {
    let target = aEvent.originalTarget;
    let tag = target.localName
    let result = (tag == "menu" || tag == "menuitem") ? tag : "other";
    this._countMouseUpEvent("click-menubar", result, aEvent.button);
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

    
    
    if (ALL_BUILTIN_ITEMS.indexOf(item.getAttribute("anonid")) != -1) {
      this._countMouseUpEvent("click-builtin-item", item.getAttribute("anonid"), aEvent.button);
      return;
    }

    
    
    let candidate = getIDBasedOnFirstIDedAncestor(item);
    if (ALL_BUILTIN_ITEMS.indexOf(candidate) != -1) {
      this._countMouseUpEvent("click-builtin-item", candidate, aEvent.button);
    }
  },

  _getWindowMeasurements: function(aWindow, searchResult) {
    let document = aWindow.document;
    let result = {};

    
    
    result.sizemode = document.documentElement.getAttribute("sizemode");

    
    let bookmarksBar = document.getElementById("PersonalToolbar");
    result.bookmarksBarEnabled = bookmarksBar && !bookmarksBar.collapsed;

    
    
    let menuBar = document.getElementById("toolbar-menubar");
    result.menuBarEnabled =
      menuBar && Services.appinfo.OS != "Darwin"
              && menuBar.getAttribute("autohide") != "true";

    
    result.titleBarEnabled = !Services.prefs.getBoolPref("browser.tabs.drawInTitlebar");

    
    
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

    if (Components.isSuccessCode(searchResult)) {
      result.currentSearchEngine = Services.search.currentEngine.name;
    }
    result.oneOffSearchEnabled = Services.prefs.getBoolPref("browser.search.showOneOffButtons");
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

  countSearchEvent: function(source, query, selection) {
    this._countEvent(["search", source]);
    if ((/^[a-zA-Z]+:[^\/\\]/).test(query)) {
      this._countEvent(["search", "urlbar-keyword"]);
    }
    if (selection) {
      this._countEvent(["search", "selection", source, selection.index, selection.kind]);
    }
  },

  countOneoffSearchEvent: function(id, type, where) {
    this._countEvent(["search-oneoff", id, type, where]);
  },

  countSearchSettingsEvent: function(source) {
    this._countEvent(["click-builtin-item", source, "search-settings"]);
  },

  countPanicEvent: function(timeId) {
    this._countEvent(["forget-button", timeId]);
  },

  _logAwesomeBarSearchResult: function (url) {
    let spec = Services.search.parseSubmissionURL(url);
    if (spec.engine) {
      let matchedEngine = "default";
      if (spec.engine.name !== Services.search.currentEngine.name) {
        matchedEngine = "other";
      }
      this.countSearchEvent("autocomplete-" + matchedEngine);
    }
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

    durationMap.customization = {
      start: aWindow.performance.now(),
      bucket: this._bucket,
    };
  },

  onCustomizeEnd: function(aWindow) {
    let durationMap = WINDOW_DURATION_MAP.get(aWindow);
    if (durationMap && "customization" in durationMap) {
      let duration = aWindow.performance.now() - durationMap.customization.start;
      this._durations.customization.push({
        duration: duration,
        bucket: durationMap.customization.bucket,
      });
      delete durationMap.customization;
    }
  },

  _contextMenuItemWhitelist: new Set([
    "close-without-interaction", 
    "custom-page-item", 
    "unknown", 
    
    "navigation", "back", "forward", "reload", "stop", "bookmarkpage",
    "spell-no-suggestions", "spell-add-to-dictionary",
    "spell-undo-add-to-dictionary", "openlinkincurrent", "openlinkintab",
    "openlink", "openlinkprivate", "bookmarklink", "sharelink", "savelink",
    "marklinkMenu", "copyemail", "copylink", "media-play", "media-pause",
    "media-mute", "media-unmute", "media-playbackrate",
    "media-playbackrate-050x", "media-playbackrate-100x",
    "media-playbackrate-150x", "media-playbackrate-200x",
    "media-showcontrols", "media-hidecontrols", "video-showstats",
    "video-hidestats", "video-fullscreen", "leave-dom-fullscreen",
    "reloadimage", "viewimage", "viewvideo", "copyimage-contents", "copyimage",
    "copyvideourl", "copyaudiourl", "saveimage", "shareimage", "sendimage",
    "setDesktopBackground", "viewimageinfo", "viewimagedesc", "savevideo",
    "sharevideo", "saveaudio", "video-saveimage", "sendvideo", "sendaudio",
    "ctp-play", "ctp-hide", "sharepage", "savepage", "markpageMenu",
    "viewbgimage", "undo", "cut", "copy", "paste", "delete", "selectall",
    "keywordfield", "searchselect", "shareselect", "frame", "showonlythisframe",
    "openframeintab", "openframe", "reloadframe", "bookmarkframe", "saveframe",
    "printframe", "viewframesource", "viewframeinfo",
    "viewpartialsource-selection", "viewpartialsource-mathml",
    "viewsource", "viewinfo", "spell-check-enabled",
    "spell-add-dictionaries-main", "spell-dictionaries",
    "spell-dictionaries-menu", "spell-add-dictionaries",
    "bidi-text-direction-toggle", "bidi-page-direction-toggle", "inspect",
    "media-eme-learn-more"
  ]),

  _contextMenuInteractions: {},

  registerContextMenuInteraction: function(keys, itemID) {
    if (itemID) {
      if (!this._contextMenuItemWhitelist.has(itemID)) {
        itemID = "other-item";
      }
      keys.push(itemID);
    }

    this._countEvent(keys, this._contextMenuInteractions);
  },

  getContextMenuInfo: function() {
    return this._contextMenuInteractions;
  },

  _bucket: BUCKET_DEFAULT,
  _bucketTimer: null,

  


  get BUCKET_DEFAULT() BUCKET_DEFAULT,

  


  get BUCKET_PREFIX() BUCKET_PREFIX,

  



  get BUCKET_SEPARATOR() BUCKET_SEPARATOR,

  get currentBucket() {
    return this._bucket;
  },

  





  setBucket: function(aName) {
    if (this._bucketTimer) {
      Timer.clearTimeout(this._bucketTimer);
      this._bucketTimer = null;
    }

    if (aName)
      this._bucket = BUCKET_PREFIX + aName;
    else
      this._bucket = BUCKET_DEFAULT;
  },

  






























  setExpiringBucket: function(aName, aTimeSteps, aTimeOffset = 0) {
    if (aTimeSteps.length === 0) {
      this.setBucket(null);
      return;
    }

    if (this._bucketTimer) {
      Timer.clearTimeout(this._bucketTimer);
      this._bucketTimer = null;
    }

    
    
    let steps = [...aTimeSteps];
    let msec = steps.shift();
    let postfix = this._toTimeStr(msec);
    this.setBucket(aName + BUCKET_SEPARATOR + postfix);

    this._bucketTimer = Timer.setTimeout(() => {
      this._bucketTimer = null;
      this.setExpiringBucket(aName, steps, aTimeOffset + msec);
    }, msec - aTimeOffset);
  },

  













  _toTimeStr: function(aTimeMS) {
    let timeStr = "";

    function reduce(aUnitLength, aSymbol) {
      if (aTimeMS >= aUnitLength) {
        let units = Math.floor(aTimeMS / aUnitLength);
        aTimeMS = aTimeMS - (units * aUnitLength)
        timeStr += units + aSymbol;
      }
    }

    reduce(MS_HOUR, "h");
    reduce(MS_MINUTE, "m");
    reduce(MS_SECOND, "s");
    reduce(1, "ms");

    return timeStr;
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
