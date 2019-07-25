




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

window.TabItem.prototype = $.extend(new Item(), {
  
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
    var $container = $(this.container);

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

    var $container = $(this.container);
    var $title = $('.tab-title', $container);
    var $thumb = $('.thumb', $container);
    var $close = $('.close', $container);
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
      
    if($.isEmptyObject(css))
      return;
      
    this.bounds.copy(rect);
    
    
    
    
    if(immediately || (!this._hasBeenDrawn) ) {
      $container.stop(true, true);
      $container.css(css);
    } else {
      TabMirror.pausePainting();
      $container.animate(css,{
        complete: function() {TabMirror.resumePainting();},
        duration: 350,
        easing: "tabcandyBounce"
      }).dequeue();
    }

    if(css.fontSize && !this.inStack()) {
      if(css.fontSize < minFontSize )
        $title.fadeOut().dequeue();
      else
        $title.fadeIn().dequeue();
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
    return $(this.container).hasClass("stacked");
  },

  
  setZ: function(value) {
    $(this.container).css({zIndex: value});
  },
    
  
  close: function() {
    this.tab.close();

    
    
  },
  
  
  addClass: function(className) {
    $(this.container).addClass(className);
  },
  
  
  removeClass: function(className) {
    $(this.container).removeClass(className);
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
   $(this.container).find("canvas").addClass("focus")
  },

  
  makeDeactive: function(){
   $(this.container).find("canvas").removeClass("focus")
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
        
    function mod($div){
      if(window.Groups) {        
        $div.data('isDragging', false);
        $div.draggable(window.Groups.dragOptions);
        $div.droppable(window.Groups.dropOptions);
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
        
        if(e.target.className == "close") {
          $(this).find("canvas").data("link").tab.close(); }
        else {
          if(!$(this).data('isDragging')) {        
            self.zoomTo(this);
          } else {
            $(this).find("canvas").data("link").tab.raw.pos = $(this).position();
          }
        }
      });
      
      $("<div class='close'></div>").appendTo($div);
      $("<div class='expander'></div>").appendTo($div);
  
      var reconnected = false;
      $div.each(function() {
        var tab = Tabs.tab(this);
        if(tab == Utils.homeTab) { 
          $(this).hide();
          reconnected = true;
        } else {
          var item = new TabItem(this, tab);
          $(this).data('tabItem', item);    
          
          item.addOnClose(self, function() {
            Items.unsquish(null, item);
          });

          if(TabItems.reconnect(item))
            reconnected = true;
          else  
            Groups.newTab(item);          
        }
      });

       







        
            
      
      
      
      
      
      
      
      
      
      
      
      
      
      
    }
    
    window.TabMirror.customize(mod);
  },

  
  register: function(item) {
    Utils.assert('only register once per item', $.inArray(item, this.items) == -1);
    this.items.push(item);
  },
  
  
  unregister: function(item) {
    var index = $.inArray(item, this.items);
    if(index != -1)
      this.items.splice(index, 1);  
  },
    
  
  
  
  
  
  
  zoomTo: function(tabEl){
    var self = this;
    var item = $(tabEl).data('tabItem');
    var childHitResult = { shouldZoom: true };
    if(item.parent)
      childHitResult = item.parent.childHit(item);
      
    if(childHitResult.shouldZoom) {
      
      var orig = {
        width: $(tabEl).width(),
        height:  $(tabEl).height(),
        pos: $(tabEl).position()
      }

      var scale = window.innerWidth/orig.width;
      
      var tab = Tabs.tab(tabEl);
      var mirror = tab.mirror;
      
      var overflow = $("body").css("overflow");
      $("body").css("overflow", "hidden");
      
      function onZoomDone(){
        UI.tabBar.show(false);              
        TabMirror.resumePainting();
        $(tabEl).find("canvas").data("link").tab.focus();
        $(tabEl).css({
          top:   orig.pos.top,
          left:  orig.pos.left,
          width: orig.width,
          height:orig.height,
          })
          .removeClass("front");  
        Navbar.show();
               
        
        
        if( self.getItemByTab(tabEl).parent ){
          var gID = self.getItemByTab(tabEl).parent.id;
          var group = Groups.group(gID);
          Groups.setActiveGroup( group );                  
        }
        else
          Groups.setActiveGroup( null );
      
        $("body").css("overflow", overflow); 
        
        if(childHitResult.callback)
          childHitResult.callback();             
      }

      TabMirror.pausePainting();
      $(tabEl)
        .addClass("front")
        .animate({
          top:    -10,
          left:   0,
          width:  orig.width*scale,
          height: orig.height*scale
          }, 200, "easeInQuad", onZoomDone);
    }    
  },

  
  getItems: function() {
    return Utils.copy(this.items);
  },
    
  
  getItemByTab: function(tab) {
    return $(tab).data("tabItem");
  },
  
  
  saveAll: function() {
    var items = this.getItems();
    $.each(items, function(index, item) {
      item.save();
    });
  },
  
  
  reconstitute: function() {
    var items = this.getItems();
    var self = this;
    $.each(items, function(index, item) {
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

