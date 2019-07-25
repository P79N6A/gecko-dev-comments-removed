




window.TabItem = function(container, tab) {
  Utils.assert('container', container);
  Utils.assert('tab', tab);
  Utils.assert('tab.mirror', tab.mirror);
  
  this.defaultSize = new Point(TabItems.tabWidth, TabItems.tabHeight);
  this.locked = {};

  this._init(container);

  this.reconnected = false;
  this._hasBeenDrawn = false;
  this.tab = tab;
  this.setResizable(true);

  TabItems.register(this);
  var self = this;
  this.tab.mirror.addOnClose(this, function(who, info) {
    TabItems.unregister(self);
  });   
     
  this.tab.mirror.addSubscriber(this, 'urlChanged', function(who, info) {
    if(!self.reconnected && (info.oldURL == 'about:blank' || !info.oldURL)) 
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
      if (!("tab" in this) || !("raw" in this.tab) || !this.reconnected) 
        return;

      var data = this.getStorageData();
      if(TabItems.storageSanity(data))
        Storage.saveTab(this.tab.raw, data);
    }catch(e){
      Utils.log("Error in saving tab value: "+e);
    }
  },
  
  
  getURL: function() {
    return this.tab.url;
  },
  
  
  _getSizeExtra: function() {
    var $container = iQ(this.container);

    var widthExtra = parseInt($container.css('padding-left')) 
        + parseInt($container.css('padding-right'));

    var heightExtra = parseInt($container.css('padding-top')) 
        + parseInt($container.css('padding-bottom'));

    return new Point(widthExtra, heightExtra);
  },
  
  
  reloadBounds: function() {
    var newBounds = Utils.getBounds(this.container);










      
      this.bounds = newBounds;
      this._updateDebugBounds();


    this.save();
  },
  
  
  setBounds: function(rect, immediately) {
    if(!isRect(rect)) {
      Utils.trace('TabItem.setBounds: rect is not a real rectangle!', rect);
      return;
    }
    
    var $container = iQ(this.container);
    var $title = iQ('.tab-title', $container);
    var $thumb = iQ('.thumb', $container);
    var $close = iQ('.close', $container);
    var extra = this._getSizeExtra();
    var css = {};
    
    const minFontSize = 8;
    const maxFontSize = 15;

    if(rect.left != this.bounds.left)
      css.left = rect.left;
      
    if(rect.top != this.bounds.top)
      css.top = rect.top;
      
    if(rect.width != this.bounds.width) {
      css.width = rect.width - extra.x;
      var scale = css.width / TabItems.tabWidth;
      
      
      
      
      css.fontSize = minFontSize + (maxFontSize-minFontSize)*(.5+.5*Math.tanh(2*scale-2))
    }

    if(rect.height != this.bounds.height) {
      css.height = rect.height - extra.y; 
    }
      
    if(iQ.isEmptyObject(css))
      return;
      
    this.bounds.copy(rect);
    
    
    
    
    if(immediately || (!this._hasBeenDrawn) ) {

      $container.css(css);
    } else {
      TabMirror.pausePainting();
      $container.animate(css, 'animate200', function() {
        TabMirror.resumePainting();
      }); 

    }

    if(css.fontSize && !this.inStack()) {
      if(css.fontSize < minFontSize )
        $title.fadeOut();
      else
        $title.fadeIn();
    }

    if(css.width) {
      if(css.width < 30) {
        $thumb.fadeOut();
        $close.fadeOut();
      } else {
        $thumb.fadeIn();
        $close.fadeIn();
      }
    }    

    this._updateDebugBounds();
    this._hasBeenDrawn = true;
    
    if(!isRect(this.bounds))
      Utils.trace('TabItem.setBounds: this.bounds is not a real rectangle!', this.bounds);

    this.save();
  },

  
  inStack: function(){
    return iQ(this.container).hasClass("stacked");
  },

  
  setZ: function(value) {
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
    var self = this;
    
    var $resizer = $('.expander', this.container);
    if(value) {
      $resizer.fadeIn();
      $(this.container).resizable({
        handles: "se",
        aspectRatio: true,
        minWidth: TabItems.minTabWidth,
        minHeight: TabItems.minTabWidth * (TabItems.tabHeight / TabItems.tabWidth),
        resize: function(){
          self.reloadBounds();
        },
        stop: function(){
          self.reloadBounds();
          self.setUserSize();        
          self.pushAway();
        } 
      });
    } else {
      $resizer.fadeOut();
      $(this.container).resizable('destroy');
    }
  },
  
  
  makeActive: function(){
   iQ(this.container).find("canvas").addClass("focus")
  },

  
  makeDeactive: function(){
   iQ(this.container).find("canvas").removeClass("focus")
  },
  
  
  
  
  zoom: function(){
    TabItems.zoomTo(this.container);
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
        
    function mod(mirror) {
      var $div = iQ(mirror.el);
      var $$div = $(mirror.el);
      var tab = mirror.tab;
      
      if(window.Groups) {        
        $$div.data('isDragging', false);
        $$div.draggable(window.Groups.dragOptions);
        $$div.droppable(window.Groups.dropOptions);
      }
      
      $div.mousedown(function(e) {
        if(!Utils.isRightClick(e))
          self.lastMouseDownTarget = e.target;
      });
        
      $div.mouseup(function(e) { 
        var same = (e.target == self.lastMouseDownTarget);
        self.lastMouseDownTarget = null;
        if(!same)
          return;
        
        if(iQ(e.target).hasClass("close")) 
          tab.close();
        else {
          if(!$(this).data('isDragging'))
            self.zoomTo(this);
          else 
            tab.raw.pos = iQ(this).position(); 
        }
      });
      
      $("<div>")
        .addClass('close')
        .appendTo($div); 
        
      $("<div>")
        .addClass('expander')
        .appendTo($div);
  
      if(tab == Utils.homeTab) 
        $div.hide();
      else {
        var item = new TabItem(mirror.el, tab);
        $div.data('tabItem', item);    
        
        item.addOnClose(self, function() {
          Items.unsquish(null, item);
        });

        if(!TabItems.reconnect(item))
          Groups.newTab(item);          
      }
    }
    
    window.TabMirror.customize(mod);
  },

  
  register: function(item) {
    Utils.assert('only register once per item', iQ.inArray(item, this.items) == -1);
    this.items.push(item);
  },
  
  
  unregister: function(item) {
    var index = iQ.inArray(item, this.items);
    if(index != -1)
      this.items.splice(index, 1);  
  },
    
  
  
  
  
  
  
  zoomTo: function(tabEl){
    var self = this;
    var $tabEl = iQ(tabEl);
    var item = this.getItemByTabElement(tabEl);
    var childHitResult = { shouldZoom: true };
    if(item.parent)
      childHitResult = item.parent.childHit(item);
      
    if(childHitResult.shouldZoom) {
      
      var orig = {
        width: $tabEl.width(),
        height:  $tabEl.height(),
        pos: $tabEl.position()
      }

      var scale = window.innerWidth/orig.width;
      
      var tab = Tabs.tab(tabEl);
      var mirror = tab.mirror;
      
      var overflow = iQ("body").css("overflow");
      iQ("body").css("overflow", "hidden");
      
      function onZoomDone(){
        try {
          UI.tabBar.show(false);              
          TabMirror.resumePainting();
          tab.focus();
          $tabEl.css({
            top:   orig.pos.top,
            left:  orig.pos.left,
            width: orig.width,
            height:orig.height,
            })
            .removeClass("front");  
          Navbar.show();
                 
          
          
          if( item.parent ){
            var gID = item.parent.id;
            var group = Groups.group(gID);
            Groups.setActiveGroup( group );
            group.setActiveTab( tabEl );                 
          }
          else
            Groups.setActiveGroup( null );
        
          iQ("body").css("overflow", overflow); 
          
          if(childHitResult.callback)
            childHitResult.callback();             
        } catch(e) {
          Utils.log(e);
        }
      }

      TabMirror.pausePainting();
      iQ(tabEl)
        .addClass("front")
        .animate({
          top:    -10,
          left:   0,
          width:  orig.width*scale,
          height: orig.height*scale
          }, 'animate200', onZoomDone);
    }    
  },

  
  getItems: function() {
    return Utils.copy(this.items);
  },
    
  
  
  
  
  getItemByTabElement: function(tabElement) {
    return iQ(tabElement).data("tabItem");
  },
  
  
  saveAll: function() {
    var items = this.getItems();
    iQ.each(items, function(index, item) {
      item.save();
    });
  },
  
  
  reconstitute: function() {
    var items = this.getItems();
    var self = this;
    iQ.each(items, function(index, item) {
      if(!self.reconnect(item))
        Groups.newTab(item);
    });
  },
  
  
  storageSanity: function(data) {
    
    var sane = true;
    if(!isRect(data.bounds)) {
      Utils.log('TabItems.storageSanity: bad bounds', data.bounds);
      sane = false;
    }
    
    return sane;
  },

  
  reconnect: function(item) {
    var found = false;

    try{
      if(item.reconnected) {
        return true;
      }
        
      var tab = Storage.getTabData(item.tab.raw);
      if (tab && this.storageSanity(tab)) {
        if(item.parent)
          item.parent.remove(item);
          
        item.setBounds(tab.bounds, true);
        
        if(isPoint(tab.userSize))
          item.userSize = new Point(tab.userSize);
          
        if(tab.groupID) {
          var group = Groups.group(tab.groupID);
          if(group) {
            group.add(item);          
          
            if(item.tab == Utils.activeTab) 
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
      Utils.log("Error in TabItems.reconnect: "+e + " at " + e.fileName + "(" + e.lineNumber + ")");
    }
        
    return found; 
  }
};

