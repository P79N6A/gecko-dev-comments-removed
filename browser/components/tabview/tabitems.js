












function TabItem(tab, options) {
  Utils.assert(tab, "tab");

  this.tab = tab;
  
  this.tab._tabViewTabItem = this;

  if (!options)
    options = {};

  
  document.body.appendChild(TabItems.fragment().cloneNode(true));
  
  
  
  let div = document.body.lastChild;
  let $div = iQ(div);

  this._showsCachedData = false;
  this.canvasSizeForced = false;
  this.$thumb = iQ('.thumb', $div);
  this.$fav   = iQ('.favicon', $div);
  this.$tabTitle = iQ('.tab-title', $div);
  this.$canvas = iQ('.thumb canvas', $div);
  this.$cachedThumb = iQ('img.cached-thumb', $div);
  this.$favImage = iQ('.favicon>img', $div);
  this.$close = iQ('.close', $div);

  this.tabCanvas = new TabCanvas(this.tab, this.$canvas[0]);

  this._hidden = false;
  this.isATabItem = true;
  this.keepProportional = true;
  this._hasBeenDrawn = false;
  this._reconnected = false;
  this.isDragging = false;
  this.isStacked = false;

  
  
  if (Utils.isEmptyObject(TabItems.tabItemPadding)) {
    TabItems.tabItemPadding.x = parseInt($div.css('padding-left'))
        + parseInt($div.css('padding-right'));

    TabItems.tabItemPadding.y = parseInt($div.css('padding-top'))
        + parseInt($div.css('padding-bottom'));
  }
  
  this.bounds = new Rect(0,0,1,1);

  this._lastTabUpdateTime = Date.now();

  
  this._init(div);

  
  
  this.dropOptions.drop = function(e) {
    let groupItem = drag.info.item.parent;
    groupItem.add(drag.info.$el);
  };

  this.draggable();

  let self = this;

  
  $div.mousedown(function(e) {
    if (!Utils.isRightClick(e))
      self.lastMouseDownTarget = e.target;
  });

  $div.mouseup(function(e) {
    var same = (e.target == self.lastMouseDownTarget);
    self.lastMouseDownTarget = null;
    if (!same)
      return;

    
    if (iQ(e.target).hasClass("close") || Utils.isMiddleClick(e)) {
      self.closedManually = true;
      self.close();
    } else {
      if (!Items.item(this).isDragging)
        self.zoomIn();
    }
  });

  this.droppable(true);

  this.$close.attr("title", tabbrowserString("tabs.closeTab"));

  TabItems.register(this);

  
  if (!TabItems.reconnectingPaused())
    this._reconnect(options);
};

