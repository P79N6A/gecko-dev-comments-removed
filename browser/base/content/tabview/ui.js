











































let Keys = { meta: false };




let UI = {
  
  
  _frameInitalized: false,

  
  
  _pageBounds : null,

  
  
  _closedLastVisibleTab : false,

  
  
  _closedSelectedTabInTabView : false,

  
  
  
  _reorderTabItemsOnShow : [],

  
  
  
  _reorderTabsOnHide : [],

  
  
  
  _currentTab : null,

  
  
  _eventListeners: {},

  
  
  
  init: function UI_init() {
    try {
      let self = this;

      
      Storage.init();
      let data = Storage.readUIData(gWindow);
      this._storageSanity(data);
      this._pageBounds = data.pageBounds;

      
      gWindow.addEventListener("tabviewshow", function() {
        self.showTabView(true);
      }, false);

      
      this._currentTab = gBrowser.selectedTab;

      
      
      
      
      
      if (false)
        this._addDevMenu();

      
      
      iQ(gTabViewFrame.contentDocument).mousedown(function(e) {
        if (iQ(":focus").length > 0) {
          iQ(":focus").each(function(element) {
            if (element.nodeName == "INPUT")
              element.blur();
          });
        }
        if (e.originalTarget.id == "content")
          self._createGroupItemOnDrag(e)
      });

      iQ(window).bind("beforeunload", function() {
        Array.forEach(gBrowser.tabs, function(tab) {
          gBrowser.showTab(tab);
        });
      });
      iQ(window).bind("unload", function() {
        self.uninit();
      });

      gWindow.addEventListener("tabviewhide", function() {
        var activeTab = self.getActiveTab();
        if (activeTab)
          activeTab.zoomIn();
      }, false);

      
      this._setTabViewFrameKeyHandlers();

      
      this._addTabActionHandlers();

      

      GroupItems.init();

      var groupItemsData = Storage.readGroupItemsData(gWindow);
      var firstTime = !groupItemsData || Utils.isEmptyObject(groupItemsData);
      var groupItemData = Storage.readGroupItemData(gWindow);
      GroupItems.reconstitute(groupItemsData, groupItemData);
      GroupItems.killNewTabGroup(); 

      
      TabItems.init();
      TabItems.pausePainting();

      if (firstTime) {
        var padding = 10;
        var infoWidth = 350;
        var infoHeight = 350;
        var pageBounds = Items.getPageBounds();
        pageBounds.inset(padding, padding);

        
        var box = new Rect(pageBounds);
        box.width =
          Math.min(box.width * 0.667, pageBounds.width - (infoWidth + padding));
        box.height = box.height * 0.667;
        var options = {
          bounds: box
        };

        var groupItem = new GroupItem([], options);

        var items = TabItems.getItems();
        items.forEach(function(item) {
          if (item.parent)
            item.parent.remove(item);

          groupItem.add(item);
        });

        
        let welcome = "How to organize your tabs";
        let more = "";
        let video = "http://videos-cdn.mozilla.net/firefox4beta/tabcandy_howto.webm";
        var html =
          "<div class='intro'>"
            + "<h1>" + welcome + "</h1>"
            + ( more && more.length ? "<div>" + more + "</div><br>" : "")
            + "<video src='" + video + "' "
            + "width='100%' preload controls>"
          + "</div>";

        box.left = box.right + padding;
        box.width = infoWidth;
        box.height = infoHeight;
        var infoItem = new InfoItem(box);
        infoItem.html(html);
      }

      
      if (this._pageBounds)
        this._resize(true);
      else
        this._pageBounds = Items.getPageBounds();

      iQ(window).resize(function() {
        self._resize();
      });

      
      var observer = {
        observe : function(subject, topic, data) {
          if (topic == "quit-application-requested") {
            if (self._isTabViewVisible())
              TabItems.saveAll(true);
            self._save();
          }
        }
      };
      Services.obs.addObserver(observer, "quit-application-requested", false);

      
      this._frameInitalized = true;
      this._save();
    } catch(e) {
      Utils.log(e);
    }
  },

  uninit: function UI_uninit() {
    TabItems.uninit();
    GroupItems.uninit();
    Storage.uninit();

    this._removeTabActionHandlers();
    this._currentTab = null;
    this._pageBounds = null;
    this._reorderTabItemsOnShow = null;
    this._reorderTabsOnHide = null;
  },

  
  
  
  
  getActiveTab: function UI_getActiveTab() {
    return this._activeTab;
  },

  
  
  
  
  
  
  
  
  
  setActiveTab: function UI_setActiveTab(tab) {
    if (tab == this._activeTab)
      return;

    if (this._activeTab) {
      this._activeTab.makeDeactive();
      this._activeTab.removeSubscriber(this, "close");
    }
    this._activeTab = tab;

    if (this._activeTab) {
      var self = this;
      this._activeTab.addSubscriber(this, "close", function() {
        self._activeTab = null;
      });

      this._activeTab.makeActive();
    }
  },

  
  
  
  _isTabViewVisible: function UI__isTabViewVisible() {
    return gTabViewDeck.selectedIndex == 1;
  },

  
  
  
  
  
  showTabView: function UI_showTabView(zoomOut) {
    if (this._isTabViewVisible())
      return;

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
    gTabViewFrame.contentWindow.focus();

    gBrowser.updateTitlebar();
#ifdef XP_MACOSX
    this._setActiveTitleColor(true);
#endif
    let event = document.createEvent("Events");
    event.initEvent("tabviewshown", true, false);

    if (zoomOut && currentTab && currentTab.tabItem) {
      item = currentTab.tabItem;
      
      
      

      
      item.zoomOut(function() {
        if (!currentTab.tabItem) 
          item = null;

        self.setActiveTab(item);

        if (item.parent) {
          var activeGroupItem = GroupItems.getActiveGroupItem();
          if (activeGroupItem)
            activeGroupItem.setTopChild(item);
        }

        self._resize(true);
        dispatchEvent(event);
      });
    } else
      dispatchEvent(event);

    TabItems.resumePainting();
  },

  
  
  
  hideTabView: function UI_hideTabView() {
    if (!this._isTabViewVisible())
      return;

    TabItems.pausePainting();

    this._reorderTabsOnHide.forEach(function(groupItem) {
      groupItem.reorderTabsBasedOnTabItemOrder();
    });
    this._reorderTabsOnHide = [];

#ifdef XP_WIN
    
    
    
    gTabViewFrame.style.marginTop = gBrowser.boxObject.y + "px";
#endif
    gTabViewDeck.selectedIndex = 0;
    gBrowser.contentWindow.focus();

    
    gBrowser.tabContainer.adjustTabstrip();

    gBrowser.updateTitlebar();
#ifdef XP_MACOSX
    this._setActiveTitleColor(false);
#endif
    let event = document.createEvent("Events");
    event.initEvent("tabviewhidden", true, false);
    dispatchEvent(event);
  },

#ifdef XP_MACOSX
  
  
  
  
  
  
  
  _setActiveTitleColor: function UI__setActiveTitleColor(set) {
    
    var mainWindow = gWindow.document.getElementById("main-window");
    if (set)
      mainWindow.setAttribute("activetitlebarcolor", "#C4C4C4");
    else
      mainWindow.removeAttribute("activetitlebarcolor");
  },
#endif

  
  
  
  _addTabActionHandlers: function UI__addTabActionHandlers() {
    var self = this;

    this._eventListeners.close = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

      if (self._isTabViewVisible()) {
        
        if (self._currentTab == tab)
          self._closedSelectedTabInTabView = true;
      } else {
        
        if (gBrowser.tabs.length > 1) {
          var groupItem = GroupItems.getActiveGroupItem();

          
          
          
          
          
          
          
          if ((groupItem && groupItem._children.length == 1) ||
              (groupItem == null && gBrowser.visibleTabs.length == 1)) {
            
            self._closedLastVisibleTab = true;
            
            if (tab && tab.tabItem)
              tab.tabItem.setZoomPrep(false);
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

    for (let name in this._eventListeners)
      AllTabs.register(name, this._eventListeners[name]);
  },

  
  
  
  _removeTabActionHandlers: function UI__removeTabActionHandlers() {
    for (let name in this._eventListeners)
      AllTabs.unregister(name, this._eventListeners[name]);
  },

  
  
  
  onTabSelect: function UI_onTabSelect(tab) {
    let currentTab = this._currentTab;
    this._currentTab = tab;

    
    if (this._isTabViewVisible() &&
        (this._closedLastVisibleTab || this._closedSelectedTabInTabView)) {
      this._closedLastVisibleTab = false;
      this._closedSelectedTabInTabView = false;
      return;
    }
    
    this._closedLastVisibleTab = false;
    this._closedSelectedTabInTabView = false;

    
    
    if (this._isTabViewVisible())
      this.hideTabView();

    let oldItem = null;
    let newItem = null;
    
    if (currentTab && currentTab.tabItem)
      oldItem = currentTab.tabItem;
    if (tab && tab.tabItem) {
      newItem = tab.tabItem;
      GroupItems.updateActiveGroupItemAndTabBar(newItem);
    }

    
    if (newItem != oldItem) {
      if (oldItem)
        oldItem.setZoomPrep(false);
      if (newItem)
        newItem.setZoomPrep(true);
    } else if (oldItem)
      oldItem.setZoomPrep(true);
  },

  
  
  
  
  
  
  setReorderTabsOnHide: function UI_setReorderTabsOnHide(groupItem) {
    if (this._isTabViewVisible()) {
      var index = this._reorderTabsOnHide.indexOf(groupItem);
      if (index == -1)
        this._reorderTabsOnHide.push(groupItem);
    }
  },

  
  
  
  
  
  
  setReorderTabItemsOnShow: function UI_setReorderTabItemsOnShow(groupItem) {
    if (!this._isTabViewVisible()) {
      var index = this._reorderTabItemsOnShow.indexOf(groupItem);
      if (index == -1)
        this._reorderTabItemsOnShow.push(groupItem);
    }
  },

  
  
  
  _setTabViewFrameKeyHandlers: function UI__setTabViewFrameKeyHandlers() {
    var self = this;

    iQ(window).keyup(function(event) {
      if (!event.metaKey) Keys.meta = false;
    });

    iQ(window).keydown(function(event) {
      if (event.metaKey) Keys.meta = true;

      if (!self.getActiveTab() || iQ(":focus").length > 0) {
        
        
        if (event.keyCode == KeyEvent.DOM_VK_TAB) {
          event.stopPropagation();
          event.preventDefault();
        }
        return;
      }

      function getClosestTabBy(norm) {
        var centers =
          [[item.bounds.center(), item] for each(item in TabItems.getItems())];
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
          if (nextTab.inStack() && !nextTab.parent.expanded)
            nextTab = nextTab.parent.getChild(0);
          self.setActiveTab(nextTab);
        }
        event.stopPropagation();
        event.preventDefault();
      } else if (event.keyCode == KeyEvent.DOM_VK_SPACE) {
        
#ifdef XP_MACOSX
        if (event.altKey && !event.metaKey && !event.shiftKey &&
            !event.ctrlKey) {
#else
        if (event.ctrlKey && !event.metaKey && !event.shiftKey &&
            !event.altKey) {
#endif
          var activeTab = self.getActiveTab();
          if (activeTab)
            activeTab.zoomIn();
          event.stopPropagation();
          event.preventDefault();
        }
      } else if (event.keyCode == KeyEvent.DOM_VK_ESCAPE || 
                 event.keyCode == KeyEvent.DOM_VK_RETURN ||
                 event.keyCode == KeyEvent.DOM_VK_ENTER) {
        let activeTab = self.getActiveTab();
        let activeGroupItem = GroupItems.getActiveGroupItem();

        if (activeGroupItem && activeGroupItem.expanded && 
            event.keyCode == KeyEvent.DOM_VK_ESCAPE)
          activeGroupItem.collapse();
        else if (activeTab)
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
      }
    });
  },

  
  
  
  
  _createGroupItemOnDrag: function UI__createGroupItemOnDrag(e) {
    const minSize = 60;
    const minMinSize = 15;

    let lastActiveGroupItem = GroupItems.getActiveGroupItem();
    GroupItems.setActiveGroupItem(null);

    var startPos = { x: e.clientX, y: e.clientY };
    var phantom = iQ("<div>")
      .addClass("groupItem phantom activeGroupItem")
      .css({
        position: "absolute",
        opacity: .7,
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
        this.container.css("z-index", z);
      },
      setOpacity: function FauxItem_setOpacity(opacity) {
        this.container.css("opacity", opacity);
      },
      
      
      pushAway: function () {},
    };
    item.setBounds(new Rect(startPos.y, startPos.x, 0, 0));

    var dragOutInfo = new Drag(item, e, true); 

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
      phantom.animate({
        width: 0,
        height: 0,
        top: phantom.position().x + phantom.height()/2,
        left: phantom.position().y + phantom.width()/2
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
      dragOutInfo.stop();
      if (phantom.css("opacity") != 1)
        collapse();
      else {
        var bounds = item.getBounds();

        
        
        var tabs = GroupItems.getOrphanedTabs();
        var insideTabs = [];
        for each(tab in tabs) {
          if (bounds.contains(tab.bounds))
            insideTabs.push(tab);
        }

        var groupItem = new GroupItem(insideTabs,{bounds:bounds});
        GroupItems.setActiveGroupItem(groupItem);
        phantom.remove();
        dragOutInfo = null;
      }
    }

    iQ(window).mousemove(updateSize)
    iQ(gWindow).one("mouseup", finalize);
    e.preventDefault();
    return false;
  },

  
  
  
  
  
  
  _resize: function UI__resize(force) {
    if (typeof force == "undefined")
      force = false;

    
    
    if (!force && !this._isTabViewVisible())
      return;

    var oldPageBounds = new Rect(this._pageBounds);
    var newPageBounds = Items.getPageBounds();
    if (newPageBounds.equals(oldPageBounds))
      return;

    var items = Items.getTopLevelItems();

    
    var itemBounds = new Rect(this._pageBounds);
    
    
    itemBounds.width = 1;
    itemBounds.height = 1;
    items.forEach(function(item) {
      if (item.locked.bounds)
        return;

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
      if (item.locked.bounds)
        return;

      var bounds = item.getBounds();
      bounds.left += newPageBounds.left - self._pageBounds.left;
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

  
  
  
  _addDevMenu: function UI__addDevMenu() {
    try {
      var self = this;

      var $select = iQ("<select>")
        .css({
          position: "absolute",
          bottom: 5,
          right: 5,
          zIndex: 99999,
          opacity: .2
        })
        .appendTo("#content")
        .change(function () {
          var index = iQ(this).val();
          try {
            commands[index].code.apply(commands[index].element);
          } catch(e) {
            Utils.log("dev menu error", e);
          }
          iQ(this).val(0);
        });

      var commands = [{
        name: "dev menu",
        code: function() { }
      }, {
        name: "show trenches",
        code: function() {
          Trenches.toggleShown();
          iQ(this).html((Trenches.showDebug ? "hide" : "show") + " trenches");
        }
      }, {












        name: "save",
        code: function() {
          self._saveAll();
        }
      }];

      var count = commands.length;
      var a;
      for (a = 0; a < count; a++) {
        commands[a].element = (iQ("<option>")
          .val(a)
          .html(commands[a].name)
          .appendTo($select))[0];
      }
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  _reset: function UI__reset() {
    Storage.wipe();
    location.href = "";
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
    if (!this._frameInitalized)
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
};


UI.init();
