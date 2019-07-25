










































(function() {

window.Keys = { meta: false };




var UIManager = {
  
  
  
  _devMode : true,

  
  
  _pageBounds : null,

  
  
  _closedLastVisibleTab : false,

  
  
  _closedSelectedTabInTabCandy : false,

  
  
  _stopZoomPreparation : false,

  
  
  
  _reorderTabItemsOnShow : [],

  
  
  
  _reorderTabsOnHide : [],

  
  
  
  _currentTab : gBrowser.selectedTab,

  
  
  
  init: function() {
    try {
      if (window.Tabs)
        this._secondaryInit();
      else {
        var self = this;
        TabsManager.addSubscriber(this, "load", function() {
          self._secondaryInit();
        });
      }
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  
  _secondaryInit: function() {
    try {
      var self = this;

      
      if (this._devMode)
        this._addDevMenu();

      
      iQ("#reset").click(function() {
        self._reset();
      });

      
      
      iQ(gTabViewFrame.contentDocument).mousedown(function(e){
        if ( e.originalTarget.id == "content" )
          self._createGroupOnDrag(e)
      });

      iQ(window).bind("beforeunload", function() {
        Array.forEach(gBrowser.tabs, function(tab) {
          tab.hidden = false;
        });
      });

      gWindow.addEventListener("tabcandyshow", function() {
        self.showTabCandy(true);
      }, false);

      gWindow.addEventListener("tabcandyhide", function() {
        var activeTab = self.getActiveTab();
        if (activeTab)
          activeTab.zoomIn();
      }, false);

      
      this._setBrowserKeyHandlers();
      this._setTabViewFrameKeyHandlers();

      
      this._addTabActionHandlers();

      
      Storage.onReady(function() {
        self._delayInit();
      });
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  _delayInit : function() {
    try {
      var self = this;

      
      var data = Storage.readUIData(gWindow);
      this._storageSanity(data);

      var groupsData = Storage.readGroupsData(gWindow);
      var firstTime = !groupsData || Utils.isEmptyObject(groupsData);
      var groupData = Storage.readGroupData(gWindow);
      Groups.reconstitute(groupsData, groupData);

      TabItems.init();

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

        var group = new Group([], options);

        var items = TabItems.getItems();
        items.forEach(function(item) {
          if (item.parent)
            item.parent.remove(item);

          group.add(item);
        });

        
        var html =
          "<div class='intro'>"
            + "<h1>Welcome to Firefox Tab Sets</h1>"
            + "<div>(more goes here)</div><br>"
            + "<video src='http://html5demos.com/assets/dizzy.ogv' "
            + "width='100%' preload controls>"
          + "</div>";

        box.left = box.right + padding;
        box.width = infoWidth;
        box.height = infoHeight;
        var infoItem = new InfoItem(box);
        infoItem.html(html);
      }

      
      if (data.pageBounds) {
        this._pageBounds = data.pageBounds;
        this._resize(true);
      } else
        this._pageBounds = Items.getPageBounds();

      iQ(window).resize(function() {
        self._resize();
      });

      
      if (data.tabCandyVisible) {
        var currentTab = self._currentTab;
        var item;

        if (currentTab && currentTab.mirror)
          item = TabItems.getItemByTabElement(currentTab.mirror.el);

        if (item)
          item.setZoomPrep(false);
        else
          self._stopZoomPreparation = true;

        self.showTabCandy();
        
        
        
        Groups.groups.forEach(function(group) {
          self._reorderTabsOnHide.push(group);
        });
      } else {
         self.hideTabCandy();
        
        
        Groups.groups.forEach(function(group) {
          self._reorderTabItemsOnShow.push(group);
        });
      }

      
      Components.utils.import("resource://gre/modules/Services.jsm");
      var observer = {
        observe : function(subject, topic, data) {
          if (topic == "quit-application-requested") {
            if (self._isTabCandyVisible())
              TabItems.saveAll(true);
            self._save();
          }
        }
      };
      Services.obs.addObserver(observer, "quit-application-requested", false);

      
      this._initialized = true;
      this._save();
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  
  getActiveTab: function() {
    return this._activeTab;
  },

  
  
  
  
  
  
  
  
  
  setActiveTab: function(tab) {
    if (tab == this._activeTab)
      return;

    if (this._activeTab) {
      this._activeTab.makeDeactive();
      this._activeTab.removeOnClose(this);
    }
    this._activeTab = tab;

    if (this._activeTab) {
      var self = this;
      this._activeTab.addOnClose(this, function() {
        self._activeTab = null;
      });

      this._activeTab.makeActive();
    }
  },

  
  
  
  _isTabCandyVisible: function() {
    return gTabViewDeck.selectedIndex == 1;
  },

  
  
  
  
  
  showTabCandy: function(zoomOut) {
    var self = this;
    var currentTab = this._currentTab;
    var item = null;

    this._reorderTabItemsOnShow.forEach(function(group) {
      group.reorderTabItemsBasedOnTabOrder();
    });
    this._reorderTabItemsOnShow = [];

    gTabViewDeck.selectedIndex = 1;
    gTabViewFrame.contentWindow.focus();

    gBrowser.updateTitlebar();
#ifdef XP_MACOSX
    this._setActiveTitleColor(true);
#endif

    if (zoomOut) {
      if (currentTab && currentTab.mirror)
        item = TabItems.getItemByTabElement(currentTab.mirror.el);

      if (item) {
        
        
        

        
        item.zoomOut(function() {
          if (!currentTab.mirror) 
            item = null;

          self.setActiveTab(item);

          var activeGroup = Groups.getActiveGroup();
          if (activeGroup)
            activeGroup.setTopChild(item);

          window.Groups.setActiveGroup(null);
          self._resize(true);
        });
      }
    }
  },

  
  
  
  hideTabCandy: function() {
    this._reorderTabsOnHide.forEach(function(group) {
      group.reorderTabsBasedOnTabItemOrder();
    });
    this._reorderTabsOnHide = [];

    gTabViewDeck.selectedIndex = 0;
    gBrowser.contentWindow.focus();

    

      gBrowser.tabContainer.adjustTabstrip();


    gBrowser.updateTitlebar();
#ifdef XP_MACOSX
    this._setActiveTitleColor(false);
#endif
  },

#ifdef XP_MACOSX
  
  
  
  
  
  
  
  _setActiveTitleColor: function(set) {
    
    var mainWindow = gWindow.document.getElementById("main-window");
    if (set)
      mainWindow.setAttribute("activetitlebarcolor", "#C4C4C4");
    else
      mainWindow.removeAttribute("activetitlebarcolor");
  },
#endif

  
  
  
  _addTabActionHandlers: function() {
    var self = this;

    Tabs.onClose(function(){
      if (self._isTabCandyVisible()) {
        
        if (self._currentTab == this)
          self._closedSelectedTabInTabCandy = true;
      } else {
        var group = Groups.getActiveGroup();
        
        
        
        
        
        if ((group && group._children.length == 1) ||
            (group == null &&
             gBrowser.visibleTabs.length == 1)) {
          self._closedLastVisibleTab = true;
          
          if (this && this.mirror) {
            var item = TabItems.getItemByTabElement(this.mirror.el);
            if (item)
              item.setZoomPrep(false);
          }
          self.showTabCandy();
        }
      }
      return false;
    });

    Tabs.onMove(function() {
      if (!self._isTabCandyVisible()) {
        var activeGroup = Groups.getActiveGroup();
        if (activeGroup) {
          var index = self._reorderTabItemsOnShow.indexOf(activeGroup);
          if (index == -1)
            self._reorderTabItemsOnShow.push(activeGroup);
        }
      }
    });

    Tabs.onFocus(function() {
      self.tabOnFocus(this);
    });
  },

  
  
  
  tabOnFocus: function(tab) {
    var self = this;
    var focusTab = tab;
    var currentTab = this._currentTab;

    this._currentTab = focusTab;
    
    if (this._isTabCandyVisible() &&
        (this._closedLastVisibleTab || this._closedSelectedTabInTabCandy)) {
      this._closedLastVisibleTab = false;
      this._closedSelectedTabInTabCandy = false;
      return;
    }

    
    
    if (this._isTabCandyVisible())
      this.hideTabCandy();

    
    this._closedLastVisibleTab = false;
    this._closedSelectedTabInTabCandy = false;

    Utils.timeout(function() { 
      
      if (self._stopZoomPreparation) {
        self._stopZoomPreparation = false;
        if (focusTab && focusTab.mirror) {
          var item = TabItems.getItemByTabElement(focusTab.mirror.el);
          if (item)
            self.setActiveTab(item);
        }
        return;
      }

      if (focusTab != self._currentTab) {
        
        return;
      }

      var visibleTabCount = gBrowser.visibleTabs.length;

      var newItem = null;
      if (focusTab && focusTab.mirror)
        newItem = TabItems.getItemByTabElement(focusTab.mirror.el);

      if (newItem)
        Groups.setActiveGroup(newItem.parent);

      
      var oldItem = null;
      if (currentTab && currentTab.mirror)
        oldItem = TabItems.getItemByTabElement(currentTab.mirror.el);

      if (newItem != oldItem) {
        if (oldItem)
          oldItem.setZoomPrep(false);

        
        
        if (visibleTabCount > 0 && newItem)
          newItem.setZoomPrep(true);
      } else {
        
        
        if (oldItem)
          oldItem.setZoomPrep(true);
      }
    }, 1);
  },

  
  
  
  
  
  
  setReorderTabsOnHide: function(group) {
    if (this._isTabCandyVisible()) {
      var index = this._reorderTabsOnHide.indexOf(group);
      if (index == -1)
        this._reorderTabsOnHide.push(group);
    }
  },

  
  
  
  
  _setBrowserKeyHandlers : function() {
    var self = this;

    gWindow.addEventListener("keypress", function(event) {
      if (self._isTabCandyVisible())
        return;

      var charCode = event.charCode;
#ifdef XP_MACOSX
      
      
      if (!event.ctrlKey && !event.metaKey && !event.shiftKey &&
          charCode == 160) { 
#else
      if (event.ctrlKey && !event.metaKey && !event.shiftKey &&
          !event.altKey && charCode == 32) { 
#endif
        event.stopPropagation();
        event.preventDefault();
        self.showTabCandy(true);
        return;
      }

      
      if (event.ctrlKey && !event.metaKey && !event.altKey &&
          (charCode == 96 || charCode == 126)) {
        event.stopPropagation();
        event.preventDefault();
        var tabItem = Groups.getNextGroupTab(event.shiftKey);
        if (tabItem)
          gBrowser.selectedTab = tabItem.tab;
      }
    }, true);
  },

  
  
  
  _setTabViewFrameKeyHandlers: function(){
    var self = this;

    iQ(window).keyup(function(event) {
      if (!event.metaKey) window.Keys.meta = false;
    });

    iQ(window).keydown(function(event) {
      if (event.metaKey) window.Keys.meta = true;

      if (!self.getActiveTab() || iQ(":focus").length > 0) {
        
        
        if (event.which == 9) {
          event.stopPropagation();
          event.preventDefault();
        }
        return;
      }

      function getClosestTabBy(norm){
        var centers =
          [[item.bounds.center(), item] for each(item in TabItems.getItems())];
        var myCenter = self.getActiveTab().bounds.center();
        var matches = centers
          .filter(function(item){return norm(item[0], myCenter)})
          .sort(function(a,b){
            return myCenter.distance(a[0]) - myCenter.distance(b[0]);
          });
        if ( matches.length > 0 )
          return matches[0][1];
        return null;
      }

      var norm = null;
      switch (event.which) {
        case 39: 
          norm = function(a, me){return a.x > me.x};
          break;
        case 37: 
          norm = function(a, me){return a.x < me.x};
          break;
        case 40: 
          norm = function(a, me){return a.y > me.y};
          break;
        case 38: 
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
      } else if (event.which == 32) {
        
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
      } else if (event.which == 27 || event.which == 13) {
        
        var activeTab = self.getActiveTab();
        if (activeTab)
          activeTab.zoomIn();
        event.stopPropagation();
        event.preventDefault();
      } else if (event.which == 9) {
        
        var activeTab = self.getActiveTab();
        if (activeTab) {
          var tabItems = (activeTab.parent ? activeTab.parent.getChildren() :
                          Groups.getOrphanedTabs());
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

  
  
  
  
  _createGroupOnDrag: function(e){
    const minSize = 60;
    const minMinSize = 15;

    var startPos = { x: e.clientX, y: e.clientY };
    var phantom = iQ("<div>")
      .addClass("group phantom")
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
      setOpacity: function FauxItem_setOpacity( opacity ) {
        this.container.css("opacity", opacity);
      },
      
      
      pushAway: function () {},
    };
    item.setBounds(new Rect(startPos.y, startPos.x, 0, 0));

    var dragOutInfo = new Drag(item, e, true); 

    function updateSize(e){
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
      if (box.width > minMinSize && box.height > minMinSize
          && (box.width > minSize || box.height > minSize))
        item.setOpacity(1);
      else
        item.setOpacity(0.7);

      e.preventDefault();
    }

    function collapse() {
      phantom.animate({
        width: 0,
        height: 0,
        top: phantom.position().top + phantom.height()/2,
        left: phantom.position().left + phantom.width()/2
      }, {
        duration: 300,
        complete: function() {
          phantom.remove();
        }
      });
    }

    function finalize(e) {
      iQ(window).unbind("mousemove", updateSize);
      dragOutInfo.stop();
      if (phantom.css("opacity") != 1)
        collapse();
      else {
        var bounds = item.getBounds();

        
        
        var tabs = Groups.getOrphanedTabs();
        var insideTabs = [];
        for each(tab in tabs) {
          if (bounds.contains(tab.bounds))
            insideTabs.push(tab);
        }

        var group = new Group(insideTabs,{bounds:bounds});
        phantom.remove();
        dragOutInfo = null;
      }
    }

    iQ(window).mousemove(updateSize)
    iQ(gWindow).one("mouseup", finalize);
    e.preventDefault();
    return false;
  },

  
  
  
  
  
  
  _resize: function(force) {
    if (typeof(force) == "undefined")
      force = false;

    
    
    if (!force && !this._isTabCandyVisible())
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

    Groups.repositionNewTabGroup(); 

    if (newPageBounds.width < this._pageBounds.width &&
        newPageBounds.width > itemBounds.width)
      newPageBounds.width = this._pageBounds.width;

    if (newPageBounds.height < this._pageBounds.height &&
        newPageBounds.height > itemBounds.height)
      newPageBounds.height = this._pageBounds.height;

    var wScale;
    var hScale;
    if ( Math.abs(newPageBounds.width - this._pageBounds.width)
         > Math.abs(newPageBounds.height - this._pageBounds.height) ) {
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

  
  
  
  _addDevMenu: function() {
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
        name: "refresh",
        code: function() {
          location.href = "tabcandy.html";
        }
      }, {
        name: "reset",
        code: function() {
          self._reset();
        }
      }, {
        name: "save",
        code: function() {
          self._saveAll();
        }
      }, {
        name: "group sites",
        code: function() {
          self._arrangeBySite();
        }
      }];

      var count = commands.length;
      var a;
      for (a = 0; a < count; a++) {
        commands[a].element = iQ("<option>")
          .val(a)
          .html(commands[a].name)
          .appendTo($select)
          .get(0);
      }
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  _reset: function() {
    Storage.wipe();
    location.href = "";
  },

  
  
  
  _storageSanity: function(data) {
    if (Utils.isEmptyObject(data))
      return true;

    if (!Utils.isRect(data.pageBounds)) {
      Utils.log("UI.storageSanity: bad pageBounds", data.pageBounds);
      data.pageBounds = null;
      return false;
    }

    return true;
  },

  
  
  
  _save: function() {
    if (!this._initialized)
      return;

    var data = {
      tabCandyVisible: this._isTabCandyVisible(),
      pageBounds: this._pageBounds
    };

    if (this._storageSanity(data))
      Storage.saveUIData(gWindow, data);
  },

  
  
  
  
  _saveAll: function() {
    this._save();
    Groups.saveAll();
    TabItems.saveAll();
  },

  
  
  
  
  _arrangeBySite: function() {
    function putInGroup(set, key) {
      var group = Groups.getGroupWithTitle(key);
      if (group) {
        set.forEach(function(el) {
          group.add(el);
        });
      } else
        new Group(set, { dontPush: true, dontArrange: true, title: key });
    }

    Groups.removeAll();

    var newTabsGroup = Groups.getNewTabGroup();
    var groups = [];
    var items = TabItems.getItems();
    items.forEach(function(item) {
      var url = item.tab.linkedBrowser.currentURI.spec;
      var domain = url.split('/')[2];

      if (!domain)
        newTabsGroup.add(item);
      else {
        var domainParts = domain.split(".");
        var mainDomain = domainParts[domainParts.length - 2];
        if (groups[mainDomain])
          groups[mainDomain].push(item.container);
        else
          groups[mainDomain] = [item.container];
      }
    });

    var leftovers = [];
    for (key in groups) {
      var set = groups[key];
      if (set.length > 1) {
        putInGroup(set, key);
      } else
        leftovers.push(set[0]);
    }
    putInGroup(leftovers, "mixed");

    Groups.arrange();
  },
};


window.UI = UIManager;
window.UI.init();

})();
