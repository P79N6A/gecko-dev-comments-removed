













































let Keys = { meta: false };




let UI = {
  
  
  
  DBLCLICK_INTERVAL: 500,

  
  
  
  DBLCLICK_OFFSET: 5,

  
  
  _frameInitialized: false,

  
  
  _pageBounds: null,

  
  
  _closedLastVisibleTab: false,

  
  
  _closedSelectedTabInTabView: false,

  
  
  restoredClosedTab: false,

  
  
  
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
  
  
  
  _storageBusyCount: 0,

  
  
  
  init: function UI_init() {
    try {
      let self = this;

      
      this._initPageDirection();

      
      Storage.init();
      let data = Storage.readUIData(gWindow);
      this._storageSanity(data);
      this._pageBounds = data.pageBounds;

      
      gWindow.addEventListener("tabviewshow", function() {
        self.showTabView(true);
      }, false);

      
      this._currentTab = gBrowser.selectedTab;

      
      iQ("#exit-button").click(function() {
        self.exit();
        self.blurAll();
      });

      
      
      iQ(gTabViewFrame.contentDocument).mousedown(function(e) {
        if (iQ(":focus").length > 0) {
          iQ(":focus").each(function(element) {
            
            if (e.target != element && element.nodeName == "INPUT")
              element.blur();
          });
        }
        if (e.originalTarget.id == "content") {
          
          if (Date.now() - self._lastClick <= self.DBLCLICK_INTERVAL && 
              (self._lastClickPositions.x - self.DBLCLICK_OFFSET) <= e.clientX &&
              (self._lastClickPositions.x + self.DBLCLICK_OFFSET) >= e.clientX &&
              (self._lastClickPositions.y - self.DBLCLICK_OFFSET) <= e.clientY &&
              (self._lastClickPositions.y + self.DBLCLICK_OFFSET) >= e.clientY) {
            GroupItems.setActiveGroupItem(null);
            TabItems.creatingNewOrphanTab = true;

            let newTab = 
              gBrowser.loadOneTab("about:blank", { inBackground: true });

            let box = 
              new Rect(e.clientX - Math.floor(TabItems.tabWidth/2),
                       e.clientY - Math.floor(TabItems.tabHeight/2),
                       TabItems.tabWidth, TabItems.tabHeight);
            newTab._tabViewTabItem.setBounds(box, true);
            newTab._tabViewTabItem.pushAway(true);
            GroupItems.setActiveOrphanTab(newTab._tabViewTabItem);

            TabItems.creatingNewOrphanTab = false;
            newTab._tabViewTabItem.zoomIn(true);

            self._lastClick = 0;
            self._lastClickPositions = null;
            gTabView.firstUseExperienced = true;
          } else {
            self._lastClick = Date.now();
            self._lastClickPositions = new Point(e.clientX, e.clientY);
            self._createGroupItemOnDrag(e);
          }
        }
      });

      iQ(window).bind("unload", function() {
        self.uninit();
      });

      gWindow.addEventListener("tabviewhide", function() {
        self.exit();
      }, false);

      
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

      
      function domWinClosedObserver(subject, topic, data) {
        if (topic == "domwindowclosed" && subject == gWindow) {
          if (self.isTabViewVisible())
            GroupItems.removeHiddenGroups();
          TabItems.saveAll(true);
          self._save();
        }
      }
      Services.obs.addObserver(
        domWinClosedObserver, "domwindowclosed", false);
      this._cleanupFunctions.push(function() {
        Services.obs.removeObserver(domWinClosedObserver, "domwindowclosed");
      });

      
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
    GroupItems.setActiveGroupItem(groupItem);
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

  
  
  
  
  
  
  
  
  
  setActiveTab: function UI_setActiveTab(tabItem) {
    if (tabItem == this._activeTab)
      return;

    if (this._activeTab) {
      this._activeTab.makeDeactive();
      this._activeTab.removeSubscriber(this, "close");
    }
    this._activeTab = tabItem;

    if (this._activeTab) {
      let self = this;
      this._activeTab.addSubscriber(this, "close", function(closedTabItem) {
        if (self._activeTab == closedTabItem)
          self.setActiveTab(null);
      });

      this._activeTab.makeActive();
    }
  },

  
  
  
  isTabViewVisible: function UI_isTabViewVisible() {
    return gTabViewDeck.selectedIndex == 1;
  },

  
  
  
  _initPageDirection: function UI__initPageDirection() {
    let chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].
                    getService(Ci.nsIXULChromeRegistry);
    let dir = chromeReg.isLocaleRTL("global");
    document.documentElement.setAttribute("dir", dir ? "rtl" : "ltr");
    this.rtl = dir;
  },

  
  
  
  
  
  showTabView: function UI_showTabView(zoomOut) {
    if (this.isTabViewVisible())
      return;

    
    this._initPageDirection();

    var self = this;
    var currentTab = this._currentTab;
    var item = null;

    this._reorderTabItemsOnShow.forEach(function(groupItem) {
      groupItem.reorderTabItemsBasedOnTabOrder();
    });
    this._reorderTabItemsOnShow = [];

#ifdef XP_WIN
    
    gTabViewFrame.style.marginTop = "";
#endif
    gTabViewDeck.selectedIndex = 1;
    gWindow.TabsInTitlebar.allowedBy("tabview-open", false);
    gTabViewFrame.contentWindow.focus();

    gBrowser.updateTitlebar();
#ifdef XP_MACOSX
    this.setTitlebarColors(true);
#endif
    let event = document.createEvent("Events");
    event.initEvent("tabviewshown", true, false);

    Storage.saveVisibilityData(gWindow, "true");

    
    
    
    
    let activeGroupItem = null;
    if (!GroupItems.getActiveOrphanTab()) {
      activeGroupItem = GroupItems.getActiveGroupItem();
      if (activeGroupItem && activeGroupItem.closeIfEmpty())
        activeGroupItem = null;
    }

    if (zoomOut && currentTab && currentTab._tabViewTabItem) {
      item = currentTab._tabViewTabItem;
      
      
      

      
      item.zoomOut(function() {
        if (!currentTab._tabViewTabItem) 
          item = null;

        self.setActiveTab(item);

        if (activeGroupItem && item.parent)
          activeGroupItem.setTopChild(item);

        self._resize(true);
        dispatchEvent(event);

        
        GroupItems.flushAppTabUpdates();

        TabItems.resumePainting();
      });
    } else {
      self.setActiveTab(null);
      dispatchEvent(event);

      
      GroupItems.flushAppTabUpdates();

      TabItems.resumePainting();
    }
  },

  
  
  
  hideTabView: function UI_hideTabView() {
    if (!this.isTabViewVisible())
      return;

    
    
    GroupItems.removeHiddenGroups();
    TabItems.pausePainting();

    this._reorderTabsOnHide.forEach(function(groupItem) {
      groupItem.reorderTabsBasedOnTabItemOrder();
    });
    this._reorderTabsOnHide = [];

#ifdef XP_WIN
    
    
    
    gTabViewFrame.style.marginTop = gBrowser.boxObject.y + "px";
#endif
    gTabViewDeck.selectedIndex = 0;
    gWindow.TabsInTitlebar.allowedBy("tabview-open", true);
    gBrowser.contentWindow.focus();

    gBrowser.updateTitlebar();
#ifdef XP_MACOSX
    this.setTitlebarColors(false);
#endif
    Storage.saveVisibilityData(gWindow, "false");

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
    if (!this._storageBusyCount) {
      TabItems.pauseReconnecting();
      GroupItems.pauseAutoclose();
    }
    
    this._storageBusyCount++;
  },
  
  
  
  
  
  storageReady: function UI_storageReady() {
    this._storageBusyCount--;
    if (!this._storageBusyCount) {
      let hasGroupItemsData = GroupItems.load();
      if (!hasGroupItemsData)
        this.reset();
  
      TabItems.resumeReconnecting();
      GroupItems._updateTabBar();
      GroupItems.resumeAutoclose();
    }
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

    
    
    
    
    
    
    function pbObserver(aSubject, aTopic, aData) {
      if (aTopic == "private-browsing") {
        
        
        if (aData == "enter") {
          
          self._privateBrowsing.wasInTabView = self.isTabViewVisible();
          if (self.isTabViewVisible())
            self.goToTab(gBrowser.selectedTab);
        }
      } else if (aTopic == "private-browsing-change-granted") {
        if (aData == "enter" || aData == "exit") {
          self._privateBrowsing.transitionMode = aData;
          self.storageBusy();
        }
      } else if (aTopic == "private-browsing-transition-complete") {
        
        if (self._privateBrowsing.transitionMode == "exit" &&
            self._privateBrowsing.wasInTabView)
          self.showTabView(false);

        self._privateBrowsing.transitionMode = "";
        self.storageReady();
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

    
    this._eventListeners.open = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

      
      if (tab.pinned)
        GroupItems.addAppTab(tab);
    };
    
    
    this._eventListeners.close = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

      
      if (tab.pinned)
        GroupItems.removeAppTab(tab);
        
      if (self.isTabViewVisible()) {
        
        if (self._currentTab == tab)
          self._closedSelectedTabInTabView = true;
      } else {
        
        
        if (self._privateBrowsing.transitionMode)
          return; 
          
        
        if (gBrowser.tabs.length > 1) {
          
          for (let a = 0; a < gBrowser.tabs.length; a++) {
            let theTab = gBrowser.tabs[a]; 
            if (theTab.pinned && gBrowser._removingTabs.indexOf(theTab) == -1) 
              return;
          }

          var groupItem = GroupItems.getActiveGroupItem();

          
          
          let closingLastOfGroup = (groupItem && 
              groupItem._children.length == 1 && 
              groupItem._children[0].tab == tab);

          
          
          
          let closingUnnamedGroup = (groupItem == null &&
              gBrowser.visibleTabs.length <= 1); 

          
          
          
          let closingBlankTabAfterRestore =
            (tab && tab._tabViewTabIsRemovedAfterRestore);

          if ((closingLastOfGroup || closingUnnamedGroup) &&
              !closingBlankTabAfterRestore) {
            
            self._closedLastVisibleTab = true;
            self.showTabView();
          }
        }
      }
    };

    
    this._eventListeners.move = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

      let activeGroupItem = GroupItems.getActiveGroupItem();
      if (activeGroupItem)
        self.setReorderTabItemsOnShow(activeGroupItem);
    };

    
    this._eventListeners.select = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

      self.onTabSelect(tab);
    };

    
    this._eventListeners.pinned = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

      TabItems.handleTabPin(tab);
      GroupItems.addAppTab(tab);
    };

    
    this._eventListeners.unpinned = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

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
    let currentTab = this._currentTab;
    this._currentTab = tab;

    
    if (this.isTabViewVisible() &&
        (this._closedLastVisibleTab || this._closedSelectedTabInTabView ||
         this.restoredClosedTab)) {
      if (this.restoredClosedTab) {
        
        
        tab.linkedBrowser.addEventListener("load", function (event) {
          tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
          TabItems._update(tab);
        }, true);
      }
      this._closedLastVisibleTab = false;
      this._closedSelectedTabInTabView = false;
      this.restoredClosedTab = false;
      return;
    }
    
    this._closedLastVisibleTab = false;
    this._closedSelectedTabInTabView = false;
    this.restoredClosedTab = false;

    
    
    if (this.isTabViewVisible())
      this.hideTabView();

    
    
    if (this._currentTab != tab)
      return;

    let oldItem = null;
    let newItem = null;

    if (currentTab && currentTab._tabViewTabItem)
      oldItem = currentTab._tabViewTabItem;

    
    if (tab && tab._tabViewTabItem) {
      if (!TabItems.reconnectingPaused()) {
        newItem = tab._tabViewTabItem;
        GroupItems.updateActiveGroupItemAndTabBar(newItem);
      }
    } else {
      
      
      
      if (!GroupItems.getActiveGroupItem() && !GroupItems.getActiveOrphanTab()) {
        for (let a = 0; a < gBrowser.tabs.length; a++) {
          let theTab = gBrowser.tabs[a]; 
          if (!theTab.pinned) {
            let tabItem = theTab._tabViewTabItem; 
            if (tabItem.parent) 
              GroupItems.setActiveGroupItem(tabItem.parent);
            else 
              GroupItems.setActiveOrphanTab(tabItem); 
              
            break;
          }
        }
      }

      if (GroupItems.getActiveGroupItem() || GroupItems.getActiveOrphanTab())
        GroupItems._updateTabBar();
    }
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
  
  
  updateTabButton: function UI__updateTabButton() {
    let groupsNumber = gWindow.document.getElementById("tabviewGroupsNumber");
    let exitButton = document.getElementById("exit-button");
    let numberOfGroups = GroupItems.groupItems.length;

    groupsNumber.setAttribute("groups", numberOfGroups);
    exitButton.setAttribute("groups", numberOfGroups);
  },

  
  
  
  getClosestTab: function UI_getClosestTab(tabCenter) {
    let cl = null;
    let clDist;
    TabItems.getItems().forEach(function (item) {
      if (item.parent && item.parent.hidden)
        return;
      let testDist = tabCenter.distance(item.bounds.center());
      if (cl==null || testDist < clDist) {
        cl = item;
        clDist = testDist;
      }
    });
    return cl;
  },

  
  
  
  _setTabViewFrameKeyHandlers: function UI__setTabViewFrameKeyHandlers() {
    var self = this;

    iQ(window).keyup(function(event) {
      if (!event.metaKey) 
        Keys.meta = false;
    });

    iQ(window).keydown(function(event) {
      if (event.metaKey) 
        Keys.meta = true;

      if ((iQ(":focus").length > 0 && iQ(":focus")[0].nodeName == "INPUT") || 
          isSearchEnabled())
        return;

      function getClosestTabBy(norm) {
        if (!self.getActiveTab())
          return null;
        var centers =
          [[item.bounds.center(), item]
             for each(item in TabItems.getItems()) if (!item.parent || !item.parent.hidden)];
        var myCenter = self.getActiveTab().bounds.center();
        var matches = centers
          .filter(function(item){return norm(item[0], myCenter)})
          .sort(function(a,b){
            return myCenter.distance(a[0]) - myCenter.distance(b[0]);
          });
        if (matches.length > 0)
          return matches[0][1];
        return null;
      }

      var norm = null;
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
          self.setActiveTab(nextTab);
        }
        event.stopPropagation();
        event.preventDefault();
      } else if (event.keyCode == KeyEvent.DOM_VK_ESCAPE) {
        let activeGroupItem = GroupItems.getActiveGroupItem();
        if (activeGroupItem && activeGroupItem.expanded)
          activeGroupItem.collapse();
        else 
          self.exit();

        event.stopPropagation();
        event.preventDefault();
      } else if (event.keyCode == KeyEvent.DOM_VK_RETURN ||
                 event.keyCode == KeyEvent.DOM_VK_ENTER) {
        let activeTab = self.getActiveTab();
        if (activeTab)
          activeTab.zoomIn();

        event.stopPropagation();
        event.preventDefault();
      } else if (event.keyCode == KeyEvent.DOM_VK_TAB) {
        
        var activeTab = self.getActiveTab();
        if (activeTab) {
          var tabItems = (activeTab.parent ? activeTab.parent.getChildren() :
                          [activeTab]);
          var length = tabItems.length;
          var currentIndex = tabItems.indexOf(activeTab);

          if (length > 1) {
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
            self.setActiveTab(tabItems[newIndex]);
          }
        }
        event.stopPropagation();
        event.preventDefault();
      } else if (event.keyCode == KeyEvent.DOM_VK_SLASH) {
        
        
        self.enableSearch(event);
      }
    });
  },

  
  
  
  
  
  enableSearch: function UI_enableSearch(event) {
    if (!isSearchEnabled()) {
      ensureSearchShown();
      SearchEventHandler.switchToInMode();
      
      if (event) {
        event.stopPropagation();
        event.preventDefault();
      }
    }
  },

  
  
  
  
  _createGroupItemOnDrag: function UI__createGroupItemOnDrag(e) {
    const minSize = 60;
    const minMinSize = 15;

    let lastActiveGroupItem = GroupItems.getActiveGroupItem();
    GroupItems.setActiveGroupItem(null);

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
      GroupItems.setActiveGroupItem(lastActiveGroupItem);
    }

    function finalize(e) {
      iQ(window).unbind("mousemove", updateSize);
      item.container.removeClass("dragRegion");
      dragOutInfo.stop();
      let box = item.getBounds();
      if (box.width > minMinSize && box.height > minMinSize &&
         (box.width > minSize || box.height > minSize)) {
        var bounds = item.getBounds();

        
        
        var tabs = GroupItems.getOrphanedTabs();
        var insideTabs = [];
        for each(let tab in tabs) {
          if (bounds.contains(tab.bounds))
            insideTabs.push(tab);
        }

        var groupItem = new GroupItem(insideTabs,{bounds:bounds});
        GroupItems.setActiveGroupItem(groupItem);
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

    if (isSearchEnabled()) {
      let matcher = createSearchTabMacher();
      let matches = matcher.matched();

      if (matches.length > 0) {
        matches[0].zoomIn();
        zoomedIn = true;
      }
      hideSearch(null);
    }

    if (!zoomedIn) {
      let unhiddenGroups = GroupItems.groupItems.filter(function(groupItem) {
        return (!groupItem.hidden && groupItem.getChildren().length > 0);
      });
      
      
      if (!unhiddenGroups.length && !GroupItems.getOrphanedTabs().length) {
        let emptyGroups = GroupItems.groupItems.filter(function (groupItem) {
          return (!groupItem.hidden && !groupItem.getChildren().length);
        });
        let group = (emptyGroups.length ? emptyGroups[0] : GroupItems.newGroup());
        if (!gBrowser._numPinnedTabs) {
          group.newTab();
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
  }
};


UI.init();
