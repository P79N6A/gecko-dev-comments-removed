













































window.TabItem = function(container, tab) {
  Utils.assert('container', container);
  Utils.assert('tab', tab);
  Utils.assert('tab.mirror', tab.mirror);

  this.defaultSize = new Point(TabItems.tabWidth, TabItems.tabHeight);
  this.locked = {};
  this.isATabItem = true;
  this._zoomPrep = false;
  this.sizeExtra = new Point();
  this.keepProportional = true;

  
  var $div = iQ(container);
  var self = this;

  $div.data('tabItem', this);
  this.isDragging = false;

  this.sizeExtra.x = parseInt($div.css('padding-left'))
      + parseInt($div.css('padding-right'));

  this.sizeExtra.y = parseInt($div.css('padding-top'))
      + parseInt($div.css('padding-bottom'));

  this.bounds = $div.bounds();
  this.bounds.width += this.sizeExtra.x;
  this.bounds.height += this.sizeExtra.y;

  
  this._init(container);

  
  
  
  this.dropOptions.drop = function(e){
    var $target = iQ(this.container);
    this.isDropTarget = false;

    var phantom = $target.data("phantomGroup");

    var group = drag.info.item.parent;
    if ( group ) {
      group.add( drag.info.$el );
    } else {
      phantom.removeClass("phantom acceptsDrop");
      new Group([$target, drag.info.$el], {container:phantom, bounds:phantom.bounds()});
    }
  };

  this.dropOptions.over = function(e){
    var $target = iQ(this.container);
    this.isDropTarget = true;

    $target.removeClass("acceptsDrop");

    var phantomMargin = 40;

    var groupBounds = this.getBoundsWithTitle();
    groupBounds.inset( -phantomMargin, -phantomMargin );

    iQ(".phantom").remove();
    var phantom = iQ("<div>")
      .addClass("group phantom acceptsDrop")
      .css({
        position: "absolute",
        zIndex: -99
      })
      .css(groupBounds.css())
      .hide()
      .appendTo("body");

    var defaultRadius = Trenches.defaultRadius;
    
    
    Trenches.defaultRadius = phantomMargin + 1;
    var updatedBounds = drag.info.snapBounds(groupBounds,'none');
    Trenches.defaultRadius = defaultRadius;

    
    if (updatedBounds)
      phantom.css(updatedBounds.css());

    phantom.fadeIn();

    $target.data("phantomGroup", phantom);
  };

  this.dropOptions.out = function(e){
    this.isDropTarget = false;
    var phantom = iQ(this.container).data("phantomGroup");
    if (phantom) {
      phantom.fadeOut(function(){
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
      gBrowser.removeTab(tab);
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
  this.tab = tab;
  this.setResizable(true);

  this._updateDebugBounds();

  TabItems.register(this);
  this.tab.mirror.addSubscriber(this, "close", function(who, info) {
    TabItems.unregister(self);
    self.removeTrenches();
  });

  this.tab.mirror.addSubscriber(this, 'urlChanged', function(who, info) {
    if (!self.reconnected && (info.oldURL == 'about:blank' || !info.oldURL))
      TabItems.reconnect(self);

    self.save();
  });
};

window.TabItem.prototype = iQ.extend(new Item(), {
  
  
  
  
  
  
  getStorageData: function(getImageData) {
    return {
      bounds: this.getBounds(),
      userSize: (Utils.isPoint(this.userSize) ? new Point(this.userSize) : null),
      url: this.tab.linkedBrowser.currentURI.spec,
      groupID: (this.parent ? this.parent.id : 0),
      imageData: (getImageData && this.tab.mirror.tabCanvas ?
                  this.tab.mirror.tabCanvas.toImageData() : null),
      title: getImageData && this.tab.label || null
    };
  },

  
  
  
  
  
  
  save: function(saveImageData) {
    try{
      if (!this.tab || !this.reconnected) 
        return;

      var data = this.getStorageData(saveImageData);
      if (TabItems.storageSanity(data))
        Storage.saveTab(this.tab, data);
    }catch(e){
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

      if (iQ.isEmptyObject(css))
        return;

      this.bounds.copy(rect);

      
      
      
      if (immediately || (!this._hasBeenDrawn) ) {
  
        $container.css(css);
      } else {
        TabMirror.pausePainting();
        $container.animate(css, {
          duration: 200,
          easing: 'tabcandyBounce',
          complete: function() {
            TabMirror.resumePainting();
          }
        });
    
      }

      if (css.fontSize && !this.inStack()) {
        if (css.fontSize < fontSizeRange.min )
          $title.fadeOut();
        else
          $title.fadeIn();
      }

      if (css.width) {

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
          if ( proportion <= .1 )
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
    var $container = iQ(this.container);
    var $title = iQ('.tab-title', $container);
    return new Rect( b.left, b.top, b.width, b.height + $title.height() );
  },

  
  
  
  inStack: function(){
    return iQ(this.container).hasClass("stacked");
  },

  
  
  
  setZ: function(value) {
    this.zIndex = value;
    iQ(this.container).css({zIndex: value});
  },

  
  
  
  
  close: function() {
    gBrowser.removeTab(this.tab);

    
    
  },

  
  
  
  addClass: function(className) {
    iQ(this.container).addClass(className);
  },

  
  
  
  removeClass: function(className) {
    iQ(this.container).removeClass(className);
  },

  
  
  
  
  addOnClose: function(referenceObject, callback) {
    this.tab.mirror.addSubscriber(referenceObject, "close", callback);
  },

  
  
  
  removeOnClose: function(referenceObject) {
    this.tab.mirror.removeSubscriber(referenceObject, "close");
  },

  
  
  
  
  setResizable: function(value){
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

  
  
  
  makeActive: function(){
   iQ(this.container).find("canvas").addClass("focus");
   iQ(this.container).find("img.cached-thumb").addClass("focus");

  },

  
  
  
  makeDeactive: function(){
   iQ(this.container).find("canvas").removeClass("focus");
   iQ(this.container).find("img.cached-thumb").removeClass("focus");
  },

  
  
  
  
  zoomIn: function() {
    var self = this;
    var $tabEl = iQ(this.container);
    var childHitResult = { shouldZoom: true };
    if (this.parent)
      childHitResult = this.parent.childHit(this);

    if (childHitResult.shouldZoom) {
      
      var orig = {
        width: $tabEl.width(),
        height:  $tabEl.height(),
        pos: $tabEl.position()
      };

      var scale = window.innerWidth/orig.width;

      var tab = this.tab;

      function onZoomDone(){
        TabMirror.resumePainting();
        
        if (gBrowser.selectedTab == tab) {
          UI.tabOnFocus(tab);
        } else {
          gBrowser.selectedTab = tab;
        }

        $tabEl
          .css({
            top:   orig.pos.top,
            left:  orig.pos.left,
            width: orig.width,
            height:orig.height,
          })
          .removeClass("front");

        
        
        if ( self.parent ){
          var gID = self.parent.id;
          var group = Groups.group(gID);
          Groups.setActiveGroup( group );
          group.setActiveTab( self );
        }
        else
          Groups.setActiveGroup( null );

        if (childHitResult.callback)
          childHitResult.callback();
      }

      
      
      
      
      
      
      var scaleCheat = 1.7;
      TabMirror.pausePainting();
      $tabEl
        .addClass("front")
        .animate({
          top:    orig.pos.top * (1-1/scaleCheat),
          left:   orig.pos.left * (1-1/scaleCheat),
          width:  orig.width*scale/scaleCheat,
          height: orig.height*scale/scaleCheat
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

    TabMirror.pausePainting();

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

        TabMirror.resumePainting();

        self._zoomPrep = false;
        self.setBounds(self.getBounds(), true, {force: true});

        if (iQ.isFunction(complete))
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

  
  
  
  init: function() {

  },

  
  
  
  register: function(item) {
    Utils.assert('item must be a TabItem', item && item.isAnItem);
    Utils.assert('only register once per item', this.items.indexOf(item) == -1);
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

  
  
  
  
  getItemByTabElement: function(tabElement) {
    return iQ(tabElement).data("tabItem");
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
      Utils.assert('item', item);
      Utils.assert('item.tab', item.tab);

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
          var group = Groups.group(tabData.groupID);
          if (group) {
            group.add(item);

            if (item.tab == gBrowser.selectedTab)
              Groups.setActiveGroup(item.parent);
          }
        }

        if (tabData.imageData) {
          var mirror = item.tab.mirror;
          mirror.showCachedData(tabData);
          
          
          iQ.timeout(function() {
            if (mirror && mirror.isShowingCachedData) {
              mirror.hideCachedData();
            }
          }, 15000);
        }

        Groups.updateTabBarForActiveGroup();

        item.reconnected = true;
        found = true;
      } else
        item.reconnected = item.tab.linkedBrowser.currentURI.spec != 'about:blank';

      item.save();
    }catch(e){
      Utils.log(e);
    }

    return found;
  }
};
