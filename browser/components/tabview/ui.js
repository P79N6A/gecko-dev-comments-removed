














































let Keys = { meta: false };




let UI = {
  
  
  
  DBLCLICK_INTERVAL: 500,

  
  
  
  DBLCLICK_OFFSET: 5,

  
  
  _frameInitialized: false,

  
  
  _pageBounds: null,

  
  
  _closedLastVisibleTab: false,

  
  
  _closedSelectedTabInTabView: false,

  
  
  restoredClosedTab: false,

  
  
  _isChangingVisibility: false,

  
  
  
  _reorderTabItemsOnShow: [],

  
  
  
  _reorderTabsOnHide: [],

  
  
  
  _currentTab: null,

  
  
  
  _lastClick: 0,

  
  
  _eventListeners: {},

  
  
  _cleanupFunctions: [],
  
  
  
  
  
  _maxInteractiveWait: 250,

  
  
  
  
  _privateBrowsing: {
    transitionMode: "",
    wasInTabView: false 
  },
  
  
  
  _storageBusy: false,

  
  
  isDOMWindowClosing: false,

  
  
  _browserKeys: null,

  
  
  _browserKeysWithShift: null,

  
  
  ignoreKeypressForSearch: false,

  
  
  _lastOpenedTab: null,

  
  
  _originalSmoothScroll: null,

  
  
  
  toString: function UI_toString() {
    return "[UI]";
  },

  
  
  
  init: function UI_init() {
    try {
      let self = this;

      
      this._initPageDirection();

      
      ThumbnailStorage.init();

      
      Storage.init();

      
      StoragePolicy.init();

      if (Storage.readWindowBusyState(gWindow))
        this.storageBusy();

      let data = Storage.readUIData(gWindow);
      this._storageSanity(data);
      this._pageBounds = data.pageBounds;

      
      Search.init();

      
      this._currentTab = gBrowser.selectedTab;

      
      iQ("#exit-button").click(function() {
        self.exit();
        self.blurAll();
      })
      .attr("title", tabviewString("button.exitTabGroups"));

      
      
      iQ(gTabViewFrame.contentDocument).mousedown(function(e) {
        if (iQ(":focus").length > 0) {
          iQ(":focus").each(function(element) {
            
            if (e.target != element && element.nodeName == "INPUT")
              element.blur();
          });
        }
        if (e.originalTarget.id == "content") {
          if (!Utils.isLeftClick(e)) {
            self._lastClick = 0;
            self._lastClickPositions = null;
          } else {
            
            if (Date.now() - self._lastClick <= self.DBLCLICK_INTERVAL && 
                (self._lastClickPositions.x - self.DBLCLICK_OFFSET) <= e.clientX &&
                (self._lastClickPositions.x + self.DBLCLICK_OFFSET) >= e.clientX &&
                (self._lastClickPositions.y - self.DBLCLICK_OFFSET) <= e.clientY &&
                (self._lastClickPositions.y + self.DBLCLICK_OFFSET) >= e.clientY) {

              let box =
                new Rect(e.clientX - Math.floor(TabItems.tabWidth/2),
                         e.clientY - Math.floor(TabItems.tabHeight/2),
                         TabItems.tabWidth, TabItems.tabHeight);
              box.inset(-30, -30);

              let opts = {immediately: true, bounds: box};
              let groupItem = new GroupItem([], opts);
              groupItem.newTab();

              self._lastClick = 0;
              self._lastClickPositions = null;
              gTabView.firstUseExperienced = true;
            } else {
              self._lastClick = Date.now();
              self._lastClickPositions = new Point(e.clientX, e.clientY);
              self._createGroupItemOnDrag(e);
            }
          }
        }
      });

      iQ(window).bind("unload", function() {
        self.uninit();
      });

      
      let mm = gWindow.messageManager;
      let callback = this._onDOMWillOpenModalDialog.bind(this);
      mm.addMessageListener("Panorama:DOMWillOpenModalDialog", callback);

      this._cleanupFunctions.push(function () {
        mm.removeMessageListener("Panorama:DOMWillOpenModalDialog", callback);
      });

      
      this._setTabViewFrameKeyHandlers();

      
      this._addTabActionHandlers();

      
      GroupItems.init();
      GroupItems.pauseArrange();
      let hasGroupItemsData = GroupItems.load();

      
      TabItems.init();
      TabItems.pausePainting();

      if (!hasGroupItemsData)
        this.reset();

      
      if (this._pageBounds)
        this._resize(true);
      else
        this._pageBounds = Items.getPageBounds();

      iQ(window).resize(function() {
        self._resize();
      });

      
      gWindow.addEventListener("SSWindowClosing", function onWindowClosing() {
        gWindow.removeEventListener("SSWindowClosing", onWindowClosing, false);

        
        self.isDOMWindowClosing = true;

        if (self.isTabViewVisible())
          GroupItems.removeHiddenGroups();

        TabItems.saveAll();
        TabItems.saveAllThumbnails({synchronously: true});

        self._save();
      }, false);

      
      let frameScript = "chrome://browser/content/tabview-content.js";
      gWindow.messageManager.loadFrameScript(frameScript, true);

      
      this._frameInitialized = true;
      this._save();

      
      
      let event = document.createEvent("Events");
      event.initEvent("tabviewframeinitialized", true, false);
      dispatchEvent(event);
    } catch(e) {
      Utils.log(e);
    } finally {
      GroupItems.resumeArrange();
    }
  },

  
  
  uninit: function UI_uninit() {
    
    this._cleanupFunctions.forEach(function(func) {
      func();
    });
    this._cleanupFunctions = [];

    
    TabItems.uninit();
    GroupItems.uninit();
    Storage.uninit();
    StoragePolicy.uninit();

    this._removeTabActionHandlers();
    this._currentTab = null;
    this._pageBounds = null;
    this._reorderTabItemsOnShow = null;
    this._reorderTabsOnHide = null;
    this._frameInitialized = false;
  },

  
  
  rtl: false,

  
  
  reset: function UI_reset() {
    let padding = Trenches.defaultRadius;
    let welcomeWidth = 300;
    let pageBounds = Items.getPageBounds();
    pageBounds.inset(padding, padding);

    let $actions = iQ("#actions");
    if ($actions) {
      pageBounds.width -= $actions.width();
      if (UI.rtl)
        pageBounds.left += $actions.width() - padding;
    }

    
    let box = new Rect(pageBounds);
    box.width = Math.min(box.width * 0.667,
                         pageBounds.width - (welcomeWidth + padding));
    box.height = box.height * 0.667;
    if (UI.rtl) {
      box.left = pageBounds.left + welcomeWidth + 2 * padding;
    }

    GroupItems.groupItems.forEach(function(group) {
      group.close();
    });
    
    let options = {
      bounds: box,
      immediately: true
    };
    let groupItem = new GroupItem([], options);
    let items = TabItems.getItems();
    items.forEach(function(item) {
      if (item.parent)
        item.parent.remove(item);
      groupItem.add(item, {immediately: true});
    });
    this.setActive(groupItem);
  },

  
  
  
  blurAll: function UI_blurAll() {
    iQ(":focus").each(function(element) {
      element.blur();
    });
  },

  
  
  
  
  
  
  isIdle: function UI_isIdle() {
    let time = Date.now();
    let maxEvent = Math.max(drag.lastMoveTime, resize.lastMoveTime);
    return (time - maxEvent) > this._maxInteractiveWait;
  },

  
  
  
  getActiveTab: function UI_getActiveTab() {
    return this._activeTab;
  },

  
  
  
  
  
  
  
  
  
  _setActiveTab: function UI__setActiveTab(tabItem) {
    if (tabItem == this._activeTab)
      return;

    if (this._activeTab) {
      this._activeTab.makeDeactive();
      this._activeTab.removeSubscriber("close", this._onActiveTabClosed);
    }

    this._activeTab = tabItem;

    if (this._activeTab) {
      this._activeTab.addSubscriber("close", this._onActiveTabClosed);
      this._activeTab.makeActive();
    }
  },

  
  
  
  
  
  
  _onActiveTabClosed: function UI__onActiveTabClosed(tabItem){
    if (UI._activeTab == tabItem)
      UI._setActiveTab(null);
  },

  
  
  
  
  
  
  
  setActive: function UI_setActive(item, options) {
    Utils.assert(item, "item must be given");

    if (item.isATabItem) {
      if (item.parent)
        GroupItems.setActiveGroupItem(item.parent);
      if (!options || !options.dontSetActiveTabInGroup)
        this._setActiveTab(item);
    } else {
      GroupItems.setActiveGroupItem(item);
      if (!options || !options.dontSetActiveTabInGroup) {
        let activeTab = item.getActiveTab()
        if (activeTab)
          this._setActiveTab(activeTab);
      }
    }
  },

  
  
  
  clearActiveTab: function UI_clearActiveTab() {
    this._setActiveTab(null);
  },

  
  
  
  isTabViewVisible: function UI_isTabViewVisible() {
    return gTabViewDeck.selectedPanel == gTabViewFrame;
  },

  
  
  
  _initPageDirection: function UI__initPageDirection() {
    let chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].
                    getService(Ci.nsIXULChromeRegistry);
    let dir = chromeReg.isLocaleRTL("global");
    document.documentElement.setAttribute("dir", dir ? "rtl" : "ltr");
    this.rtl = dir;
  },

  
  
  
  
  
  showTabView: function UI_showTabView(zoomOut) {
    if (this.isTabViewVisible() || this._isChangingVisibility)
      return;

    this._isChangingVisibility = true;

    
    let tabStrip = gBrowser.tabContainer.mTabstrip;
    this._originalSmoothScroll = tabStrip.smoothScroll;
    tabStrip.smoothScroll = false;

    
    this._initPageDirection();

    var self = this;
    var currentTab = this._currentTab;

    this._reorderTabItemsOnShow.forEach(function(groupItem) {
      groupItem.reorderTabItemsBasedOnTabOrder();
    });
    this._reorderTabItemsOnShow = [];

#ifdef XP_WIN
    
    gTabViewFrame.style.marginTop = "";
#endif
    gTabViewDeck.selectedPanel = gTabViewFrame;
    gWindow.TabsInTitlebar.allowedBy("tabview-open", false);
    gTabViewFrame.contentWindow.focus();

    gBrowser.updateTitlebar();
#ifdef XP_MACOSX
    this.setTitlebarColors(true);
#endif
    let event = document.createEvent("Events");
    event.initEvent("tabviewshown", true, false);

    Storage.saveVisibilityData(gWindow, "true");

    if (zoomOut && currentTab && currentTab._tabViewTabItem) {
      let item = currentTab._tabViewTabItem;
      
      
      

      
      item.zoomOut(function() {
        if (!currentTab._tabViewTabItem) 
          item = null;

        self.setActive(item);

        self._resize(true);
        self._isChangingVisibility = false;
        dispatchEvent(event);

        
        GroupItems.flushAppTabUpdates();

        TabItems.resumePainting();
      });
    } else {
      self.clearActiveTab();
      self._isChangingVisibility = false;
      dispatchEvent(event);

      
      GroupItems.flushAppTabUpdates();

      TabItems.resumePainting();
    }

    if (gTabView.firstUseExperienced)
      gTabView.enableSessionRestore();
  },

  
  
  
  hideTabView: function UI_hideTabView() {
    if (!this.isTabViewVisible() || this._isChangingVisibility)
      return;

    this._isChangingVisibility = true;

    
    
    GroupItems.removeHiddenGroups();
    TabItems.pausePainting();

    this._reorderTabsOnHide.forEach(function(groupItem) {
      groupItem.reorderTabsBasedOnTabItemOrder();
    });
    this._reorderTabsOnHide = [];

#ifdef XP_WIN
    
    
    
    gTabViewFrame.style.marginTop = gBrowser.boxObject.y + "px";
#endif
    gTabViewDeck.selectedPanel = gBrowserPanel;
    gWindow.TabsInTitlebar.allowedBy("tabview-open", true);
    gBrowser.selectedBrowser.focus();

    gBrowser.updateTitlebar();
    gBrowser.tabContainer.mTabstrip.smoothScroll = this._originalSmoothScroll;
#ifdef XP_MACOSX
    this.setTitlebarColors(false);
#endif
    Storage.saveVisibilityData(gWindow, "false");

    this._isChangingVisibility = false;

    let event = document.createEvent("Events");
    event.initEvent("tabviewhidden", true, false);
    dispatchEvent(event);
  },