TabItem.prototype = Utils.extend(new Item(), new Subscribable(), {
  
  
  
  toString: function TabItem_toString() {
    return "[TabItem (" + this.tab + ")]";
  },

  
  
  
  
  forceCanvasSize: function TabItem_forceCanvasSize(w, h) {
    this.canvasSizeForced = true;
    this.$canvas[0].width = w;
    this.$canvas[0].height = h;
    this.tabCanvas.paint();
  },

  
  
  
  
  
  
  unforceCanvasSize: function TabItem_unforceCanvasSize() {
    this.canvasSizeForced = false;
  },

  
  
  
  
  isShowingCachedData: function TabItem_isShowingCachedData() {
    return this._showsCachedData;
  },

  
  
  
  
  showCachedData: function TabItem_showCachedData() {
    let {title, url} = this.getTabState();
    let thumbnailURL = gPageThumbnails.getThumbnailURL(url);

    this.$cachedThumb.attr("src", thumbnailURL).show();
    this.$canvas.css({opacity: 0});

    let tooltip = (title && title != url ? title + "\n" + url : url);
    this.$tabTitle.text(title).attr("title", tooltip);
    this._showsCachedData = true;
  },

  
  
  
  hideCachedData: function TabItem_hideCachedData() {
    this.$cachedThumb.attr("src", "").hide();
    this.$canvas.css({opacity: 1.0});
    this._showsCachedData = false;
  },

  
  
  
  getStorageData: function TabItem_getStorageData() {
    let data = {
      groupID: (this.parent ? this.parent.id : 0)
    };
    if (this.parent && this.parent.getActiveTab() == this)
      data.active = true;

    return data;
  },

  
  
  
  save: function TabItem_save() {
    try {
      if (!this.tab || !Utils.isValidXULTab(this.tab) || !this._reconnected) 
        return;

      let data = this.getStorageData();
      if (TabItems.storageSanity(data))
        Storage.saveTab(this.tab, data);
    } catch(e) {
      Utils.log("Error in saving tab value: "+e);
    }
  },

  
  
  
  _getCurrentTabStateEntry: function TabItem__getCurrentTabStateEntry() {
    let tabState = Storage.getTabState(this.tab);

    if (tabState) {
      let index = (tabState.index || tabState.entries.length) - 1;
      if (index in tabState.entries)
        return tabState.entries[index];
    }

    return null;
  },

  
  
  
  
  getTabState: function TabItem_getTabState() {
    let entry = this._getCurrentTabStateEntry();
    let title = "";
    let url = "";

    if (entry) {
      if (entry.title)
        title = entry.title;

      url = entry.url;
    } else {
      url = this.tab.linkedBrowser.currentURI.spec;
    }

    return {title: title, url: url};
  },

  
  
  
  
  
  
  
  
  
  
  
  _reconnect: function TabItem__reconnect(options) {
    Utils.assertThrow(!this._reconnected, "shouldn't already be reconnected");
    Utils.assertThrow(this.tab, "should have a xul:tab");

    let tabData = Storage.getTabData(this.tab);
    let groupItem;

    if (tabData && TabItems.storageSanity(tabData)) {
      
      
      this.showCachedData();

      if (this.parent)
        this.parent.remove(this, {immediately: true});

      if (tabData.groupID)
        groupItem = GroupItems.groupItem(tabData.groupID);
      else
        groupItem = new GroupItem([], {immediately: true, bounds: tabData.bounds});

      if (groupItem) {
        groupItem.add(this, {immediately: true});

        
        if (tabData.active)
          groupItem.setActiveTab(this);

        
        
        if (this.tab.selected ||
            (!GroupItems.getActiveGroupItem() && !this.tab.hidden))
          UI.setActive(this.parent);
      }
    } else {
      if (options && options.groupItemId)
        groupItem = GroupItems.groupItem(options.groupItemId);

      if (groupItem) {
        groupItem.add(this, {immediately: true});
      } else {
        
        GroupItems.newTab(this, {immediately: true});
      }
    }

    this._reconnected = true;
    this.save();
    this._sendToSubscribers("reconnected");
  },

  
  
  
  setHidden: function TabItem_setHidden(val) {
    if (val)
      this.addClass("tabHidden");
    else
      this.removeClass("tabHidden");
    this._hidden = val;
  },

  
  
  
  getHidden: function TabItem_getHidden() {
    return this._hidden;
  },

  
  
  
  
  
  
  
  
  
  
  
  setBounds: function TabItem_setBounds(inRect, immediately, options) {
    Utils.assert(Utils.isRect(inRect), 'TabItem.setBounds: rect is not a real rectangle!');

    if (!options)
      options = {};

    
    let validSize = TabItems.calcValidSize(
      new Point(inRect.width, inRect.height), 
      {hideTitle: (this.isStacked || options.hideTitle === true)});
    let rect = new Rect(inRect.left, inRect.top, 
      validSize.x, validSize.y);

    var css = {};

    if (rect.left != this.bounds.left || options.force)
      css.left = rect.left;

    if (rect.top != this.bounds.top || options.force)
      css.top = rect.top;

    if (rect.width != this.bounds.width || options.force) {
      css.width = rect.width - TabItems.tabItemPadding.x;
      css.fontSize = TabItems.getFontSizeFromWidth(rect.width);
      css.fontSize += 'px';
    }

    if (rect.height != this.bounds.height || options.force) {
      css.height = rect.height - TabItems.tabItemPadding.y;
      if (!this.isStacked)
        css.height -= TabItems.fontSizeRange.max;
    }

    if (Utils.isEmptyObject(css))
      return;

    this.bounds.copy(rect);

    
    
    
    if (immediately || (!this._hasBeenDrawn)) {
      this.$container.css(css);
    } else {
      TabItems.pausePainting();
      this.$container.animate(css, {
          duration: 200,
        easing: "tabviewBounce",
        complete: function() {
          TabItems.resumePainting();
        }
      });
    }

    if (css.fontSize && !(this.parent && this.parent.isStacked())) {
      if (css.fontSize < TabItems.fontSizeRange.min)
        immediately ? this.$tabTitle.hide() : this.$tabTitle.fadeOut();
      else
        immediately ? this.$tabTitle.show() : this.$tabTitle.fadeIn();
    }

    if (css.width) {
      TabItems.update(this.tab);

      let widthRange, proportion;

      if (this.parent && this.parent.isStacked()) {
        if (UI.rtl) {
          this.$fav.css({top:0, right:0});
        } else {
          this.$fav.css({top:0, left:0});
        }
        widthRange = new Range(70, 90);
        proportion = widthRange.proportion(css.width); 
      } else {
        if (UI.rtl) {
          this.$fav.css({top:4, right:2});
        } else {
          this.$fav.css({top:4, left:4});
        }
        widthRange = new Range(40, 45);
        proportion = widthRange.proportion(css.width); 
      }

      if (proportion <= .1)
        this.$close.hide();
      else
        this.$close.show().css({opacity:proportion});

      var pad = 1 + 5 * proportion;
      var alphaRange = new Range(0.1,0.2);
      this.$fav.css({
       "-moz-padding-start": pad + "px",
       "-moz-padding-end": pad + 2 + "px",
       "padding-top": pad + "px",
       "padding-bottom": pad + "px",
       "border-color": "rgba(0,0,0,"+ alphaRange.scale(proportion) +")",
      });
    }

    this._hasBeenDrawn = true;

    UI.clearShouldResizeItems();

    rect = this.getBounds(); 

    Utils.assert(Utils.isRect(this.bounds), 'TabItem.setBounds: this.bounds is not a real rectangle!');

    if (!this.parent && Utils.isValidXULTab(this.tab))
      this.setTrenches(rect);

    this.save();
  },

  
  
  
  setZ: function TabItem_setZ(value) {
    this.zIndex = value;
    this.$container.css({zIndex: value});
  },

  
  
  
  
  
  
  
  close: function TabItem_close(groupClose) {
    
    
    
    if (!groupClose && gBrowser.tabs.length == 1) {
      let group = this.tab._tabViewTabItem.parent;
      group.newTab(null, { closedLastTab: true });
    }

    
    
    
    gBrowser.removeTab(this.tab);
    let tabClosed = !this.tab;

    if (tabClosed)
      this._sendToSubscribers("tabRemoved");

    
    
    return tabClosed;
  },

  
  
  
  addClass: function TabItem_addClass(className) {
    this.$container.addClass(className);
  },

  
  
  
  removeClass: function TabItem_removeClass(className) {
    this.$container.removeClass(className);
  },

  
  
  
  makeActive: function TabItem_makeActive() {
    this.$container.addClass("focus");

    if (this.parent)
      this.parent.setActiveTab(this);
  },

  
  
  
  makeDeactive: function TabItem_makeDeactive() {
    this.$container.removeClass("focus");
  },

  
  
  
  
  
  
  zoomIn: function TabItem_zoomIn(isNewBlankTab) {
    
    if (this.parent && this.parent.hidden)
      return;

    let self = this;
    let $tabEl = this.$container;
    let $canvas = this.$canvas;

    Search.hide();

    UI.setActive(this);
    TabItems._update(this.tab, {force: true});

    
    let tab = this.tab;

    function onZoomDone() {
      $canvas.css({ 'transform': null });
      $tabEl.removeClass("front");

      UI.goToTab(tab);

      
      
      if (!tab.selected) {
        UI.onTabSelect(gBrowser.selectedTab);
      } else {
        if (isNewBlankTab)
          gWindow.gURLBar.focus();
      }
      if (self.parent && self.parent.expanded)
        self.parent.collapse();

      self._sendToSubscribers("zoomedIn");
    }

    let animateZoom = gPrefBranch.getBoolPref("animate_zoom");
    if (animateZoom) {
      let transform = this.getZoomTransform();
      TabItems.pausePainting();

      if (this.parent && this.parent.expanded)
        $tabEl.removeClass("stack-trayed");
      $tabEl.addClass("front");
      $canvas
        .css({ 'transform-origin': transform.transformOrigin })
        .animate({ 'transform': transform.transform }, {
          duration: 230,
          easing: 'fast',
          complete: function() {
            onZoomDone();

            setTimeout(function() {
              TabItems.resumePainting();
            }, 0);
          }
        });
    } else {
      setTimeout(onZoomDone, 0);
    }
  },

  
  
  
  
  
  
  
  zoomOut: function TabItem_zoomOut(complete) {
    let $tab = this.$container, $canvas = this.$canvas;
    var self = this;
    
    let onZoomDone = function onZoomDone() {
      $tab.removeClass("front");
      $canvas.css("transform", null);

      if (typeof complete == "function")
        complete();
    };

    UI.setActive(this);
    TabItems._update(this.tab, {force: true});

    $tab.addClass("front");

    let animateZoom = gPrefBranch.getBoolPref("animate_zoom");
    if (animateZoom) {
      
      
      let transform = this.getZoomTransform(2);
      TabItems.pausePainting();

      $canvas.css({
        'transform': transform.transform,
        'transform-origin': transform.transformOrigin
      });

      $canvas.animate({ "transform": "scale(1.0)" }, {
        duration: 300,
        easing: 'cubic-bezier', 
        complete: function() {
          TabItems.resumePainting();
          onZoomDone();
        }
      });
    } else {
      onZoomDone();
    }
  },

  
  
  
  
  getZoomTransform: function TabItem_getZoomTransform(scaleCheat) {
    
    
    let { left, top, width, height, right, bottom } = this.$container.bounds();

    let { innerWidth: windowWidth, innerHeight: windowHeight } = window;

    
    
    
    
    
    
    
    

    if (!scaleCheat)
      scaleCheat = 1.7;

    let zoomWidth = width + (window.innerWidth - width) / scaleCheat;
    let zoomScaleFactor = zoomWidth / width;

    let zoomHeight = height * zoomScaleFactor;
    let zoomTop = top * (1 - 1/scaleCheat);
    let zoomLeft = left * (1 - 1/scaleCheat);

    let xOrigin = (left - zoomLeft) / ((left - zoomLeft) + (zoomLeft + zoomWidth - right)) * 100;
    let yOrigin = (top - zoomTop) / ((top - zoomTop) + (zoomTop + zoomHeight - bottom)) * 100;

    return {
      transformOrigin: xOrigin + "% " + yOrigin + "%",
      transform: "scale(" + zoomScaleFactor + ")"
    };
  },

  
  
  
  updateCanvas: function TabItem_updateCanvas() {
    
    let $canvas = this.$canvas;
    if (!this.canvasSizeForced) {
      let w = $canvas.width();
      let h = $canvas.height();
      if (w != $canvas[0].width || h != $canvas[0].height) {
        $canvas[0].width = w;
        $canvas[0].height = h;
      }
    }

    TabItems._lastUpdateTime = Date.now();
    this._lastTabUpdateTime = TabItems._lastUpdateTime;

    if (this.tabCanvas)
      this.tabCanvas.paint();

    
    if (this.isShowingCachedData())
      this.hideCachedData();
  }
});




