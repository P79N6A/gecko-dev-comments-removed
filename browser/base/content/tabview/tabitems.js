



















































function TabItem(tab, options) {
  Utils.assert(tab, "tab");

  this.tab = tab;
  
  this.tab._tabViewTabItem = this;

  if (!options)
    options = {};

  
  var $div = iQ('<div>')
    .addClass('tab')
    .html("<div class='thumb'>" +
          "<img class='cached-thumb' style='display:none'/><canvas moz-opaque/></div>" +
          "<div class='favicon'><img/></div>" +
          "<span class='tab-title'>&nbsp;</span>"
    )
    .appendTo('body');

  this._cachedImageData = null;
  this.shouldHideCachedData = false;
  this.canvasSizeForced = false;
  this.$thumb = iQ('.thumb', $div);
  this.$fav   = iQ('.favicon', $div);
  this.$tabTitle = iQ('.tab-title', $div);
  this.$canvas = iQ('.thumb canvas', $div);
  this.$cachedThumb = iQ('img.cached-thumb', $div);
  this.$favImage = iQ('.favicon>img', $div);

  iQ("<div>")
    .addClass('close')
    .appendTo($div);
  this.$close = iQ('.close', $div);

  iQ("<div>")
    .addClass('expander')
    .appendTo($div);

  this.tabCanvas = new TabCanvas(this.tab, this.$canvas[0]);

  this.defaultSize = new Point(TabItems.tabWidth, TabItems.tabHeight);
  this.locked = {};
  this._hidden = false;
  this.isATabItem = true;
  this._zoomPrep = false;
  this.sizeExtra = new Point();
  this.keepProportional = true;
  this._hasBeenDrawn = false;
  this._reconnected = false;
  this.isStacked = false;

  var self = this;

  this.isDragging = false;

  this.sizeExtra.x = parseInt($div.css('padding-left'))
      + parseInt($div.css('padding-right'));

  this.sizeExtra.y = parseInt($div.css('padding-top'))
      + parseInt($div.css('padding-bottom'));

  this.bounds = $div.bounds();

  this._lastTabUpdateTime = Date.now();

  
  this._init($div[0]);

  
  
  
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

    
    if (iQ(e.target).hasClass("close") || e.button == 1) {
      self.close();
    } else {
      if (!Items.item(this).isDragging)
        self.zoomIn();
    }
  });

  this.setResizable(true, options.immediately);
  this.droppable(true);
  this._updateDebugBounds();

  TabItems.register(this);

  
  if (!TabItems.reconnectingPaused())
    this._reconnect();
};

