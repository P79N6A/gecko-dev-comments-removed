



















































function TabItem(tab, options) {
  Utils.assert(tab, "tab");

  this.tab = tab;
  
  this.tab._tabViewTabItem = this;

  if (!options)
    options = {};

  
  document.body.appendChild(TabItems.fragment().cloneNode(true));
  
  
  
  let div = document.body.lastChild;
  let $div = iQ(div);

  this._cachedImageData = null;
  this.canvasSizeForced = false;
  this.$thumb = iQ('.thumb', $div);
  this.$fav   = iQ('.favicon', $div);
  this.$tabTitle = iQ('.tab-title', $div);
  this.$canvas = iQ('.thumb canvas', $div);
  this.$cachedThumb = iQ('img.cached-thumb', $div);
  this.$favImage = iQ('.favicon>img', $div);
  this.$close = iQ('.close', $div);

  this.tabCanvas = new TabCanvas(this.tab, this.$canvas[0]);

  this.defaultSize = new Point(TabItems.tabWidth, TabItems.tabHeight);
  this._hidden = false;
  this.isATabItem = true;
  this.keepProportional = true;
  this._hasBeenDrawn = false;
  this._reconnected = false;
  this.isStacked = false;
  this.url = "";

  var self = this;

  this.isDragging = false;

  
  
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
    var $target = this.$container;
    this.isDropTarget = false;

    var phantom = $target.data("phantomGroupItem");

    var groupItem = drag.info.item.parent;
    if (groupItem) {
      groupItem.add(drag.info.$el);
    } else {
      phantom.removeClass("phantom acceptsDrop");
      new GroupItem([$target, drag.info.$el], {container:phantom, bounds:phantom.bounds()});
    }
  };

  this.dropOptions.over = function(e) {
    var $target = this.$container;
    this.isDropTarget = true;

    $target.removeClass("acceptsDrop");

    var phantomMargin = 40;

    var groupItemBounds = this.getBounds();
    groupItemBounds.inset(-phantomMargin, -phantomMargin);

    iQ(".phantom").remove();
    var phantom = iQ("<div>")
      .addClass("groupItem phantom acceptsDrop")
      .css({
        position: "absolute",
        zIndex: -99
      })
      .css(groupItemBounds)
      .hide()
      .appendTo("body");

    var defaultRadius = Trenches.defaultRadius;
    
    
    Trenches.defaultRadius = phantomMargin + 1;
    var updatedBounds = drag.info.snapBounds(groupItemBounds,'none');
    Trenches.defaultRadius = defaultRadius;

    
    if (updatedBounds)
      phantom.css(updatedBounds);

    phantom.fadeIn();

    $target.data("phantomGroupItem", phantom);
  };

  this.dropOptions.out = function(e) {
    this.isDropTarget = false;
    var phantom = this.$container.data("phantomGroupItem");
    if (phantom) {
      phantom.fadeOut(function() {
        iQ(this).remove();
      });
    }
  };

  this.draggable();

  
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

  this.setResizable(true, options.immediately);
  this.droppable(true);

  TabItems.register(this);

  
  if (!TabItems.reconnectingPaused())
    this._reconnect();
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

  
  
  
  
  isShowingCachedData: function() {
    return (this._cachedImageData != null);
  },

  
  
  
  
  
  
  
  showCachedData: function TabItem_showCachedData(tabData) {
    this._cachedImageData = tabData.imageData;
    this.$cachedThumb.attr("src", this._cachedImageData).show();
    this.$canvas.css({opacity: 0.0});
    this.$tabTitle.text(tabData.title ? tabData.title : "");
  },

  
  
  
  hideCachedData: function TabItem_hideCachedData() {
    this.$cachedThumb.hide();
    this.$canvas.css({opacity: 1.0});
    if (this._cachedImageData)
      this._cachedImageData = null;
  },

  
  
  
  
  
  
  getStorageData: function TabItem_getStorageData(getImageData) {
    let imageData = null;

    if (getImageData) { 
      if (this._cachedImageData)
        imageData = this._cachedImageData;
      else if (this.tabCanvas)
        imageData = this.tabCanvas.toImageData();
    }

    return {
      bounds: this.getBounds(),
      userSize: (Utils.isPoint(this.userSize) ? new Point(this.userSize) : null),
      url: this.tab.linkedBrowser.currentURI.spec,
      groupID: (this.parent ? this.parent.id : 0),
      imageData: imageData,
      title: getImageData && this.tab.label || null
    };
  },

  
  
  
  
  
  
  save: function TabItem_save(saveImageData) {
    try{
      if (!this.tab || this.tab.parentNode == null || !this._reconnected) 
        return;

      var data = this.getStorageData(saveImageData);
      if (TabItems.storageSanity(data))
        Storage.saveTab(this.tab, data);
    } catch(e) {
      Utils.log("Error in saving tab value: "+e);
    }
  },

  
  
  
  
  _reconnect: function TabItem__reconnect() {
    Utils.assertThrow(!this._reconnected, "shouldn't already be reconnected");
    Utils.assertThrow(this.tab, "should have a xul:tab");

    let tabData = null;
    let self = this;
    let imageDataCb = function(imageData) {
      Utils.assertThrow(tabData, "tabData");
      
      tabData.imageData = imageData;

      let currentUrl = self.tab.linkedBrowser.currentURI.spec;
      
      
      
      if (tabData.imageData &&
          (tabData.url == currentUrl || currentUrl == 'about:blank')) {
        self.showCachedData(tabData);
      }
    };
    
    
    tabData = Storage.getTabData(this.tab, imageDataCb);
    if (tabData && TabItems.storageSanity(tabData)) {
      if (self.parent)
        self.parent.remove(self, {immediately: true});

      self.setBounds(tabData.bounds, true);

      if (Utils.isPoint(tabData.userSize))
        self.userSize = new Point(tabData.userSize);

      if (tabData.groupID) {
        var groupItem = GroupItems.groupItem(tabData.groupID);
        if (groupItem) {
          groupItem.add(self, {immediately: true});

          
          
          if (self.tab == gBrowser.selectedTab ||
              (!GroupItems.getActiveGroupItem() && !self.tab.hidden))
            UI.setActive(self.parent);
        }
      } else {
        
        
        
        
        
        
        
        
        
        
        if (GroupItems.getActiveGroupItem() == null)
          GroupItems.newTab(self, {immediately: true});
      }
    } else {
      
      if (!TabItems.creatingNewOrphanTab)
        GroupItems.newTab(self, {immediately: true});
    }

    self._reconnected = true;
    self.save();
    self._sendToSubscribers("reconnected");
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
    if (!Utils.isRect(inRect)) {
      Utils.trace('TabItem.setBounds: rect is not a real rectangle!', inRect);
      return;
    }

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

    if (!Utils.isRect(this.bounds))
      Utils.trace('TabItem.setBounds: this.bounds is not a real rectangle!', this.bounds);

    if (!this.parent && this.tab.parentNode != null)
      this.setTrenches(rect);

    this.save();
  },

  
  
  
  setZ: function TabItem_setZ(value) {
    this.zIndex = value;
    this.$container.css({zIndex: value});
  },

  
  
  
  
  
  
  
  close: function TabItem_close(groupClose) {
    
    
    
    if (!groupClose && gBrowser.tabs.length == 1) {
      if (this.tab._tabViewTabItem.parent) {
        group = this.tab._tabViewTabItem.parent;
      } else {
        let emptyGroups = GroupItems.groupItems.filter(function (groupItem) {
          return (!groupItem.getChildren().length);
        });
        group = (emptyGroups.length ? emptyGroups[0] : GroupItems.newGroup());
      }
      group.newTab();
    }
    
    
    
    gBrowser.removeTab(this.tab);
    let tabNotClosed = 
      Array.some(gBrowser.tabs, function(tab) { return tab == this.tab; }, this);
    if (!tabNotClosed)
      this._sendToSubscribers("tabRemoved");

    
    
    return !tabNotClosed;
  },

  
  
  
  addClass: function TabItem_addClass(className) {
    this.$container.addClass(className);
  },

  
  
  
  removeClass: function TabItem_removeClass(className) {
    this.$container.removeClass(className);
  },

  
  
  
  
  setResizable: function TabItem_setResizable(value, immediately) {
    var $resizer = iQ('.expander', this.container);

    if (value) {
      this.resizeOptions.minWidth = TabItems.minTabWidth;
      this.resizeOptions.minHeight = TabItems.minTabHeight;
      immediately ? $resizer.show() : $resizer.fadeIn();
      this.resizable(true);
    } else {
      immediately ? $resizer.hide() : $resizer.fadeOut();
      this.resizable(false);
    }
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

    UI.setActive(this);
    TabItems._update(this.tab, {force: true});

    
    let tab = this.tab;

    function onZoomDone() {
      $canvas.css({ '-moz-transform': null });
      $tabEl.removeClass("front");

      UI.goToTab(tab);

      
      
      if (tab != gBrowser.selectedTab) {
        UI.onTabSelect(gBrowser.selectedTab);
      } else { 
        if (isNewBlankTab)
          gWindow.gURLBar.focus();
      }
      if (self.parent && self.parent.expanded)
        self.parent.collapse();
    }

    let animateZoom = gPrefBranch.getBoolPref("animate_zoom");
    if (animateZoom) {
      let transform = this.getZoomTransform();
      TabItems.pausePainting();

      if (this.parent && this.parent.expanded)
        $tabEl.removeClass("stack-trayed");
      $tabEl.addClass("front");
      $canvas
        .css({ '-moz-transform-origin': transform.transformOrigin })
        .animate({ '-moz-transform': transform.transform }, {
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
      $canvas.css("-moz-transform", null);

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
        '-moz-transform': transform.transform,
        '-moz-transform-origin': transform.transformOrigin
      });

      $canvas.animate({ "-moz-transform": "scale(1.0)" }, {
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
  creatingNewOrphanTab: false,
  tempCanvas: null,
  _reconnectingPaused: false,
  tabItemPadding: {},

  
  
  
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
    this.tempCanvas = $canvas[0];
    
    
    this.tempCanvas.width = 150;
    this.tempCanvas.height = 112;

    
    this._eventListeners["open"] = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow || tab.pinned)
        return;

      self.link(tab);
    }
    
    
    this._eventListeners["attrModified"] = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow || tab.pinned)
        return;

      self.update(tab);
    }
    
    this._eventListeners["close"] = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow || tab.pinned)
        return;

      
      if (!UI.isDOMWindowClosing)
        self.unlink(tab);
    }
    for (let name in this._eventListeners) {
      AllTabs.register(name, this._eventListeners[name]);
    }

    
    AllTabs.tabs.forEach(function(tab) {
      if (tab.ownerDocument.defaultView != gWindow || tab.pinned)
        return;

      self.link(tab, {immediately: true});
      self.update(tab);
    });
  },

  
  
  uninit: function TabItems_uninit() {
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
            "<div class='close'></div>" +
            "<div class='expander'></div>";
    this._fragment = document.createDocumentFragment();
    this._fragment.appendChild(div);

    return this._fragment;
  },

  
  
  
  isComplete: function TabItems_update(tab) {
    
    
    
    
    Utils.assertThrow(tab, "tab");
    return (
      tab.linkedBrowser.contentDocument.readyState == 'complete' &&
      !(tab.linkedBrowser.contentDocument.URL == 'about:blank' &&
        tab._tabViewTabItem.url != 'about:blank')
    );
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

      

      
      if (this.shouldLoadFavIcon(tab.linkedBrowser)) {
        let iconUrl = gFavIconService.getFaviconImageForPage(
                        tab.linkedBrowser.currentURI).spec;
        if (tabItem.$favImage[0].src != iconUrl)
          tabItem.$favImage[0].src = iconUrl;

        iQ(tabItem.$fav[0]).show();
      } else {
        if (tabItem.$favImage[0].hasAttribute("src"))
          tabItem.$favImage[0].removeAttribute("src");
        iQ(tabItem.$fav[0]).hide();
      }

      
      let label = tab.label;
      let $name = tabItem.$tabTitle;
      if ($name.text() != label)
        $name.text(label);

      
      
      this._tabsWaitingForUpdate.remove(tab);

      
      let tabUrl = tab.linkedBrowser.currentURI.spec;
      if (tabUrl != tabItem.url) {
        let oldURL = tabItem.url;
        tabItem.url = tabUrl;
        tabItem.save();
      }

      
      if (!this.isComplete(tab) && (!options || !options.force)) {
        
        this._tabsWaitingForUpdate.push(tab);
        return;
      }

      
      let $canvas = tabItem.$canvas;
      if (!tabItem.canvasSizeForced) {
        let w = $canvas.width();
        let h = $canvas.height();
        if (w != tabItem.$canvas[0].width || h != tabItem.$canvas[0].height) {
          tabItem.$canvas[0].width = w;
          tabItem.$canvas[0].height = h;
        }
      }

      this._lastUpdateTime = Date.now();
      tabItem._lastTabUpdateTime = this._lastUpdateTime;

      tabItem.tabCanvas.paint();

      
      if (tabItem.isShowingCachedData())
        tabItem.hideCachedData();

      
      tabItem._sendToSubscribers("updated");
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  shouldLoadFavIcon: function TabItems_shouldLoadFavIcon(browser) {
    return !(browser.contentDocument instanceof window.ImageDocument) &&
           gBrowser.shouldLoadFavIcon(browser.contentDocument.documentURIObject);
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
      

      if (tab._tabViewTabItem == UI.getActiveOrphanTab())
        UI.setActive(null, { onlyRemoveActiveTab: true });

      this.unregister(tab._tabViewTabItem);
      tab._tabViewTabItem._sendToSubscribers("close");
      tab._tabViewTabItem.$container.remove();
      tab._tabViewTabItem.removeTrenches();
      Items.unsquish(null, tab._tabViewTabItem);

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

  
  
  
  
  
  
  saveAll: function TabItems_saveAll(saveImageData) {
    var items = this.getItems();
    items.forEach(function(item) {
      item.save(saveImageData);
    });
  },

  
  
  
  
  
  storageSanity: function TabItems_storageSanity(data) {
    var sane = true;
    if (!Utils.isRect(data.bounds)) {
      Utils.log('TabItems.storageSanity: bad bounds', data.bounds);
      sane = false;
    }

    return sane;
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

TabCanvas.prototype = {
  
  
  
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

    let ctx = this.canvas.getContext("2d");
    let tempCanvas = TabItems.tempCanvas;
    let bgColor = '#fff';

    if (w < tempCanvas.width) {
      
      
      
      let tempCtx = tempCanvas.getContext("2d");
      this._drawWindow(tempCtx, tempCanvas.width, tempCanvas.height, bgColor);

      
      try {
        this._fillCanvasBackground(ctx, w, h, bgColor);
        ctx.drawImage(tempCanvas, 0, 0, w, h);
      } catch (e) {
        Utils.error('paint', e);
      }
    } else {
      
      
      this._drawWindow(ctx, w, h, bgColor);
    }
  },

  
  
  
  
  _fillCanvasBackground: function TabCanvas__fillCanvasBackground(ctx, width, height, bgColor) {
    ctx.fillStyle = bgColor;
    ctx.fillRect(0, 0, width, height);
  },

  
  
  
  _drawWindow: function TabCanvas__drawWindow(ctx, width, height, bgColor) {
    this._fillCanvasBackground(ctx, width, height, bgColor);

    let rect = this._calculateClippingRect(width, height);
    let scaler = width / rect.width;

    ctx.save();
    ctx.scale(scaler, scaler);

    try {
      let win = this.tab.linkedBrowser.contentWindow;
      ctx.drawWindow(win, rect.left, rect.top, rect.width, rect.height,
                     bgColor, ctx.DRAWWINDOW_DO_NOT_FLUSH);
    } catch (e) {
      Utils.error('paint', e);
    }

    ctx.restore();
  },

  
  
  
  
  _calculateClippingRect: function TabCanvas__calculateClippingRect(origWidth, origHeight) {
    let win = this.tab.linkedBrowser.contentWindow;

    
    
    let maxWidth = Math.max(1, win.innerWidth - 25);
    let maxHeight = win.innerHeight;

    let height = Math.min(maxHeight, Math.floor(origHeight * maxWidth / origWidth));
    let width = Math.floor(origWidth * height / origHeight);

    
    
    let factor = 0.7;
    if (width < maxWidth * factor) {
      width = maxWidth * factor;
      height = Math.floor(origHeight * width / origWidth);
    }

    let left = win.scrollX + Math.max(0, Math.round((maxWidth - width) / 2));
    let top = win.scrollY;

    return new Rect(left, top, width, height);
  },

  
  
  toImageData: function TabCanvas_toImageData() {
    return this.canvas.toDataURL("image/png", "");
  }
};
