

















































window.TabItem = function(tab) {

  Utils.assert(tab, "tab");

  this.tab = tab;
  
  this.tab.tabItem = this;

  
  var $div = iQ('<div>')
    .addClass('tab')
    .html("<div class='thumb'><div class='thumb-shadow'></div>" +
          "<img class='cached-thumb' style='display:none'/><canvas/></div>" +
          "<div class='favicon'><img/></div>" +
          "<span class='tab-title'>&nbsp;</span>"
    )
    .appendTo('body');

  this.canvasSizeForced = false;
  this.isShowingCachedData = false;
  this.favEl = (iQ('.favicon>img', $div))[0];
  this.nameEl = (iQ('.tab-title', $div))[0];
  this.canvasEl = (iQ('.thumb canvas', $div))[0];
  this.cachedThumbEl = (iQ('img.cached-thumb', $div))[0];

  this.tabCanvas = new TabCanvas(this.tab, this.canvasEl);

  this.defaultSize = new Point(TabItems.tabWidth, TabItems.tabHeight);
  this.locked = {};
  this.isATabItem = true;
  this._zoomPrep = false;
  this.sizeExtra = new Point();
  this.keepProportional = true;

  var self = this;

  this.isDragging = false;

  this.sizeExtra.x = parseInt($div.css('padding-left'))
      + parseInt($div.css('padding-right'));

  this.sizeExtra.y = parseInt($div.css('padding-top'))
      + parseInt($div.css('padding-bottom'));

  this.bounds = $div.bounds();

  
  this._init($div[0]);

  
  
  
  this.dropOptions.drop = function(e) {
    var $target = iQ(this.container);
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
    var $target = iQ(this.container);
    this.isDropTarget = true;

    $target.removeClass("acceptsDrop");

    var phantomMargin = 40;

    var groupItemBounds = this.getBoundsWithTitle();
    groupItemBounds.inset(-phantomMargin, -phantomMargin);

    iQ(".phantom").remove();
    var phantom = iQ("<div>")
      .addClass("groupItem phantom acceptsDrop")
      .css({
        position: "absolute",
        zIndex: -99
      })
      .css(groupItemBounds.css())
      .hide()
      .appendTo("body");

    var defaultRadius = Trenches.defaultRadius;
    
    
    Trenches.defaultRadius = phantomMargin + 1;
    var updatedBounds = drag.info.snapBounds(groupItemBounds,'none');
    Trenches.defaultRadius = defaultRadius;

    
    if (updatedBounds)
      phantom.css(updatedBounds.css());

    phantom.fadeIn();

    $target.data("phantomGroupItem", phantom);
  };

  this.dropOptions.out = function(e) {
    this.isDropTarget = false;
    var phantom = iQ(this.container).data("phantomGroupItem");
    if (phantom) {
      phantom.fadeOut(function() {
        iQ(this).remove();
      });
    }
  };

  this.draggable();
  this.droppable(true);

  
  $div.mousedown(function(e) {
    if (!Utils.isRightClick(e))
      self.lastMouseDownTarget = e.target;
  });

  $div.mouseup(function(e) {
    var same = (e.target == self.lastMouseDownTarget);
    self.lastMouseDownTarget = null;
    if (!same)
      return;

    if (iQ(e.target).hasClass("close"))
      self.close();
    else {
      if (!Items.item(this).isDragging)
        self.zoomIn();
    }
  });

  iQ("<div>")
    .addClass('close')
    .appendTo($div);

  iQ("<div>")
    .addClass('expander')
    .appendTo($div);

  
  this.reconnected = false;
  this._hasBeenDrawn = false;
  this.setResizable(true);

  this._updateDebugBounds();

  TabItems.register(this);

  if (!TabItems.reconnect(this))
    GroupItems.newTab(this);
};