TabItem.prototype = Utils.extend(new Item(), new Subscribable(), {
  
  
  
  
  forceCanvasSize: function TabItem_forceCanvasSize(w, h) {
    this.canvasSizeForced = true;
    this.$canvas[0].width = w;
    this.$canvas[0].height = h;
    this.tabCanvas.paint();
  },

  
  
  
  _getFontSizeFromWidth: function TabItem__getFontSizeFromWidth(width) {
    let widthRange = new Range(0,TabItems.tabWidth);
    let proportion = widthRange.proportion(width-this.sizeExtra.x, true); 
    return TabItems.fontSizeRange.scale(proportion);
  },

  
  
  
  
  
  
  unforceCanvasSize: function TabItem_unforceCanvasSize() {
    this.canvasSizeForced = false;
  },

  
  
  
  
  isShowingCachedData: function() {
    return (this._cachedImageData != null);
  },

  
  
  
  
  
  
  
  showCachedData: function TabItem_showCachedData(tabData) {
    if (!this._cachedImageData) {
      TabItems.cachedDataCounter++;
      this.tab.linkedBrowser._tabViewTabItemWithCachedData = this;
      if (TabItems.cachedDataCounter == 1)
        gBrowser.addTabsProgressListener(TabItems.tabsProgressListener);
    }
    this._cachedImageData = tabData.imageData;
    this.$cachedThumb.attr("src", this._cachedImageData).show();
    this.$canvas.css({opacity: 0.0});
    this.$tabTitle.text(tabData.title ? tabData.title : "");
  },

  
  
  
  hideCachedData: function TabItem_hideCachedData() {
    this.$cachedThumb.hide();
    this.$canvas.css({opacity: 1.0});
    if (this._cachedImageData) {
      TabItems.cachedDataCounter--;
      this._cachedImageData = null;
      this.tab.linkedBrowser._tabViewTabItemWithCachedData = null;
      if (TabItems.cachedDataCounter == 0)
        gBrowser.removeTabsProgressListener(TabItems.tabsProgressListener);
    }
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
    
    let tabData = Storage.getTabData(this.tab);
    if (tabData && TabItems.storageSanity(tabData)) {
      if (this.parent)
        this.parent.remove(this, {immediately: true});

      this.setBounds(tabData.bounds, true);

      if (Utils.isPoint(tabData.userSize))
        this.userSize = new Point(tabData.userSize);

      if (tabData.groupID) {
        var groupItem = GroupItems.groupItem(tabData.groupID);
        if (groupItem) {
          groupItem.add(this, {immediately: true});

          
          
          if (this.tab == gBrowser.selectedTab || 
              (!GroupItems.getActiveGroupItem() && !this.tab.hidden))
            GroupItems.setActiveGroupItem(this.parent);
        }
      }

      if (tabData.imageData)
        this.showCachedData(tabData);
    } else {
      
      if (!TabItems.creatingNewOrphanTab)
        GroupItems.newTab(this, {immediately: true});
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

    if (this._zoomPrep)
      this.bounds.copy(rect);
    else {
      var css = {};

      if (rect.left != this.bounds.left || options.force)
        css.left = rect.left;

      if (rect.top != this.bounds.top || options.force)
        css.top = rect.top;

      if (rect.width != this.bounds.width || options.force) {
        css.width = rect.width - this.sizeExtra.x;
        css.fontSize = this._getFontSizeFromWidth(rect.width);
        css.fontSize += 'px';
      }

      if (rect.height != this.bounds.height || options.force) {
        if (!this.isStacked)
          css.height = rect.height - this.sizeExtra.y - TabItems.fontSizeRange.max;
        else
          css.height = rect.height - this.sizeExtra.y;
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

      if (css.fontSize && !this.isStacked) {
        if (css.fontSize < TabItems.fontSizeRange.min)
          immediately ? this.$tabTitle.hide() : this.$tabTitle.fadeOut();
        else
          immediately ? this.$tabTitle.show() : this.$tabTitle.fadeIn();
      }

      if (css.width) {
        TabItems.update(this.tab);

        let widthRange, proportion;

        if (this.isStacked) {
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
    }

    UI.clearShouldResizeItems();

    this._updateDebugBounds();
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

  
  
  
  
  
  close: function TabItem_close() {
    
    
    
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

    var self = this;
    var $tabEl = this.$container;
    var childHitResult = { shouldZoom: true };
    if (this.parent)
      childHitResult = this.parent.childHit(this);

    if (childHitResult.shouldZoom) {
      
      var tab = this.tab;
      var orig = $tabEl.bounds();

      function onZoomDone() {
        UI.goToTab(tab);

        
        
        if (tab != gBrowser.selectedTab) {
          UI.onTabSelect(gBrowser.selectedTab);
        } else { 
          if (isNewBlankTab)
            gWindow.gURLBar.focus();
        }
        if (childHitResult.callback)
          childHitResult.callback();
      }

      let animateZoom = gPrefBranch.getBoolPref("animate_zoom");
      if (animateZoom) {
        TabItems.pausePainting();
        $tabEl.addClass("front")
        .animate(this.getZoomRect(), {
          duration: 230,
          easing: 'fast',
          complete: function() {
            onZoomDone();

            setTimeout(function() {
              TabItems.resumePainting();

              $tabEl
                .css(orig)
                .removeClass("front");
            }, 0);
          }
        });
      } else {
        setTimeout(onZoomDone, 0);
      } 
    }
  },

  
  
  
  
  
  
  
  zoomOut: function TabItem_zoomOut(complete) {
    var $tab = this.$container;
    var self = this;
    
    let onZoomDone = function onZoomDone() {
      self.setZoomPrep(false);

      GroupItems.setActiveOrphanTab(null);

      if (typeof complete == "function")
        complete();
    };
    
    let animateZoom = gPrefBranch.getBoolPref("animate_zoom");
    if (animateZoom) {
      let box = this.getBounds();
      box.width -= this.sizeExtra.x;
      if (!this.isStacked)
        box.height -= this.sizeExtra.y + TabItems.fontSizeRange.max;
      else
        box.height -= this.sizeExtra.y;
  
      TabItems.pausePainting();
      $tab.animate({
        left: box.left,
        top: box.top,
        width: box.width,
        height: box.height
      }, {
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

  
  
  
  
  
  
  getZoomRect: function TabItem_getZoomRect(scaleCheat) {
    let $tabEl = iQ(this.container);
    let orig = $tabEl.bounds();
    
    
    
    
    
    
    if (!scaleCheat)
      scaleCheat = 1.7;

    let zoomWidth = orig.width + (window.innerWidth - orig.width) / scaleCheat;
    return {
      top:    orig.top    * (1 - 1/scaleCheat),
      left:   orig.left   * (1 - 1/scaleCheat),
      width:  zoomWidth,
      height: orig.height * zoomWidth / orig.width
    };
  },

  
  
  
  
  
  setZoomPrep: function TabItem_setZoomPrep(value) {
    let animateZoom = gPrefBranch.getBoolPref("animate_zoom");

    var $div = this.$container;

    if (value && animateZoom) {
      this._zoomPrep = true;

      
      
      
      
      
      
      
      

      $div
        .addClass('front')
        .css(this.getZoomRect(2));
    } else {
      let box = this.getBounds();

      this._zoomPrep = false;
      $div.removeClass('front');

      this.setBounds(box, true, {force: true});
    }
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
  items: [],
  paintingPaused: 0,
  cachedDataCounter: 0,  
  tabsProgressListener: null,
  _tabsWaitingForUpdate: [],
  _heartbeat: null, 
  _heartbeatTiming: 100, 
  _lastUpdateTime: Date.now(),
  _eventListeners: [],
  _pauseUpdateForTest: false,
  creatingNewOrphanTab: false,
  tempCanvas: null,
  _reconnectingPaused: false,

  
  
  
  init: function TabItems_init() {
    Utils.assert(window.AllTabs, "AllTabs must be initialized first");
    let self = this;
    
    this.minTabHeight = this.minTabWidth * this.tabHeight / this.tabWidth;
    this.tabAspect = this.tabHeight / this.tabWidth;
    this.invTabAspect = 1 / this.tabAspect;

    let $canvas = iQ("<canvas>")
      .attr('moz-opaque', '');
    $canvas.appendTo(iQ("body"));
    $canvas.hide();
    this.tempCanvas = $canvas[0];
    
    
    this.tempCanvas.width = 150;
    this.tempCanvas.height = 150;

    this.tabsProgressListener = {
      onStateChange: function(browser, webProgress, request, stateFlags, status) {
        if ((stateFlags & Ci.nsIWebProgressListener.STATE_STOP) &&
            (stateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)) {
          
          
          if (browser._tabViewTabItemWithCachedData)
            browser._tabViewTabItemWithCachedData.shouldHideCachedData = true;
        }
      }
    };

    
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
    if (this.tabsProgressListener)
      gBrowser.removeTabsProgressListener(this.tabsProgressListener);

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
    this._tabsWaitingForUpdate = null;
  },

  
  
  
  update: function TabItems_update(tab) {
    try {
      Utils.assertThrow(tab, "tab");
      Utils.assertThrow(!tab.pinned, "shouldn't be an app tab");
      Utils.assertThrow(tab._tabViewTabItem, "should already be linked");

      let shouldDefer = (
        this.isPaintingPaused() ||
        this._tabsWaitingForUpdate.length ||
        Date.now() - this._lastUpdateTime < this._heartbeatTiming
      );

      if (shouldDefer) {
        if (this._tabsWaitingForUpdate.indexOf(tab) == -1)
          this._tabsWaitingForUpdate.push(tab);
        this.startHeartbeat();
      } else
        this._update(tab);
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  _update: function TabItems__update(tab) {
    try {
      if (this._pauseUpdateForTest)
        return;

      Utils.assertThrow(tab, "tab");

      
      let index = this._tabsWaitingForUpdate.indexOf(tab);
      if (index != -1)
        this._tabsWaitingForUpdate.splice(index, 1);

      
      Utils.assertThrow(tab._tabViewTabItem, "must already be linked");
      let tabItem = tab._tabViewTabItem;

      
      if (this.shouldLoadFavIcon(tab.linkedBrowser)) {
        let iconUrl = tab.image;
        if (!iconUrl)
          iconUrl = Utils.defaultFaviconURL;

        if (iconUrl != tabItem.$favImage[0].src)
          tabItem.$favImage[0].src = iconUrl;

        iQ(tabItem.$fav[0]).show();
      } else {
        if (tabItem.$favImage[0].hasAttribute("src"))
          tabItem.$favImage[0].removeAttribute("src");
        iQ(tabItem.$fav[0]).hide();
      }

      
      let tabUrl = tab.linkedBrowser.currentURI.spec;
      if (tabUrl != tabItem.url) {
        let oldURL = tabItem.url;
        tabItem.url = tabUrl;
        tabItem.save();
      }

      
      let label = tab.label;
      let $name = tabItem.$tabTitle;
      if (!tabItem.isShowingCachedData() && $name.text() != label)
        $name.text(label);

      
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

      
      if (tabItem.isShowingCachedData() && tabItem.shouldHideCachedData)
        tabItem.hideCachedData();
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
      

      if (tab._tabViewTabItem == GroupItems.getActiveOrphanTab())
        GroupItems.setActiveOrphanTab(null);

      this.unregister(tab._tabViewTabItem);
      tab._tabViewTabItem._sendToSubscribers("close");
      tab._tabViewTabItem.$container.remove();
      tab._tabViewTabItem.removeTrenches();
      Items.unsquish(null, tab._tabViewTabItem);

      tab._tabViewTabItem = null;
      Storage.saveTab(tab, null);

      let index = this._tabsWaitingForUpdate.indexOf(tab);
      if (index != -1)
        this._tabsWaitingForUpdate.splice(index, 1);
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

    if (this._tabsWaitingForUpdate.length && UI.isIdle()) {
      this._update(this._tabsWaitingForUpdate[0]);
      
    }

    if (this._tabsWaitingForUpdate.length) {
      this.startHeartbeat();
    }
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
  
  
  
  
  
  
  _getWidthForHeight: function TabItems__getWidthForHeight(height, options) {    
    let titleSize = (options !== undefined && options.hideTitle === true) ? 
      0 : TabItems.fontSizeRange.max;
    return Math.max(0, Math.max(TabItems.minTabHeight, height - titleSize)) * 
      TabItems.invTabAspect;
  },

  
  
  
  
  
  _getHeightForWidth: function TabItems__getHeightForWidth(width, options) {
    let titleSize = (options !== undefined && options.hideTitle === true) ? 
      0 : TabItems.fontSizeRange.max;
    return Math.max(0, Math.max(TabItems.minTabWidth,width)) *
      TabItems.tabAspect + titleSize;
  },
  
  
  
  
  
  calcValidSize: function TabItems_calcValidSize(size, options) {
    Utils.assert(Utils.isPoint(size), 'input is a Point');
    let retSize = new Point(0,0);
    if (size.x==-1) {
      retSize.x = this._getWidthForHeight(size.y, options);
      retSize.y = size.y;
    } else if (size.y==-1) {
      retSize.x = size.x;
      retSize.y = this._getHeightForWidth(size.x, options);
    } else {
      let fitHeight = this._getHeightForWidth(size.x, options);
      let fitWidth = this._getWidthForHeight(size.y, options);

      
      if (fitWidth < size.x) {
        retSize.x = fitWidth;
        retSize.y = size.y;
      } else {
        retSize.x = size.x;
        retSize.y = fitHeight;
      }
    }
    return retSize;
  }
};





function TabCanvas(tab, canvas) {
  this.init(tab, canvas);
};

TabCanvas.prototype = {
  
  
  init: function TabCanvas_init(tab, canvas) {
    this.tab = tab;
    this.canvas = canvas;

    var $canvas = iQ(canvas);
    var w = $canvas.width();
    var h = $canvas.height();
    canvas.width = w;
    canvas.height = h;
  },

  
  
  paint: function TabCanvas_paint(evt) {
    var w = this.canvas.width;
    var h = this.canvas.height;
    if (!w || !h)
      return;

    let fromWin = this.tab.linkedBrowser.contentWindow;
    if (fromWin == null) {
      Utils.log('null fromWin in paint');
      return;
    }

    let tempCanvas = TabItems.tempCanvas;
    if (w < tempCanvas.width) {
      
      
      
      
      var tempCtx = tempCanvas.getContext("2d");
      
      let canvW = tempCanvas.width;
      let canvH = (h/w) * canvW;
      
      var scaler = canvW/fromWin.innerWidth;
  
      tempCtx.save();
      tempCtx.clearRect(0,0,tempCanvas.width,tempCanvas.height);
      tempCtx.scale(scaler, scaler);
      try{
        tempCtx.drawWindow(fromWin, fromWin.scrollX, fromWin.scrollY, 
          canvW/scaler, canvH/scaler, "#fff");
      } catch(e) {
        Utils.error('paint', e);
      }  
      tempCtx.restore();
      
      
      var destCtx = this.canvas.getContext("2d");      
      try{
        
        destCtx.drawImage(tempCanvas, 0, 0, w, w);
      } catch(e) {
        Utils.error('paint', e);
      }  
      
    } else {
      
      
      
      var ctx = this.canvas.getContext("2d");
      
      var scaler = w/fromWin.innerWidth;
  
      
  
      ctx.save();
      ctx.scale(scaler, scaler);
      try{
        ctx.drawWindow(fromWin, fromWin.scrollX, fromWin.scrollY, 
          w/scaler, h/scaler, "#fff",
          Ci.nsIDOMCanvasRenderingContext2D.DRAWWINDOW_DO_NOT_FLUSH);
      } catch(e) {
        Utils.error('paint', e);
      }
  
      ctx.restore();
    }
  },

  
  
  toImageData: function TabCanvas_toImageData() {
    return this.canvas.toDataURL("image/png", "");
  }
};
