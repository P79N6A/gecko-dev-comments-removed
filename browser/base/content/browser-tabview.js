# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

let TabView = {
  _deck: null,
  _iframe: null,
  _window: null,
  _initialized: false,
  _browserKeyHandlerInitialized: false,
  _closedLastVisibleTabBeforeFrameInitialized: false,
  _isFrameLoading: false,
  _initFrameCallbacks: [],
  PREF_BRANCH: "browser.panorama.",
  PREF_FIRST_RUN: "browser.panorama.experienced_first_run",
  PREF_STARTUP_PAGE: "browser.startup.page",
  PREF_RESTORE_ENABLED_ONCE: "browser.panorama.session_restore_enabled_once",
  GROUPS_IDENTIFIER: "tabview-groups",
  VISIBILITY_IDENTIFIER: "tabview-visibility",

  
  get windowTitle() {
    delete this.windowTitle;
    let brandBundle = document.getElementById("bundle_brand");
    let brandShortName = brandBundle.getString("brandShortName");
    let title = gNavigatorBundle.getFormattedString("tabview.title", [brandShortName]);
    return this.windowTitle = title;
  },

  
  get firstUseExperienced() {
    let pref = this.PREF_FIRST_RUN;
    if (Services.prefs.prefHasUserValue(pref))
      return Services.prefs.getBoolPref(pref);

    return false;
  },

  
  set firstUseExperienced(val) {
    Services.prefs.setBoolPref(this.PREF_FIRST_RUN, val);
  },

  
  get sessionRestoreEnabledOnce() {
    let pref = this.PREF_RESTORE_ENABLED_ONCE;
    if (Services.prefs.prefHasUserValue(pref))
      return Services.prefs.getBoolPref(pref);

    return false;
  },

  
  set sessionRestoreEnabledOnce(val) {
    Services.prefs.setBoolPref(this.PREF_RESTORE_ENABLED_ONCE, val);
  },

  
  init: function TabView_init() {
    
    goSetCommandEnabled("Browser:ToggleTabView", window.toolbar.visible);
    if (!window.toolbar.visible)
      return;

    if (this._initialized)
      return;

    if (this.firstUseExperienced) {
      

      let data = SessionStore.getWindowValue(window, this.VISIBILITY_IDENTIFIER);
      if (data && data == "true") {
        this.show();
      } else {
        try {
          data = SessionStore.getWindowValue(window, this.GROUPS_IDENTIFIER);
          if (data) {
            let parsedData = JSON.parse(data);
            this.updateGroupNumberBroadcaster(parsedData.totalNumber || 1);
          }
        } catch (e) { }

        let self = this;
        
        
        this._tabShowEventListener = function(event) {
          if (!self._window)
            self._initFrame(function() {
              self._window.UI.onTabSelect(gBrowser.selectedTab);
              if (self._closedLastVisibleTabBeforeFrameInitialized) {
                self._closedLastVisibleTabBeforeFrameInitialized = false;
                self._window.UI.showTabView(false);
              }
            });
        };
        this._tabCloseEventListener = function(event) {
          if (!self._window && gBrowser.visibleTabs.length == 0)
            self._closedLastVisibleTabBeforeFrameInitialized = true;
        };
        gBrowser.tabContainer.addEventListener(
          "TabShow", this._tabShowEventListener, false);
        gBrowser.tabContainer.addEventListener(
          "TabClose", this._tabCloseEventListener, false);

       if (this._tabBrowserHasHiddenTabs()) {
         this._setBrowserKeyHandlers();
       } else {
         
         this._SSWindowStateReadyListener = function (event) {
           if (this._tabBrowserHasHiddenTabs())
             this._setBrowserKeyHandlers();
         }.bind(this);
         window.addEventListener(
           "SSWindowStateReady", this._SSWindowStateReadyListener, false);
        }
      }
    }

    Services.prefs.addObserver(this.PREF_BRANCH, this, false);

    this._initialized = true;
  },

  
  
  observe: function TabView_observe(subject, topic, data) {
    if (data == this.PREF_FIRST_RUN && this.firstUseExperienced) {
      this._addToolbarButton();
      this.enableSessionRestore();
    }
  },

  
  
  uninit: function TabView_uninit() {
    if (!this._initialized)
      return;

    Services.prefs.removeObserver(this.PREF_BRANCH, this);

    if (this._tabShowEventListener)
      gBrowser.tabContainer.removeEventListener(
        "TabShow", this._tabShowEventListener, false);

    if (this._tabCloseEventListener)
      gBrowser.tabContainer.removeEventListener(
        "TabClose", this._tabCloseEventListener, false);

    if (this._SSWindowStateReadyListener)
      window.removeEventListener(
        "SSWindowStateReady", this._SSWindowStateReadyListener, false);

    this._initialized = false;

    if (this._window) {
      this._window = null;
    }

    if (this._iframe) {
      this._iframe.remove();
      this._iframe = null;
    }
  },

  
  
  
  _initFrame: function TabView__initFrame(callback) {
    let hasCallback = typeof callback == "function";

    
    if (!window.toolbar.visible)
      return;

    if (this._window) {
      if (hasCallback)
        callback();
      return;
    }

    if (hasCallback)
      this._initFrameCallbacks.push(callback);

    if (this._isFrameLoading)
      return;

    this._isFrameLoading = true;

    TelemetryStopwatch.start("PANORAMA_INITIALIZATION_TIME_MS");

    
    this._deck = document.getElementById("tab-view-deck");

    
    this._iframe = document.createElement("iframe");
    this._iframe.id = "tab-view";
    this._iframe.setAttribute("transparent", "true");
    this._iframe.setAttribute("tooltip", "tab-view-tooltip");
    this._iframe.flex = 1;

    let self = this;

    window.addEventListener("tabviewframeinitialized", function onInit() {
      window.removeEventListener("tabviewframeinitialized", onInit, false);

      TelemetryStopwatch.finish("PANORAMA_INITIALIZATION_TIME_MS");

      self._isFrameLoading = false;
      self._window = self._iframe.contentWindow;
      self._setBrowserKeyHandlers();

      if (self._tabShowEventListener) {
        gBrowser.tabContainer.removeEventListener(
          "TabShow", self._tabShowEventListener, false);
        self._tabShowEventListener = null;
      }
      if (self._tabCloseEventListener) {
        gBrowser.tabContainer.removeEventListener(
          "TabClose", self._tabCloseEventListener, false);
        self._tabCloseEventListener = null;
      }
      if (self._SSWindowStateReadyListener) {
        window.removeEventListener(
          "SSWindowStateReady", self._SSWindowStateReadyListener, false);
        self._SSWindowStateReadyListener = null;
      }

      self._initFrameCallbacks.forEach(function (cb) cb());
      self._initFrameCallbacks = [];
    }, false);

    this._iframe.setAttribute("src", "chrome://browser/content/tabview.html");
    this._deck.appendChild(this._iframe);

    
    let tooltip = document.createElement("tooltip");
    tooltip.id = "tab-view-tooltip";
    tooltip.setAttribute("onpopupshowing", "return TabView.fillInTooltip(document.tooltipNode);");
    document.getElementById("mainPopupSet").appendChild(tooltip);
  },

  
  getContentWindow: function TabView_getContentWindow() {
    return this._window;
  },

  
  isVisible: function TabView_isVisible() {
    return (this._deck ? this._deck.selectedPanel == this._iframe : false);
  },

  
  show: function TabView_show() {
    if (this.isVisible())
      return;

    let self = this;
    this._initFrame(function() {
      self._window.UI.showTabView(true);
    });
  },

  
  hide: function TabView_hide() {
    if (this.isVisible() && this._window) {
      this._window.UI.exit();
    }
  },

  
  toggle: function TabView_toggle() {
    if (this.isVisible())
      this.hide();
    else 
      this.show();
  },

  
  _tabBrowserHasHiddenTabs: function TabView_tabBrowserHasHiddenTabs() {
    return (gBrowser.tabs.length - gBrowser.visibleTabs.length) > 0;
  },

  
  updateContextMenu: function TabView_updateContextMenu(tab, popup) {
    let separator = document.getElementById("context_tabViewNamedGroups");
    let isEmpty = true;

    while (popup.firstChild && popup.firstChild != separator)
      popup.removeChild(popup.firstChild);

    let self = this;
    this._initFrame(function() {
      let activeGroup = tab._tabViewTabItem.parent;
      let groupItems = self._window.GroupItems.groupItems;

      groupItems.forEach(function(groupItem) {
        
        
        
        if (!groupItem.hidden &&
            (groupItem.getTitle().trim() || groupItem.getChildren().length) &&
            (!activeGroup || activeGroup.id != groupItem.id)) {
          let menuItem = self._createGroupMenuItem(groupItem);
          popup.insertBefore(menuItem, separator);
          isEmpty = false;
        }
      });
      separator.hidden = isEmpty;
    });
  },

  
  _createGroupMenuItem: function TabView__createGroupMenuItem(groupItem) {
    let menuItem = document.createElement("menuitem");
    let title = groupItem.getTitle();

    if (!title.trim()) {
      let topChildLabel = groupItem.getTopChild().tab.label;
      let childNum = groupItem.getChildren().length;

      if (childNum > 1) {
        let num = childNum - 1;
        title =
          gNavigatorBundle.getString("tabview.moveToUnnamedGroup.label");
        title = PluralForm.get(num, title).replace("#1", topChildLabel).replace("#2", num);
      } else {
        title = topChildLabel;
      }
    }

    menuItem.setAttribute("label", title);
    menuItem.setAttribute("tooltiptext", title);
    menuItem.setAttribute("crop", "center");
    menuItem.setAttribute("class", "tabview-menuitem");
    menuItem.setAttribute(
      "oncommand",
      "TabView.moveTabTo(TabContextMenu.contextTab,'" + groupItem.id + "')");

    return menuItem;
  },

  
  moveTabTo: function TabView_moveTabTo(tab, groupItemId) {
    if (this._window) {
      this._window.GroupItems.moveTabToGroupItem(tab, groupItemId);
    } else {
      let self = this;
      this._initFrame(function() {
        self._window.GroupItems.moveTabToGroupItem(tab, groupItemId);
      });
    }
  },

  
  
  
  _setBrowserKeyHandlers: function TabView__setBrowserKeyHandlers() {
    if (this._browserKeyHandlerInitialized)
      return;

    this._browserKeyHandlerInitialized = true;

    let self = this;
    window.addEventListener("keypress", function(event) {
      if (self.isVisible() || !self._tabBrowserHasHiddenTabs())
        return;

      let charCode = event.charCode;
      
      if (event.ctrlKey && !event.metaKey && !event.altKey &&
          (charCode == 96 || charCode == 126)) {
        event.stopPropagation();
        event.preventDefault();

        self._initFrame(function() {
          let groupItems = self._window.GroupItems;
          let tabItem = groupItems.getNextGroupItemTab(event.shiftKey);
          if (!tabItem)
            return;

          if (gBrowser.selectedTab.pinned)
            groupItems.updateActiveGroupItemAndTabBar(tabItem, {dontSetActiveTabInGroup: true});
          else
            gBrowser.selectedTab = tabItem.tab;
        });
      }
    }, true);
  },

  
  
  prepareUndoCloseTab: function TabView_prepareUndoCloseTab(blankTabToRemove) {
    if (this._window) {
      this._window.UI.restoredClosedTab = true;

      if (blankTabToRemove && blankTabToRemove._tabViewTabItem)
        blankTabToRemove._tabViewTabItem.isRemovedAfterRestore = true;
    }
  },

  
  
  afterUndoCloseTab: function TabView_afterUndoCloseTab() {
    if (this._window)
      this._window.UI.restoredClosedTab = false;
  },

  
  
  moveToGroupPopupShowing: function TabView_moveToGroupPopupShowing(event) {
    
    
    let numHiddenTabs = gBrowser.tabs.length - gBrowser.visibleTabs.length;
    if (this._window || numHiddenTabs > 0)
      this.updateContextMenu(TabContextMenu.contextTab, event.target);
  },

  
  
  
  _addToolbarButton: function TabView__addToolbarButton() {
    let buttonId = "tabview-button";

    if (CustomizableUI.getPlacementOfWidget(buttonId))
      return;

    let allTabsBtnPlacement = CustomizableUI.getPlacementOfWidget("alltabs-button");
    
    let desiredPosition = allTabsBtnPlacement.position + 1;
    CustomizableUI.addWidgetToArea(buttonId, "TabsToolbar", desiredPosition);
    
    
    document.persist("TabsToolbar", "currentset");
  },

  
  
  
  updateGroupNumberBroadcaster: function TabView_updateGroupNumberBroadcaster(number) {
    let groupsNumber = document.getElementById("tabviewGroupsNumber");
    groupsNumber.setAttribute("groups", number);
  },

  
  
  
  
  enableSessionRestore: function TabView_enableSessionRestore() {
    if (!this._window || !this.firstUseExperienced)
      return;

    
    if (this.sessionRestoreEnabledOnce)
      return;

    this.sessionRestoreEnabledOnce = true;

    
    if (Services.prefs.getIntPref(this.PREF_STARTUP_PAGE) != 3) {
      Services.prefs.setIntPref(this.PREF_STARTUP_PAGE, 3);

      
      this._window.UI.notifySessionRestoreEnabled();
    }
  },

  
  
  
  fillInTooltip: function fillInTooltip(tipElement) {
    let retVal = false;
    let titleText = null;
    let direction = tipElement.ownerDocument.dir;

    while (!titleText && tipElement) {
      if (tipElement.nodeType == Node.ELEMENT_NODE)
        titleText = tipElement.getAttribute("title");
      tipElement = tipElement.parentNode;
    }
    let tipNode = document.getElementById("tab-view-tooltip");
    tipNode.style.direction = direction;

    if (titleText) {
      tipNode.setAttribute("label", titleText);
      retVal = true;
    }

    return retVal;
  }
};