#ifdef XP_MACOSX
  
  
  
  
  
  
  
  
  
  setTitlebarColors: function UI_setTitlebarColors(colors) {
    
    var mainWindow = gWindow.document.getElementById("main-window");
    if (colors === true) {
      mainWindow.setAttribute("activetitlebarcolor", "#C4C4C4");
      mainWindow.setAttribute("inactivetitlebarcolor", "#EDEDED");
    } else if (colors && "active" in colors && "inactive" in colors) {
      mainWindow.setAttribute("activetitlebarcolor", colors.active);
      mainWindow.setAttribute("inactivetitlebarcolor", colors.inactive);
    } else {
      mainWindow.removeAttribute("activetitlebarcolor");
      mainWindow.removeAttribute("inactivetitlebarcolor");
    }
  },
#endif

  
  
  
  
  storageBusy: function UI_storageBusy() {
    if (this._storageBusy)
      return;

    this._storageBusy = true;

    TabItems.pauseReconnecting();
    GroupItems.pauseAutoclose();
  },
  
  
  
  
  
  storageReady: function UI_storageReady() {
    if (!this._storageBusy)
      return;

    this._storageBusy = false;

    let hasGroupItemsData = GroupItems.load();
    if (!hasGroupItemsData)
      this.reset();

    TabItems.resumeReconnecting();
    GroupItems._updateTabBar();
    GroupItems.resumeAutoclose();
  },

  
  
  
  _addTabActionHandlers: function UI__addTabActionHandlers() {
    var self = this;

    
    function handleSSWindowStateBusy() {
      self.storageBusy();
    }
    
    function handleSSWindowStateReady() {
      self.storageReady();
    }
    
    gWindow.addEventListener("SSWindowStateBusy", handleSSWindowStateBusy, false);
    gWindow.addEventListener("SSWindowStateReady", handleSSWindowStateReady, false);

    this._cleanupFunctions.push(function() {
      gWindow.removeEventListener("SSWindowStateBusy", handleSSWindowStateBusy, false);
      gWindow.removeEventListener("SSWindowStateReady", handleSSWindowStateReady, false);
    });

    
    
    
    
    
    
    function pbObserver(subject, topic, data) {
      if (topic == "private-browsing") {
        
        
        if (data == "enter") {
          
          self._privateBrowsing.wasInTabView = self.isTabViewVisible();
          if (self.isTabViewVisible())
            self.goToTab(gBrowser.selectedTab);
        }
      } else if (topic == "private-browsing-change-granted") {
        if (data == "enter" || data == "exit") {
          Search.hide();
          self._privateBrowsing.transitionMode = data;

          
          
          if (data == "enter")
            TabItems.saveAllThumbnails({synchronously: true});
        }
      } else if (topic == "private-browsing-transition-complete") {
        
        if (self._privateBrowsing.transitionMode == "exit" &&
            self._privateBrowsing.wasInTabView)
          self.showTabView(false);

        self._privateBrowsing.transitionMode = "";
      }
    }

    Services.obs.addObserver(pbObserver, "private-browsing", false);
    Services.obs.addObserver(pbObserver, "private-browsing-change-granted", false);
    Services.obs.addObserver(pbObserver, "private-browsing-transition-complete", false);

    this._cleanupFunctions.push(function() {
      Services.obs.removeObserver(pbObserver, "private-browsing");
      Services.obs.removeObserver(pbObserver, "private-browsing-change-granted");
      Services.obs.removeObserver(pbObserver, "private-browsing-transition-complete");
    });

    
    this._eventListeners.open = function (event) {
      let tab = event.target;

      
      if (tab.pinned)
        GroupItems.addAppTab(tab);
      else if (self.isTabViewVisible() && !self._storageBusyCount)
        self._lastOpenedTab = tab;
    };
    
    
    this._eventListeners.close = function (event) {
      let tab = event.target;

      
      if (tab.pinned)
        GroupItems.removeAppTab(tab);
        
      if (self.isTabViewVisible()) {
        
        if (self._currentTab == tab)
          self._closedSelectedTabInTabView = true;
      } else {
        
        
        if (self._storageBusy)
          return;

        
        if (gBrowser.tabs.length > 1) {
          
          for (let a = 0; a < gBrowser._numPinnedTabs; a++) {
            if (!gBrowser.tabs[a].closing)
              return;
          }

          let groupItem = GroupItems.getActiveGroupItem();

          
          
          let closingLastOfGroup = (groupItem && 
              groupItem._children.length == 1 && 
              groupItem._children[0].tab == tab);

          
          
          
          let tabItem = tab && tab._tabViewTabItem;
          let closingBlankTabAfterRestore =
            (tabItem && tabItem.isRemovedAfterRestore);

          if (closingLastOfGroup && !closingBlankTabAfterRestore) {
            
            self._closedLastVisibleTab = true;
            self.showTabView();
          }
        }
      }
    };

    
    this._eventListeners.move = function (event) {
      let tab = event.target;

      if (GroupItems.groupItems.length > 0) {
        if (tab.pinned) {
          if (gBrowser._numPinnedTabs > 1)
            GroupItems.arrangeAppTab(tab);
        } else {
          let activeGroupItem = GroupItems.getActiveGroupItem();
          if (activeGroupItem)
            self.setReorderTabItemsOnShow(activeGroupItem);
        }
      }
    };

    
    this._eventListeners.select = function (event) {
      self.onTabSelect(event.target);
    };

    
    this._eventListeners.pinned = function (event) {
      let tab = event.target;

      TabItems.handleTabPin(tab);
      GroupItems.addAppTab(tab);
    };

    
    this._eventListeners.unpinned = function (event) {
      let tab = event.target;

      TabItems.handleTabUnpin(tab);
      GroupItems.removeAppTab(tab);

      let groupItem = tab._tabViewTabItem.parent;
      if (groupItem)
        self.setReorderTabItemsOnShow(groupItem);
    };

    
    for (let name in this._eventListeners)
      AllTabs.register(name, this._eventListeners[name]);
  },

  
  
  
  _removeTabActionHandlers: function UI__removeTabActionHandlers() {
    for (let name in this._eventListeners)
      AllTabs.unregister(name, this._eventListeners[name]);
  },

  
  
  
  goToTab: function UI_goToTab(xulTab) {
    
    if (gBrowser.selectedTab == xulTab)
      this.onTabSelect(xulTab);
    else
      gBrowser.selectedTab = xulTab;
  },

  
  
  
  onTabSelect: function UI_onTabSelect(tab) {
    this._currentTab = tab;

    if (this.isTabViewVisible()) {
      
      
      
      
      if (!this.restoredClosedTab && !this._privateBrowsing.transitionMode &&
          this._lastOpenedTab == tab && tab._tabViewTabItem) {
        tab._tabViewTabItem.zoomIn(true);
        this._lastOpenedTab = null;
        return;
      }
      if (this._closedLastVisibleTab ||
          (this._closedSelectedTabInTabView && !this.closedLastTabInTabView) ||
          this.restoredClosedTab) {
        if (this.restoredClosedTab) {
          
          
          tab.linkedBrowser.addEventListener("load", function onLoad(event) {
            tab.linkedBrowser.removeEventListener("load", onLoad, true);
            TabItems._update(tab);
          }, true);
        }
        this._closedLastVisibleTab = false;
        this._closedSelectedTabInTabView = false;
        this.closedLastTabInTabView = false;
        this.restoredClosedTab = false;
        return;
      }
    }
    
    this._closedLastVisibleTab = false;
    this._closedSelectedTabInTabView = false;
    this.closedLastTabInTabView = false;
    this.restoredClosedTab = false;
    this._lastOpenedTab = null;

    
    
    if (this.isTabViewVisible()) {
      
      if (tab && tab._tabViewTabItem && tab._tabViewTabItem.parent &&
          tab._tabViewTabItem.parent.hidden)
        tab._tabViewTabItem.parent._unhide({immediately: true});

      this.hideTabView();
    }

    
    
    if (this._currentTab != tab)
      return;

    let newItem = null;
    
    if (tab && tab._tabViewTabItem) {
      if (!TabItems.reconnectingPaused()) {
        newItem = tab._tabViewTabItem;
        GroupItems.updateActiveGroupItemAndTabBar(newItem);
      }
    } else {
      
      
      
      if (!GroupItems.getActiveGroupItem()) {
        for (let a = 0; a < gBrowser.tabs.length; a++) {
          let theTab = gBrowser.tabs[a];
          if (!theTab.pinned) {
            let tabItem = theTab._tabViewTabItem;
            this.setActive(tabItem.parent);
            break;
          }
        }
      }

      if (GroupItems.getActiveGroupItem())
        GroupItems._updateTabBar();
    }
  },

  
  
  
  _onDOMWillOpenModalDialog: function UI__onDOMWillOpenModalDialog(cx) {
    if (!this.isTabViewVisible())
      return;

    let index = gBrowser.browsers.indexOf(cx.target);
    if (index == -1)
      return;

    let tab = gBrowser.tabs[index];

    
    
    
    
    if (gBrowser.selectedTab == tab && this._currentTab == tab)
      this.onTabSelect(tab);
  },

  
  
  
  
  
  
  setReorderTabsOnHide: function UI_setReorderTabsOnHide(groupItem) {
    if (this.isTabViewVisible()) {
      var index = this._reorderTabsOnHide.indexOf(groupItem);
      if (index == -1)
        this._reorderTabsOnHide.push(groupItem);
    }
  },

  
  
  
  
  
  
  setReorderTabItemsOnShow: function UI_setReorderTabItemsOnShow(groupItem) {
    if (!this.isTabViewVisible()) {
      var index = this._reorderTabItemsOnShow.indexOf(groupItem);
      if (index == -1)
        this._reorderTabItemsOnShow.push(groupItem);
    }
  },
  
  
  updateTabButton: function UI_updateTabButton() {
    let exitButton = document.getElementById("exit-button");
    let numberOfGroups = GroupItems.groupItems.length;

    exitButton.setAttribute("groups", numberOfGroups);
    gTabView.updateGroupNumberBroadcaster(numberOfGroups);
  },

  
  
  
  getClosestTab: function UI_getClosestTab(tabCenter) {
    let cl = null;
    let clDist;
    TabItems.getItems().forEach(function (item) {
      if (!item.parent || item.parent.hidden)
        return;
      let testDist = tabCenter.distance(item.bounds.center());
      if (cl==null || testDist < clDist) {
        cl = item;
        clDist = testDist;
      }
    });
    return cl;
  },

  
  
  
  _setupBrowserKeys: function UI__setupKeyWhiteList() {
    let keys = {};

    [
#ifdef XP_UNIX
      "quitApplication",
#else
      "redo",
#endif
#ifdef XP_MACOSX
      "preferencesCmdMac", "minimizeWindow", "hideThisAppCmdMac",
#endif
      "newNavigator", "newNavigatorTab", "undo", "cut", "copy", "paste", 
      "selectAll", "find"
    ].forEach(function(key) {
      let element = gWindow.document.getElementById("key_" + key);
      let code = element.getAttribute("key").toLocaleLowerCase().charCodeAt(0);
      keys[code] = key;
    });
    this._browserKeys = keys;

    keys = {};
    
    
    
    [
#ifdef XP_UNIX
      "redo",
#endif
#ifdef XP_MACOSX
      "fullScreen",
#endif
      "closeWindow", "tabview", "undoCloseTab", "undoCloseWindow",
      "privatebrowsing"
    ].forEach(function(key) {
      let element = gWindow.document.getElementById("key_" + key);
      let code = element.getAttribute("key").toLocaleLowerCase().charCodeAt(0);
      keys[code] = key;
    });
    this._browserKeysWithShift = keys;
  },

  
  
  
  _setTabViewFrameKeyHandlers: function UI__setTabViewFrameKeyHandlers() {
    let self = this;

    this._setupBrowserKeys();

    iQ(window).keyup(function(event) {
      if (!event.metaKey)
        Keys.meta = false;
    });

    iQ(window).keypress(function(event) {
      if (event.metaKey)
        Keys.meta = true;

      function processBrowserKeys(evt) {
        
        if (evt.altKey)
          return;

#ifdef XP_MACOSX
        if (evt.metaKey) {
#else
        if (evt.ctrlKey) {
#endif
          let preventDefault = true;
          if (evt.shiftKey) {
            
            
            
            let lowercaseCharCode =
              String.fromCharCode(evt.charCode).toLocaleLowerCase().charCodeAt(0);
            if (lowercaseCharCode in self._browserKeysWithShift) {
              let key = self._browserKeysWithShift[lowercaseCharCode];
              if (key == "tabview")
                self.exit();
              else
                preventDefault = false;
            }
          } else {
            if (evt.charCode in self._browserKeys) {
              let key = self._browserKeys[evt.charCode];
              if (key == "find")
                self.enableSearch();
              else
                preventDefault = false;
            }
          }
          if (preventDefault) {
            evt.stopPropagation();
            evt.preventDefault();
          }
        }
      }
      if ((iQ(":focus").length > 0 && iQ(":focus")[0].nodeName == "INPUT") ||
          Search.isEnabled() || self.ignoreKeypressForSearch) {
        self.ignoreKeypressForSearch = false;
        processBrowserKeys(event);
        return;
      }

      function getClosestTabBy(norm) {
        if (!self.getActiveTab())
          return null;

        let activeTab = self.getActiveTab();
        let activeTabGroup = activeTab.parent;
        let myCenter = activeTab.bounds.center();
        let match;

        TabItems.getItems().forEach(function (item) {
          if (!item.parent.hidden &&
              (!activeTabGroup.expanded || activeTabGroup.id == item.parent.id)) {
            let itemCenter = item.bounds.center();

            if (norm(itemCenter, myCenter)) {
              let itemDist = myCenter.distance(itemCenter);
              if (!match || match[0] > itemDist)
                match = [itemDist, item];
            }
          }
        });

        return match && match[1];
      }

      let preventDefault = true;
      let activeTab;
      let norm = null;
      switch (event.keyCode) {
        case KeyEvent.DOM_VK_RIGHT:
          norm = function(a, me){return a.x > me.x};
          break;
        case KeyEvent.DOM_VK_LEFT:
          norm = function(a, me){return a.x < me.x};
          break;
        case KeyEvent.DOM_VK_DOWN:
          norm = function(a, me){return a.y > me.y};
          break;
        case KeyEvent.DOM_VK_UP:
          norm = function(a, me){return a.y < me.y}
          break;
      }

      if (norm != null) {
        var nextTab = getClosestTabBy(norm);
        if (nextTab) {
          if (nextTab.isStacked && !nextTab.parent.expanded)
            nextTab = nextTab.parent.getChild(0);
          self.setActive(nextTab);
        }
      } else {
        switch(event.keyCode) {
          case KeyEvent.DOM_VK_ESCAPE:
            let activeGroupItem = GroupItems.getActiveGroupItem();
            if (activeGroupItem && activeGroupItem.expanded)
              activeGroupItem.collapse();
            else
              self.exit();
            break;
          case KeyEvent.DOM_VK_RETURN:
          case KeyEvent.DOM_VK_ENTER:
            activeTab = self.getActiveTab();
            if (activeTab)
              activeTab.zoomIn();
            break;
          case KeyEvent.DOM_VK_TAB:
            
            activeTab = self.getActiveTab();
            if (activeTab) {
              let tabItems = (activeTab.parent ? activeTab.parent.getChildren() :
                              [activeTab]);
              let length = tabItems.length;
              let currentIndex = tabItems.indexOf(activeTab);

              if (length > 1) {
                let newIndex;
                if (event.shiftKey) {
                  if (currentIndex == 0)
                    newIndex = (length - 1);
                  else
                    newIndex = (currentIndex - 1);
                } else {
                  if (currentIndex == (length - 1))
                    newIndex = 0;
                  else
                    newIndex = (currentIndex + 1);
                }
                self.setActive(tabItems[newIndex]);
              }
            }
            break;
          default:
            processBrowserKeys(event);
            preventDefault = false;
        }
        if (preventDefault) {
          event.stopPropagation();
          event.preventDefault();
        }
      }
    });
  },

  
  
  
  enableSearch: function UI_enableSearch() {
    if (!Search.isEnabled()) {
      Search.ensureShown();
      Search.switchToInMode();
    }
  },

  
  
  
  
  _createGroupItemOnDrag: function UI__createGroupItemOnDrag(e) {
    const minSize = 60;
    const minMinSize = 15;

    let lastActiveGroupItem = GroupItems.getActiveGroupItem();

    var startPos = { x: e.clientX, y: e.clientY };
    var phantom = iQ("<div>")
      .addClass("groupItem phantom activeGroupItem dragRegion")
      .css({
        position: "absolute",
        zIndex: -1,
        cursor: "default"
      })
      .appendTo("body");

    var item = { 
      container: phantom,
      isAFauxItem: true,
      bounds: {},
      getBounds: function FauxItem_getBounds() {
        return this.container.bounds();
      },
      setBounds: function FauxItem_setBounds(bounds) {
        this.container.css(bounds);
      },
      setZ: function FauxItem_setZ(z) {
        
      },
      setOpacity: function FauxItem_setOpacity(opacity) {
        this.container.css("opacity", opacity);
      },
      
      
      pushAway: function () {},
    };
    item.setBounds(new Rect(startPos.y, startPos.x, 0, 0));

    var dragOutInfo = new Drag(item, e);

    function updateSize(e) {
      var box = new Rect();
      box.left = Math.min(startPos.x, e.clientX);
      box.right = Math.max(startPos.x, e.clientX);
      box.top = Math.min(startPos.y, e.clientY);
      box.bottom = Math.max(startPos.y, e.clientY);
      item.setBounds(box);

      
      var stationaryCorner = "";

      if (startPos.y == box.top)
        stationaryCorner += "top";
      else
        stationaryCorner += "bottom";

      if (startPos.x == box.left)
        stationaryCorner += "left";
      else
        stationaryCorner += "right";

      dragOutInfo.snap(stationaryCorner, false, false); 

      box = item.getBounds();
      if (box.width > minMinSize && box.height > minMinSize &&
         (box.width > minSize || box.height > minSize))
        item.setOpacity(1);
      else
        item.setOpacity(0.7);

      e.preventDefault();
    }

    let self = this;
    function collapse() {
      let center = phantom.bounds().center();
      phantom.animate({
        width: 0,
        height: 0,
        top: center.y,
        left: center.x
      }, {
        duration: 300,
        complete: function() {
          phantom.remove();
        }
      });
      self.setActive(lastActiveGroupItem);
    }

    function finalize(e) {
      iQ(window).unbind("mousemove", updateSize);
      item.container.removeClass("dragRegion");
      dragOutInfo.stop();
      let box = item.getBounds();
      if (box.width > minMinSize && box.height > minMinSize &&
         (box.width > minSize || box.height > minSize)) {
        let opts = {bounds: item.getBounds(), focusTitle: true};
        let groupItem = new GroupItem([], opts);
        self.setActive(groupItem);
        phantom.remove();
        dragOutInfo = null;
        gTabView.firstUseExperienced = true;
      } else {
        collapse();
      }
    }

    iQ(window).mousemove(updateSize)
    iQ(gWindow).one("mouseup", finalize);
    e.preventDefault();
    return false;
  },

  
  
  
  
  
  
  _resize: function UI__resize(force) {
    if (!this._pageBounds)
      return;

    
    
    
    
    if (!force && !this.isTabViewVisible())
      return;

    let oldPageBounds = new Rect(this._pageBounds);
    let newPageBounds = Items.getPageBounds();
    if (newPageBounds.equals(oldPageBounds))
      return;

    if (!this.shouldResizeItems())
      return;

    var items = Items.getTopLevelItems();

    
    var itemBounds = new Rect(this._pageBounds);
    
    
    itemBounds.width = 1;
    itemBounds.height = 1;
    items.forEach(function(item) {
      var bounds = item.getBounds();
      itemBounds = (itemBounds ? itemBounds.union(bounds) : new Rect(bounds));
    });

    if (newPageBounds.width < this._pageBounds.width &&
        newPageBounds.width > itemBounds.width)
      newPageBounds.width = this._pageBounds.width;

    if (newPageBounds.height < this._pageBounds.height &&
        newPageBounds.height > itemBounds.height)
      newPageBounds.height = this._pageBounds.height;

    var wScale;
    var hScale;
    if (Math.abs(newPageBounds.width - this._pageBounds.width)
         > Math.abs(newPageBounds.height - this._pageBounds.height)) {
      wScale = newPageBounds.width / this._pageBounds.width;
      hScale = newPageBounds.height / itemBounds.height;
    } else {
      wScale = newPageBounds.width / itemBounds.width;
      hScale = newPageBounds.height / this._pageBounds.height;
    }

    var scale = Math.min(hScale, wScale);
    var self = this;
    var pairs = [];
    items.forEach(function(item) {
      var bounds = item.getBounds();
      bounds.left += (UI.rtl ? -1 : 1) * (newPageBounds.left - self._pageBounds.left);
      bounds.left *= scale;
      bounds.width *= scale;

      bounds.top += newPageBounds.top - self._pageBounds.top;
      bounds.top *= scale;
      bounds.height *= scale;

      pairs.push({
        item: item,
        bounds: bounds
      });
    });

    Items.unsquish(pairs);

    pairs.forEach(function(pair) {
      pair.item.setBounds(pair.bounds, true);
      pair.item.snap();
    });

    this._pageBounds = Items.getPageBounds();
    this._save();
  },
  
  
  
  
  
  
  
  
  shouldResizeItems: function UI_shouldResizeItems() {
    let newPageBounds = Items.getPageBounds();
    
    
    if (this._minimalRect === undefined || this._feelsCramped === undefined) {

      
      
      
      let feelsCramped = false;
      let minimalRect = new Rect(0, 0, 1, 1);
      
      Items.getTopLevelItems()
        .forEach(function UI_shouldResizeItems_checkItem(item) {
          let bounds = new Rect(item.getBounds());
          feelsCramped = feelsCramped || (item.userSize &&
            (item.userSize.x > bounds.width || item.userSize.y > bounds.height));
          bounds.inset(-Trenches.defaultRadius, -Trenches.defaultRadius);
          minimalRect = minimalRect.union(bounds);
        });
      
      
      minimalRect.left = 0;
      minimalRect.top  = 0;
  
      this._minimalRect = minimalRect;
      this._feelsCramped = feelsCramped;
    }

    return this._minimalRect.width > newPageBounds.width ||
      this._minimalRect.height > newPageBounds.height ||
      this._feelsCramped;
  },
  
  
  
  
  
  
  clearShouldResizeItems: function UI_clearShouldResizeItems() {
    delete this._minimalRect;
    delete this._feelsCramped;
  },

  
  
  
  exit: function UI_exit() {
    let self = this;
    let zoomedIn = false;

    if (Search.isEnabled()) {
      let matcher = Search.createSearchTabMatcher();
      let matches = matcher.matched();

      if (matches.length > 0) {
        matches[0].zoomIn();
        zoomedIn = true;
      }
      Search.hide();
    }

    if (!zoomedIn) {
      let unhiddenGroups = GroupItems.groupItems.filter(function(groupItem) {
        return (!groupItem.hidden && groupItem.getChildren().length > 0);
      });
      
      
      if (!unhiddenGroups.length) {
        let emptyGroups = GroupItems.groupItems.filter(function (groupItem) {
          return (!groupItem.hidden && !groupItem.getChildren().length);
        });
        let group = (emptyGroups.length ? emptyGroups[0] : GroupItems.newGroup());
        if (!gBrowser._numPinnedTabs) {
          group.newTab(null, { closedLastTab: true });
          return;
        }
      }

      
      
      let activeTabItem = this.getActiveTab();
      if (!activeTabItem) {
        let tabItem = gBrowser.selectedTab._tabViewTabItem;
        if (tabItem) {
          if (!tabItem.parent || !tabItem.parent.hidden) {
            activeTabItem = tabItem;
          } else { 
            if (unhiddenGroups.length > 0)
              activeTabItem = unhiddenGroups[0].getActiveTab();
          }
        }
      }

      if (activeTabItem) {
        activeTabItem.zoomIn();
      } else {
        if (gBrowser._numPinnedTabs > 0) {
          if (gBrowser.selectedTab.pinned) {
            self.goToTab(gBrowser.selectedTab);
          } else {
            Array.some(gBrowser.tabs, function(tab) {
              if (tab.pinned) {
                self.goToTab(tab);
                return true;
              }
              return false
            });
          }
        }
      }
    }
  },

  
  
  
  _storageSanity: function UI__storageSanity(data) {
    if (Utils.isEmptyObject(data))
      return true;

    if (!Utils.isRect(data.pageBounds)) {
      Utils.log("UI.storageSanity: bad pageBounds", data.pageBounds);
      data.pageBounds = null;
      return false;
    }

    return true;
  },

  
  
  
  _save: function UI__save() {
    if (!this._frameInitialized)
      return;

    var data = {
      pageBounds: this._pageBounds
    };

    if (this._storageSanity(data))
      Storage.saveUIData(gWindow, data);
  },

  
  
  
  
  _saveAll: function UI__saveAll() {
    this._save();
    GroupItems.saveAll();
    TabItems.saveAll();
  },

  
  
  
  shouldLoadFavIcon: function UI_shouldLoadFavIcon(browser) {
    return !(browser.contentDocument instanceof window.ImageDocument) &&
            (browser.currentURI.schemeIs("about") ||
             gBrowser.shouldLoadFavIcon(browser.contentDocument.documentURIObject));
  },

  
  
  
  getFavIconUrlForTab: function UI_getFavIconUrlForTab(tab) {
    let url;

    if (tab.image) {
      
      if (/^https?:/.test(tab.image))
        url = gFavIconService.getFaviconLinkForIcon(gWindow.makeURI(tab.image)).spec;
      else
        url = tab.image;
    } else {
      url = gFavIconService.getFaviconImageForPage(tab.linkedBrowser.currentURI).spec;
    }

    return url;
  },

  
  
  
  
  
  notifySessionRestoreEnabled: function UI_notifySessionRestoreEnabled() {
    let brandBundle = gWindow.document.getElementById("bundle_brand");
    let brandShortName = brandBundle.getString("brandShortName");
    let notificationText = tabviewBundle.formatStringFromName(
      "tabview.notification.sessionStore", [brandShortName], 1);

    let banner = iQ("<div>")
      .text(notificationText)
      .addClass("banner")
      .appendTo("body");

    let onFadeOut = function () {
      banner.remove();
    };

    let onFadeIn = function () {
      setTimeout(function () {
        banner.animate({opacity: 0}, {duration: 1500, complete: onFadeOut});
      }, 5000);
    };

    banner.animate({opacity: 0.7}, {duration: 1500, complete: onFadeIn});
  }
};


UI.init();