let TabItems = {
  minTabWidth: 40,
  tabWidth: 160,
  tabHeight: 120,
  tabAspect: 0, 
  invTabAspect: 0, 
  fontSize: 9,
  fontSizeRange: new Range(8,15),
  _fragment: null,
  items: [],
  paintingPaused: 0,
  _tabsWaitingForUpdate: null,
  _heartbeat: null, 
  _heartbeatTiming: 200, 
  _maxTimeForUpdating: 200, 
  _lastUpdateTime: Date.now(),
  _eventListeners: [],
  _pauseUpdateForTest: false,
  _reconnectingPaused: false,
  tabItemPadding: {},
  _mozAfterPaintHandler: null,

  
  
  
  toString: function TabItems_toString() {
    return "[TabItems count=" + this.items.length + "]";
  },

  
  
  
  init: function TabItems_init() {
    Utils.assert(window.AllTabs, "AllTabs must be initialized first");
    let self = this;
    
    
    this._tabsWaitingForUpdate = new TabPriorityQueue();
    this.minTabHeight = this.minTabWidth * this.tabHeight / this.tabWidth;
    this.tabAspect = this.tabHeight / this.tabWidth;
    this.invTabAspect = 1 / this.tabAspect;

    let $canvas = iQ("<canvas>")
      .attr('moz-opaque', '');
    $canvas.appendTo(iQ("body"));
    $canvas.hide();

    let mm = gWindow.messageManager;
    this._mozAfterPaintHandler = this.onMozAfterPaint.bind(this);
    mm.addMessageListener("Panorama:MozAfterPaint", this._mozAfterPaintHandler);

    
    this._eventListeners.open = function (event) {
      let tab = event.target;

      if (!tab.pinned)
        self.link(tab);
    }
    
    
    this._eventListeners.attrModified = function (event) {
      let tab = event.target;

      if (!tab.pinned)
        self.update(tab);
    }
    
    this._eventListeners.close = function (event) {
      let tab = event.target;

      
      if (!tab.pinned && !UI.isDOMWindowClosing)
        self.unlink(tab);
    }
    for (let name in this._eventListeners) {
      AllTabs.register(name, this._eventListeners[name]);
    }

    let activeGroupItem = GroupItems.getActiveGroupItem();
    let activeGroupItemId = activeGroupItem ? activeGroupItem.id : null;
    
    AllTabs.tabs.forEach(function (tab) {
      if (tab.pinned)
        return;

      let options = {immediately: true};
      
      
      
      
      if (!tab.hidden && activeGroupItemId)
         options.groupItemId = activeGroupItemId;
      self.link(tab, options);
      self.update(tab);
    });
  },

  
  
  uninit: function TabItems_uninit() {
    let mm = gWindow.messageManager;
    mm.removeMessageListener("Panorama:MozAfterPaint", this._mozAfterPaintHandler);

    for (let name in this._eventListeners) {
      AllTabs.unregister(name, this._eventListeners[name]);
    }
    this.items.forEach(function(tabItem) {
      for (let x in tabItem) {
        if (typeof tabItem[x] == "object")
          tabItem[x] = null;
      }
    });

    this.items = null;
    this._eventListeners = null;
    this._lastUpdateTime = null;
    this._tabsWaitingForUpdate.clear();
  },

  
  
  
  
  
  fragment: function TabItems_fragment() {
    if (this._fragment)
      return this._fragment;

    let div = document.createElement("div");
    div.classList.add("tab");
    div.innerHTML = "<div class='thumb'>" +
            "<img class='cached-thumb' style='display:none'/><canvas moz-opaque/></div>" +
            "<div class='favicon'><img/></div>" +
            "<span class='tab-title'>&nbsp;</span>" +
            "<div class='close'></div>";
    this._fragment = document.createDocumentFragment();
    this._fragment.appendChild(div);

    return this._fragment;
  },

  
  
  
  _isComplete: function TabItems__isComplete(tab, callback) {
    Utils.assertThrow(tab, "tab");

    let mm = tab.linkedBrowser.messageManager;
    let message = "Panorama:isDocumentLoaded";

    mm.addMessageListener(message, function onMessage(cx) {
      mm.removeMessageListener(cx.name, onMessage);
      callback(cx.json.isLoaded);
    });
    mm.sendAsyncMessage(message);
  },

  
  
  
  onMozAfterPaint: function TabItems_onMozAfterPaint(cx) {
    let index = gBrowser.browsers.indexOf(cx.target);
    if (index == -1)
      return;

    let tab = gBrowser.tabs[index];
    if (!tab.pinned)
      this.update(tab);
  },

  
  
  
  update: function TabItems_update(tab) {
    try {
      Utils.assertThrow(tab, "tab");
      Utils.assertThrow(!tab.pinned, "shouldn't be an app tab");
      Utils.assertThrow(tab._tabViewTabItem, "should already be linked");

      let shouldDefer = (
        this.isPaintingPaused() ||
        this._tabsWaitingForUpdate.hasItems() ||
        Date.now() - this._lastUpdateTime < this._heartbeatTiming
      );

      if (shouldDefer) {
        this._tabsWaitingForUpdate.push(tab);
        this.startHeartbeat();
      } else
        this._update(tab);
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  
  
  
  
  
  
  
  _update: function TabItems__update(tab, options) {
    try {
      if (this._pauseUpdateForTest)
        return;

      Utils.assertThrow(tab, "tab");

      
      Utils.assertThrow(tab._tabViewTabItem, "must already be linked");
      let tabItem = tab._tabViewTabItem;

      
      
      FavIcons.getFavIconUrlForTab(tab, function TabItems__update_getFavIconUrlCallback(iconUrl) {
        let favImage = tabItem.$favImage[0];
        let fav = tabItem.$fav;
        if (iconUrl) {
          if (favImage.src != iconUrl)
            favImage.src = iconUrl;
          fav.show();
        } else {
          if (favImage.hasAttribute("src"))
            favImage.removeAttribute("src");
          fav.hide();
        }
        tabItem._sendToSubscribers("iconUpdated");
      });

      
      let label = tab.label;
      let $name = tabItem.$tabTitle;
      if ($name.text() != label)
        $name.text(label);

      
      
      this._tabsWaitingForUpdate.remove(tab);

      
      let tabUrl = tab.linkedBrowser.currentURI.spec;
      let tooltip = (label == tabUrl ? label : label + "\n" + tabUrl);
      tabItem.$container.attr("title", tooltip);

      
      if (options && options.force) {
        tabItem.updateCanvas();
        tabItem._sendToSubscribers("updated");
      } else {
        this._isComplete(tab, function TabItems__update_isComplete(isComplete) {
          if (!Utils.isValidXULTab(tab) || tab.pinned)
            return;

          if (isComplete) {
            tabItem.updateCanvas();
            tabItem._sendToSubscribers("updated");
          } else {
            this._tabsWaitingForUpdate.push(tab);
          }
        }.bind(this));
      }
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  link: function TabItems_link(tab, options) {
    try {
      Utils.assertThrow(tab, "tab");
      Utils.assertThrow(!tab.pinned, "shouldn't be an app tab");
      Utils.assertThrow(!tab._tabViewTabItem, "shouldn't already be linked");
      new TabItem(tab, options); 
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  unlink: function TabItems_unlink(tab) {
    try {
      Utils.assertThrow(tab, "tab");
      Utils.assertThrow(tab._tabViewTabItem, "should already be linked");
      

      this.unregister(tab._tabViewTabItem);
      tab._tabViewTabItem._sendToSubscribers("close");
      tab._tabViewTabItem.$container.remove();
      tab._tabViewTabItem.removeTrenches();
      Items.unsquish(null, tab._tabViewTabItem);

      tab._tabViewTabItem.tab = null;
      tab._tabViewTabItem.tabCanvas.tab = null;
      tab._tabViewTabItem.tabCanvas = null;
      tab._tabViewTabItem = null;
      Storage.saveTab(tab, null);

      this._tabsWaitingForUpdate.remove(tab);
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  handleTabPin: function TabItems_handleTabPin(xulTab) {
    this.unlink(xulTab);
  },

  
  
  handleTabUnpin: function TabItems_handleTabUnpin(xulTab) {
    this.link(xulTab);
    this.update(xulTab);
  },

  
  
  
  
  
  
  
  startHeartbeat: function TabItems_startHeartbeat() {
    if (!this._heartbeat) {
      let self = this;
      this._heartbeat = setTimeout(function() {
        self._checkHeartbeat();
      }, this._heartbeatTiming);
    }
  },

  
  
  
  
  
  _checkHeartbeat: function TabItems__checkHeartbeat() {
    this._heartbeat = null;

    if (this.isPaintingPaused())
      return;

    
    if (!UI.isIdle()) {
      this.startHeartbeat();
      return;
    }

    let accumTime = 0;
    let items = this._tabsWaitingForUpdate.getItems();
    
    
    while (accumTime < this._maxTimeForUpdating && items.length) {
      let updateBegin = Date.now();
      this._update(items.pop());
      let updateEnd = Date.now();

      
      
      
      let deltaTime = updateEnd - updateBegin;
      accumTime += deltaTime;
    }

    if (this._tabsWaitingForUpdate.hasItems())
      this.startHeartbeat();
  },

  
  
  
  
  
  
  pausePainting: function TabItems_pausePainting() {
    this.paintingPaused++;
    if (this._heartbeat) {
      clearTimeout(this._heartbeat);
      this._heartbeat = null;
    }
  },

  
  
  
  
  
  resumePainting: function TabItems_resumePainting() {
    this.paintingPaused--;
    Utils.assert(this.paintingPaused > -1, "paintingPaused should not go below zero");
    if (!this.isPaintingPaused())
      this.startHeartbeat();
  },

  
  
  
  
  isPaintingPaused: function TabItems_isPaintingPaused() {
    return this.paintingPaused > 0;
  },

  
  
  
  pauseReconnecting: function TabItems_pauseReconnecting() {
    Utils.assertThrow(!this._reconnectingPaused, "shouldn't already be paused");

    this._reconnectingPaused = true;
  },
  
  
  
  
  resumeReconnecting: function TabItems_resumeReconnecting() {
    Utils.assertThrow(this._reconnectingPaused, "should already be paused");

    this._reconnectingPaused = false;
    this.items.forEach(function(item) {
      if (!item._reconnected)
        item._reconnect();
    });
  },
  
  
  
  
  reconnectingPaused: function TabItems_reconnectingPaused() {
    return this._reconnectingPaused;
  },
  
  
  
  
  register: function TabItems_register(item) {
    Utils.assert(item && item.isAnItem, 'item must be a TabItem');
    Utils.assert(this.items.indexOf(item) == -1, 'only register once per item');
    this.items.push(item);
  },

  
  
  
  unregister: function TabItems_unregister(item) {
    var index = this.items.indexOf(item);
    if (index != -1)
      this.items.splice(index, 1);
  },

  
  
  
  getItems: function TabItems_getItems() {
    return Utils.copy(this.items);
  },

  
  
  
  saveAll: function TabItems_saveAll() {
    let tabItems = this.getItems();

    tabItems.forEach(function TabItems_saveAll_forEach(tabItem) {
      tabItem.save();
    });
  },

  
  
  
  
  
  storageSanity: function TabItems_storageSanity(data) {
    return true;
  },

  
  
  
  getFontSizeFromWidth: function TabItem_getFontSizeFromWidth(width) {
    let widthRange = new Range(0, TabItems.tabWidth);
    let proportion = widthRange.proportion(width - TabItems.tabItemPadding.x, true);
    
    return TabItems.fontSizeRange.scale(proportion);
  },

  
  
  
  _getWidthForHeight: function TabItems__getWidthForHeight(height) {
    return height * TabItems.invTabAspect;
  },

  
  
  
  _getHeightForWidth: function TabItems__getHeightForWidth(width) {
    return width * TabItems.tabAspect;
  },

  
  
  
  
  calcValidSize: function TabItems_calcValidSize(size, options) {
    Utils.assert(Utils.isPoint(size), 'input is a Point');

    let width = Math.max(TabItems.minTabWidth, size.x);
    let showTitle = !options || !options.hideTitle;
    let titleSize = showTitle ? TabItems.fontSizeRange.max : 0;
    let height = Math.max(TabItems.minTabHeight, size.y - titleSize);
    let retSize = new Point(width, height);

    if (size.x > -1)
      retSize.y = this._getHeightForWidth(width);
    if (size.y > -1)
      retSize.x = this._getWidthForHeight(height);

    if (size.x > -1 && size.y > -1) {
      if (retSize.x < size.x)
        retSize.y = this._getHeightForWidth(retSize.x);
      else
        retSize.x = this._getWidthForHeight(retSize.y);
    }

    if (showTitle)
      retSize.y += titleSize;

    return retSize;
  }
};








function TabPriorityQueue() {
};

TabPriorityQueue.prototype = {
  _low: [], 
  _high: [], 

  
  
  
  toString: function TabPriorityQueue_toString() {
    return "[TabPriorityQueue count=" + (this._low.length + this._high.length) + "]";
  },

  
  
  
  clear: function TabPriorityQueue_clear() {
    this._low = [];
    this._high = [];
  },

  
  
  
  hasItems: function TabPriorityQueue_hasItems() {
    return (this._low.length > 0) || (this._high.length > 0);
  },

  
  
  
  getItems: function TabPriorityQueue_getItems() {
    return this._low.concat(this._high);
  },

  
  
  
  push: function TabPriorityQueue_push(tab) {
    
    
    
    
    
    
    let item = tab._tabViewTabItem;
    if (item.parent && (item.parent.isStacked() &&
      !item.parent.isTopOfStack(item) &&
      !item.parent.expanded)) {
      let idx = this._high.indexOf(tab);
      if (idx != -1) {
        this._high.splice(idx, 1);
        this._low.unshift(tab);
      } else if (this._low.indexOf(tab) == -1)
        this._low.unshift(tab);
    } else {
      let idx = this._low.indexOf(tab);
      if (idx != -1) {
        this._low.splice(idx, 1);
        this._high.unshift(tab);
      } else if (this._high.indexOf(tab) == -1)
        this._high.unshift(tab);
    }
  },

  
  
  
  pop: function TabPriorityQueue_pop() {
    let ret = null;
    if (this._high.length)
      ret = this._high.pop();
    else if (this._low.length)
      ret = this._low.pop();
    return ret;
  },

  
  
  
  peek: function TabPriorityQueue_peek() {
    let ret = null;
    if (this._high.length)
      ret = this._high[this._high.length-1];
    else if (this._low.length)
      ret = this._low[this._low.length-1];
    return ret;
  },

  
  
  
  remove: function TabPriorityQueue_remove(tab) {
    let index = this._high.indexOf(tab);
    if (index != -1)
      this._high.splice(index, 1);
    else {
      index = this._low.indexOf(tab);
      if (index != -1)
        this._low.splice(index, 1);
    }
  }
};





function TabCanvas(tab, canvas) {
  this.tab = tab;
  this.canvas = canvas;
};

TabCanvas.prototype = Utils.extend(new Subscribable(), {
  
  
  
  toString: function TabCanvas_toString() {
    return "[TabCanvas (" + this.tab + ")]";
  },

  
  
  paint: function TabCanvas_paint(evt) {
    var w = this.canvas.width;
    var h = this.canvas.height;
    if (!w || !h)
      return;

    if (!this.tab.linkedBrowser.contentWindow) {
      Utils.log('no tab.linkedBrowser.contentWindow in TabCanvas.paint()');
      return;
    }

    let win = this.tab.linkedBrowser.contentWindow;
    gPageThumbnails.captureToCanvas(win, this.canvas);

    this._sendToSubscribers("painted");
  },

  
  
  toImageData: function TabCanvas_toImageData() {
    return this.canvas.toDataURL("image/png");
  }
});
