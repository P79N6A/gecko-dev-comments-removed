













































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
    $target.removeClass("acceptsDrop");
    var phantom = $target.data("phantomGroup");
    
    var group = drag.info.item.parent;
    if ( group == null ){
      phantom.removeClass("phantom");
      phantom.removeClass("group-content");
      group = new Group([$target, drag.info.$el], {container:phantom});
    } else 
      group.add( drag.info.$el );      
  };
  
  this.dropOptions.over = function(e){
    var $target = iQ(this.container);

    var groupBounds = Groups.getBoundingBox( [drag.info.$el, $target] );
    groupBounds.inset( -20, -20 );

    iQ(".phantom").remove();
    var phantom = iQ("<div>")
      .addClass('group phantom group-content')
      .css({
        position: "absolute",
        zIndex: -99
      })
      .css(groupBounds)
      .appendTo("body")
      .hide()
      .fadeIn();
      
    $target.data("phantomGroup", phantom);      
  };
  
  this.dropOptions.out = function(e){      
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
      tab.close();
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
  this.tab.mirror.addOnClose(this, function(who, info) {
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
  
  getStorageData: function() {
    return {
      bounds: this.getBounds(), 
      userSize: (isPoint(this.userSize) ? new Point(this.userSize) : null),
      url: this.tab.url,
      groupID: (this.parent ? this.parent.id : 0)
    };
  },

  
  save: function() {
    try{
      if (!this.tab || !this.tab.raw || !this.reconnected) 
        return;

      var data = this.getStorageData();
      if (TabItems.storageSanity(data))
        Storage.saveTab(this.tab.raw, data);
    }catch(e){
      Utils.log("Error in saving tab value: "+e);
    }
  },
  
  
  getURL: function() {
    return this.tab.url;
  },
    
  
  setBounds: function(rect, immediately, options) {
    if (!isRect(rect)) {
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
      
      const minFontSize = 8;
      const maxFontSize = 15;
  
      if (rect.left != this.bounds.left || options.force)
        css.left = rect.left;
        
      if (rect.top != this.bounds.top || options.force)
        css.top = rect.top;
        
      if (rect.width != this.bounds.width || options.force) {
        css.width = rect.width - this.sizeExtra.x;
        var scale = css.width / TabItems.tabWidth;
        
        
        
        
        css.fontSize = minFontSize + (maxFontSize-minFontSize)*(.5+.5*Math.tanh(2*scale-2))
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
        if (css.fontSize < minFontSize )
          $title.fadeOut();
        else
          $title.fadeIn();
      }
  
              
      
      
      
      function slider(bounds, val){
        var keys = [];
        for (var key in bounds){ keys.push(key); bounds[key] = parseFloat(bounds[key]); };
        keys.sort(function(a,b){return a-b});
        var min = keys[0], max = keys[1];
        
        function slide(value){
          if ( value >= max ) return bounds[max];
          if ( value <= min ) return bounds[min];
          var rise = bounds[max] - bounds[min];
          var run = max-min;
          var value = rise * (value-min)/run;
          if ( value >= bounds[max] ) return bounds[max];
          if ( value <= bounds[min] ) return bounds[min];
          return value;
        }
        
        if ( val == undefined )
          return slide;
        return slide(val);
      };

      if (css.width && !this.inStack()) {
        $fav.css({top:4,left:4});
        
        var opacity = slider({70:1, 60:0}, css.width);
        $close.show().css({opacity:opacity});
        if ( opacity <= .1 ) $close.hide()

        var pad = slider({70:6, 60:1}, css.width);
        $fav.css({
         "padding-left": pad + "px",
         "padding-right": pad + 2 + "px",
         "padding-top": pad + "px",
         "padding-bottom": pad + "px",
         "border-color": "rgba(0,0,0,"+ slider({70:.2, 60:.1}, css.width) +")",
        });
      } 
      
      if (css.width && this.inStack()){
        $fav.css({top:0, left:0});
        var opacity = slider({90:1, 70:0}, css.width);
        
        var pad = slider({90:6, 70:1}, css.width);
        $fav.css({
         "padding-left": pad + "px",
         "padding-right": pad + 2 + "px",
         "padding-top": pad + "px",
         "padding-bottom": pad + "px",
         "border-color": "rgba(0,0,0,"+ slider({90:.2, 70:.1}, css.width) +")",
        });
      }   

      this._hasBeenDrawn = true;
    }

    this._updateDebugBounds();
    rect = this.getBounds(); 
    
    if (!isRect(this.bounds))
      Utils.trace('TabItem.setBounds: this.bounds is not a real rectangle!', this.bounds);
    
    if (this.parent === null)
      this.setTrenches(rect);

    this.save();
  },

  
  inStack: function(){
    return iQ(this.container).hasClass("stacked");
  },

  
  setZ: function(value) {
    this.zIndex = value;
    iQ(this.container).css({zIndex: value});
  },
    
  
  close: function() {
    this.tab.close();

    
    
  },
  
  
  addClass: function(className) {
    iQ(this.container).addClass(className);
  },
  
  
  removeClass: function(className) {
    iQ(this.container).removeClass(className);
  },
  
  
  addOnClose: function(referenceObject, callback) {
    this.tab.mirror.addOnClose(referenceObject, callback);      
  },

  
  removeOnClose: function(referenceObject) {
    this.tab.mirror.removeOnClose(referenceObject);      
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
   iQ(this.container).find("canvas").addClass("focus")
  },

  
  makeDeactive: function(){
   iQ(this.container).find("canvas").removeClass("focus")
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
        
        if (tab.isFocused()) {
          Page.tabOnFocus(tab);
        } else {
          tab.focus();
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

  
  init: function() {
    this.items = [];
    
    var self = this;
    window.TabMirror.customize(function(mirror) {
      var $div = iQ(mirror.el);
      var tab = mirror.tab;
      var item = new TabItem(mirror.el, tab);
        
      item.addOnClose(self, function() {
        Items.unsquish(null, item);
      });

      if (!self.reconnect(item))
        Groups.newTab(item);          
    });
  },

  
  register: function(item) {
    Utils.assert('only register once per item', iQ.inArray(item, this.items) == -1);
    this.items.push(item);
  },
  
  
  unregister: function(item) {
    var index = iQ.inArray(item, this.items);
    if (index != -1)
      this.items.splice(index, 1);  
  },
    
  
  getItems: function() {
    return Utils.copy(this.items);
  },
    
  
  
  
  
  getItemByTabElement: function(tabElement) {
    return iQ(tabElement).data("tabItem");
  },
  
  
  saveAll: function() {
    var items = this.getItems();
    items.forEach(function(item) {
      item.save();
    });
  },
  
  
  storageSanity: function(data) {
    
    var sane = true;
    if (!isRect(data.bounds)) {
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
        
      if (!item.tab.raw)
        return false;
        
      var tab = Storage.getTabData(item.tab.raw);       
      if (tab && this.storageSanity(tab)) {
        if (item.parent)
          item.parent.remove(item);
          
        item.setBounds(tab.bounds, true);
        
        if (isPoint(tab.userSize))
          item.userSize = new Point(tab.userSize);
          
        if (tab.groupID) {
          var group = Groups.group(tab.groupID);
          if (group) {
            group.add(item);          
          
            if (item.tab == Utils.activeTab) 
              Groups.setActiveGroup(item.parent);
          }
        }  
        
        Groups.updateTabBarForActiveGroup();
        
        item.reconnected = true;
        found = true;
      } else
        item.reconnected = (item.tab.url != 'about:blank');
    
      item.save();
    }catch(e){
      Utils.log(e);
    }
        
    return found; 
  }
};