window.TabItem.prototype = Utils.extend(new Item(), new Subscribable(), {
  
  
  
  
  forceCanvasSize: function(w, h) {
    this.canvasSizeForced = true;
    this.canvasEl.width = w;
    this.canvasEl.height = h;
    this.tabCanvas.paint();
  },

  
  
  
  
  
  
  unforceCanvasSize: function() {
    this.canvasSizeForced = false;
  },

  
  
  
  
  showCachedData: function(tabData) {
    this.isShowingCachedData = true;
    var $nameElement = iQ(this.nameEl);
    var $canvasElement = iQ(this.canvasEl);
    var $cachedThumbElement = iQ(this.cachedThumbEl);
    $cachedThumbElement.attr("src", tabData.imageData).show();
    $canvasElement.css({opacity: 0.0});
    $nameElement.text(tabData.title ? tabData.title : "");
  },

  
  
  
  hideCachedData: function() {
    var $canvasElement = iQ(this.canvasEl);
    var $cachedThumbElement = iQ(this.cachedThumbEl);
    $cachedThumbElement.hide();
    $canvasElement.css({opacity: 1.0});
    this.isShowingCachedData = false;
  },

  
  
  
  
  
  
  getStorageData: function(getImageData) {
    return {
      bounds: this.getBounds(),
      userSize: (Utils.isPoint(this.userSize) ? new Point(this.userSize) : null),
      url: this.tab.linkedBrowser.currentURI.spec,
      groupID: (this.parent ? this.parent.id : 0),
      imageData: (getImageData && this.tabCanvas ?
                  this.tabCanvas.toImageData() : null),
      title: getImageData && this.tab.label || null
    };
  },

  
  
  
  
  
  
  save: function(saveImageData) {
    try{
      if (!this.tab || this.tab.parentNode == null || !this.reconnected) 
        return;

      var data = this.getStorageData(saveImageData);
      if (TabItems.storageSanity(data))
        Storage.saveTab(this.tab, data);
    } catch(e) {
      Utils.log("Error in saving tab value: "+e);
    }
  },

  
  
  
  
  
  
  
  
  
  
  
  setBounds: function(rect, immediately, options) {
    if (!Utils.isRect(rect)) {
      Utils.trace('TabItem.setBounds: rect is not a real rectangle!', rect);
      return;
    }

    if (!options)
      options = {};

    if (this._zoomPrep)
      this.bounds.copy(rect);
    else {
      var $container = iQ(this.container);
      var $title = iQ('.tab-title', $container);
      var $thumb = iQ('.thumb', $container);
      var $close = iQ('.close', $container);
      var $fav   = iQ('.favicon', $container);
      var css = {};

      const fontSizeRange = new Range(8,15);

      if (rect.left != this.bounds.left || options.force)
        css.left = rect.left;

      if (rect.top != this.bounds.top || options.force)
        css.top = rect.top;

      if (rect.width != this.bounds.width || options.force) {
        css.width = rect.width - this.sizeExtra.x;
        let widthRange = new Range(0,TabItems.tabWidth);
        let proportion = widthRange.proportion(css.width, true); 

        css.fontSize = fontSizeRange.scale(proportion); 
        css.fontSize += 'px';
      }

      if (rect.height != this.bounds.height || options.force)
        css.height = rect.height - this.sizeExtra.y;

      if (Utils.isEmptyObject(css))
        return;

      this.bounds.copy(rect);

      
      
      
      if (immediately || (!this._hasBeenDrawn)) {
        $container.css(css);
      } else {
        TabItems.pausePainting();
        $container.animate(css, {
          duration: 200,
          easing: "tabviewBounce",
          complete: function() {
            TabItems.resumePainting();
          }
        });
      }

      if (css.fontSize && !this.inStack()) {
        if (css.fontSize < fontSizeRange.min)
          $title.fadeOut();
        else
          $title.fadeIn();
      }

      if (css.width) {
        TabItems.update(this.tab);

        let widthRange, proportion;

        if (this.inStack()) {
          $fav.css({top:0, left:0});
          widthRange = new Range(70, 90);
          proportion = widthRange.proportion(css.width); 
        } else {
          $fav.css({top:4,left:4});
          widthRange = new Range(60, 70);
          proportion = widthRange.proportion(css.width); 
          $close.show().css({opacity:proportion});
          if (proportion <= .1)
            $close.hide()
        }

        var pad = 1 + 5 * proportion;
        var alphaRange = new Range(0.1,0.2);
        $fav.css({
         "padding-left": pad + "px",
         "padding-right": pad + 2 + "px",
         "padding-top": pad + "px",
         "padding-bottom": pad + "px",
         "border-color": "rgba(0,0,0,"+ alphaRange.scale(proportion) +")",
        });
      }

      this._hasBeenDrawn = true;
    }

    this._updateDebugBounds();
    rect = this.getBounds(); 

    if (!Utils.isRect(this.bounds))
      Utils.trace('TabItem.setBounds: this.bounds is not a real rectangle!', this.bounds);

    if (!this.parent && this.tab.parentNode != null)
      this.setTrenches(rect);

    this.save();
  },

  
  
  
  getBoundsWithTitle: function() {
    var b = this.getBounds();
    var $title = iQ(this.container).find('.tab-title');
    var height = b.height;
    if ( Utils.isNumber($title.height()) )
      height += $title.height();
    return new Rect(b.left, b.top, b.width, height);
  },

  
  
  
  inStack: function() {
    return iQ(this.container).hasClass("stacked");
  },

  
  
  
  setZ: function(value) {
    this.zIndex = value;
    iQ(this.container).css({zIndex: value});
  },

  
  
  
  
  close: function() {
    gBrowser.removeTab(this.tab);
    this._sendToSubscribers("tabRemoved");

    
    
  },

  
  
  
  addClass: function(className) {
    iQ(this.container).addClass(className);
  },

  
  
  
  removeClass: function(className) {
    iQ(this.container).removeClass(className);
  },

  
  
  
  
  setResizable: function(value) {
    var $resizer = iQ('.expander', this.container);

    this.resizeOptions.minWidth = TabItems.minTabWidth;
    this.resizeOptions.minHeight = TabItems.minTabWidth * (TabItems.tabHeight / TabItems.tabWidth);

    if (value) {
      $resizer.fadeIn();
      this.resizable(true);
    } else {
      $resizer.fadeOut();
      this.resizable(false);
    }
  },

  
  
  
  makeActive: function() {
   iQ(this.container).find("canvas").addClass("focus");
   iQ(this.container).find("img.cached-thumb").addClass("focus");

  },

  
  
  
  makeDeactive: function() {
   iQ(this.container).find("canvas").removeClass("focus");
   iQ(this.container).find("img.cached-thumb").removeClass("focus");
  },

  
  
  
  
  
  
  zoomIn: function(isNewBlankTab) {
    var self = this;
    var $tabEl = iQ(this.container);
    var childHitResult = { shouldZoom: true };
    if (this.parent)
      childHitResult = this.parent.childHit(this);

    if (childHitResult.shouldZoom) {
      
      var orig = $tabEl.bounds();
      var scale = window.innerWidth/orig.width;
      var tab = this.tab;

      function onZoomDone() {
        TabItems.resumePainting();
        
        if (gBrowser.selectedTab == tab)
          UI.tabOnFocus(tab);
        else
          gBrowser.selectedTab = tab;

        $tabEl
          .css(orig.css())
          .removeClass("front");

        
        
        if (self.parent) {
          var gID = self.parent.id;
          var groupItem = GroupItems.groupItem(gID);
          GroupItems.setActiveGroupItem(groupItem);
          groupItem.setActiveTab(self);
        } else {
          GroupItems.setActiveGroupItem(null);
          GroupItems.setActiveOrphanTab(self);
        }
        GroupItems.updateTabBar();

        if (isNewBlankTab)
          gWindow.gURLBar.focus();

        if (childHitResult.callback)
          childHitResult.callback();
      }

      
      
      
      
      
      
      var scaleCheat = 1.7;
      TabItems.pausePainting();
      $tabEl
        .addClass("front")
        .animate({
          top:    orig.top    * (1 - 1/scaleCheat),
          left:   orig.left   * (1 - 1/scaleCheat),
          width:  orig.width  * scale/scaleCheat,
          height: orig.height * scale/scaleCheat
        }, {
          duration: 230,
          easing: 'fast',
          complete: onZoomDone
        });
    }
  },

  
  
  
  
  
  
  
  zoomOut: function(complete) {
    var $tab = iQ(this.container);

    var box = this.getBounds();
    box.width -= this.sizeExtra.x;
    box.height -= this.sizeExtra.y;

    TabItems.pausePainting();

    var self = this;
    $tab.animate({
      left: box.left,
      top: box.top,
      width: box.width,
      height: box.height
    }, {
      duration: 300,
      easing: 'cubic-bezier', 
      complete: function() { 
        $tab.removeClass('front');

        GroupItems.setActiveOrphanTab(null);

        TabItems.resumePainting();

        self._zoomPrep = false;
        self.setBounds(self.getBounds(), true, {force: true});

        if (typeof complete == "function")
          complete();
      }
    });
  },

  
  
  
  
  
  setZoomPrep: function(value) {
    var $div = iQ(this.container);
    var data;

    var box = this.getBounds();
    if (value) {
      this._zoomPrep = true;

      
      
      
      
      
      
      
      
      var scaleCheat = 2;
      $div
        .addClass('front')
        .css({
          left: box.left * (1-1/scaleCheat),
          top: box.top * (1-1/scaleCheat),
          width: window.innerWidth/scaleCheat,
          height: box.height * (window.innerWidth / box.width)/scaleCheat
        });
    } else {
      this._zoomPrep = false;
      $div.removeClass('front');

      this.setBounds(box, true, {force: true});
    }
  }
});




window.TabItems = {
  minTabWidth: 40,
  tabWidth: 160,
  tabHeight: 120,
  fontSize: 9,
  items: [],
  paintingPaused: 0,
  _tabsWaitingForUpdate: [],
  _heartbeatOn: false,
  _heartbeatTiming: 100, 
  _lastUpdateTime: Date.now(),
  _eventListeners: [],

  
  
  
  init: function() {
    Utils.assert(window.AllTabs, "AllTabs must be initialized first");
    var self = this;

    
    this._eventListeners["open"] = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

      self.link(tab);
    }
    
    
    this._eventListeners["attrModified"] = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

      self.update(tab);
    }
    
    this._eventListeners["close"] = function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

      self.unlink(tab);
    }
    for (let name in this._eventListeners) {
      AllTabs.register(name, this._eventListeners[name]);
    }

    
    AllTabs.tabs.forEach(function(tab) {
      if (tab.ownerDocument.defaultView != gWindow)
        return;

      self.link(tab);
      self.update(tab);
    });
  },

  
  
  uninit: function() {
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

  
  
  
  update: function(tab) {
    try {
      Utils.assertThrow(tab, "tab");

      let shouldDefer = (
        this.isPaintingPaused() ||
        this._tabsWaitingForUpdate.length ||
        Date.now() - this._lastUpdateTime < this._heartbeatTiming
      );

      let isCurrentTab = (
        !UI._isTabViewVisible() &&
        tab == gBrowser.selectedTab
      );

      if (shouldDefer && !isCurrentTab) {
        if (this._tabsWaitingForUpdate.indexOf(tab) == -1)
          this._tabsWaitingForUpdate.push(tab);
      } else
        this._update(tab);
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  _update: function(tab) {
    try {
      Utils.assertThrow(tab, "tab");

      
      let index = this._tabsWaitingForUpdate.indexOf(tab);
      if (index != -1)
        this._tabsWaitingForUpdate.splice(index, 1);

      
      Utils.assertThrow(tab.tabItem, "must already be linked");
      let tabItem = tab.tabItem;

      
      let iconUrl = tab.image;
      if (iconUrl == null)
        iconUrl = "chrome://mozapps/skin/places/defaultFavicon.png";

      if (iconUrl != tabItem.favEl.src)
        tabItem.favEl.src = iconUrl;

      
      let tabUrl = tab.linkedBrowser.currentURI.spec;
      if (tabUrl != tabItem.url) {
        let oldURL = tabItem.url;
        tabItem.url = tabUrl;

        if (!tabItem.reconnected && (oldURL == 'about:blank' || !oldURL))
          this.reconnect(tabItem);

        tabItem.save();
      }

      
      let label = tab.label;
      let $name = iQ(tabItem.nameEl);
      if (!tabItem.isShowingCachedData && $name.text() != label)
        $name.text(label);

      
      let $canvas = iQ(tabItem.canvasEl);
      if (!tabItem.canvasSizeForced) {
        let w = $canvas.width();
        let h = $canvas.height();
        if (w != tabItem.canvasEl.width || h != tabItem.canvasEl.height) {
          tabItem.canvasEl.width = w;
          tabItem.canvasEl.height = h;
        }
      }

      tabItem.tabCanvas.paint();

      
      
      if (tabItem.isShowingCachedData && !tab.hasAttribute("busy"))
        tabItem.hideCachedData();
    } catch(e) {
      Utils.log(e);
    }

    this._lastUpdateTime = Date.now();
  },

  
  
  
  link: function(tab){
    try {
      Utils.assertThrow(tab, "tab");
      Utils.assertThrow(!tab.tabItem, "shouldn't already be linked");
      new TabItem(tab); 
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  unlink: function(tab) {
    try {
      Utils.assertThrow(tab, "tab");
      Utils.assertThrow(tab.tabItem, "should already be linked");

      this.unregister(tab.tabItem);
      tab.tabItem._sendToSubscribers("close");
      iQ(tab.tabItem.container).remove();
      tab.tabItem.removeTrenches();
      Items.unsquish(null, tab.tabItem);

      tab.tabItem = null;
      Storage.saveTab(tab, null);

      let index = this._tabsWaitingForUpdate.indexOf(tab);
      if (index != -1)
        this._tabsWaitingForUpdate.splice(index, 1);
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  heartbeat: function() {
    if (!this._heartbeatOn)
      return;

    if (this._tabsWaitingForUpdate.length) {
      this._update(this._tabsWaitingForUpdate[0]);
      
    }

    let self = this;
    if (this._tabsWaitingForUpdate.length) {
      setTimeout(function() {
        self.heartbeat();
      }, this._heartbeatTiming);
    } else
      this._hearbeatOn = false;
  },

  
  
  
  
  
  
  pausePainting: function() {
    this.paintingPaused++;

    if (this.isPaintingPaused() && this._heartbeatOn)
      this._heartbeatOn = false;
  },

  
  
  
  
  
  resumePainting: function() {
    this.paintingPaused--;

    if (!this.isPaintingPaused() &&
        this._tabsWaitingForUpdate.length &&
        !this._heartbeatOn) {
      this._heartbeatOn = true;
      this.heartbeat();
    }
  },

  
  
  
  
  isPaintingPaused: function() {
    return this.paintingPaused > 0;
  },

  
  
  
  register: function(item) {
    Utils.assert(item && item.isAnItem, 'item must be a TabItem');
    Utils.assert(this.items.indexOf(item) == -1, 'only register once per item');
    this.items.push(item);
  },

  
  
  
  unregister: function(item) {
    var index = this.items.indexOf(item);
    if (index != -1)
      this.items.splice(index, 1);
  },

  
  
  
  getItems: function() {
    return Utils.copy(this.items);
  },

  
  
  
  
  
  
  saveAll: function(saveImageData) {
    var items = this.getItems();
    items.forEach(function(item) {
      item.save(saveImageData);
    });
  },

  
  
  
  
  
  storageSanity: function(data) {
    var sane = true;
    if (!Utils.isRect(data.bounds)) {
      Utils.log('TabItems.storageSanity: bad bounds', data.bounds);
      sane = false;
    }

    return sane;
  },

  
  
  
  reconnect: function(item) {
    var found = false;

    try{
      Utils.assert(item, 'item');
      Utils.assert(item.tab, 'item.tab');

      if (item.reconnected)
        return true;

      if (!item.tab)
        return false;

      let tabData = Storage.getTabData(item.tab);
      if (tabData && this.storageSanity(tabData)) {
        if (item.parent)
          item.parent.remove(item);

        item.setBounds(tabData.bounds, true);

        if (Utils.isPoint(tabData.userSize))
          item.userSize = new Point(tabData.userSize);

        if (tabData.groupID) {
          var groupItem = GroupItems.groupItem(tabData.groupID);
          if (groupItem) {
            groupItem.add(item);

            if (item.tab == gBrowser.selectedTab)
              GroupItems.setActiveGroupItem(item.parent);
          }
        }

        if (tabData.imageData) {
          item.showCachedData(tabData);
          
          
          setTimeout(function() {
            if (item && item.isShowingCachedData) {
              item.hideCachedData();
            }
          }, 15000);
        }

        item.reconnected = true;
        found = true;
      } else
        item.reconnected = item.tab.linkedBrowser.currentURI.spec != 'about:blank';

      item.save();
    } catch(e) {
      Utils.log(e);
    }

    return found;
  }
};





var TabCanvas = function(tab, canvas) {
  this.init(tab, canvas);
};

TabCanvas.prototype = {
  
  
  init: function(tab, canvas) {
    this.tab = tab;
    this.canvas = canvas;

    var $canvas = iQ(canvas);
    var w = $canvas.width();
    var h = $canvas.height();
    canvas.width = w;
    canvas.height = h;
  },

  
  
  paint: function(evt) {
    var ctx = this.canvas.getContext("2d");

    var w = this.canvas.width;
    var h = this.canvas.height;
    if (!w || !h)
      return;

    let fromWin = this.tab.linkedBrowser.contentWindow;
    if (fromWin == null) {
      Utils.log('null fromWin in paint');
      return;
    }

    var scaler = w/fromWin.innerWidth;

    

    ctx.save();
    ctx.scale(scaler, scaler);
    try{
      ctx.drawWindow(fromWin, fromWin.scrollX, fromWin.scrollY, w/scaler, h/scaler, "#fff");
    } catch(e) {
      Utils.error('paint', e);
    }

    ctx.restore();
  },

  
  
  toImageData: function() {
    return this.canvas.toDataURL("image/png", "");
  }
};
