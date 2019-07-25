
window.TabItem = function(container, tab) {
  this._init(container);
  this.tab = tab;
  this.defaultSize = new Point(TabItems.tabWidth, TabItems.tabHeight);
  this.setResizable(true);
};

window.TabItem.prototype = $.extend(new Item(), {
  
  getStorageData: function() {
    return {
      bounds: this.bounds, 
      userSize: this.userSize,
      url: this.tab.url,
      groupID: (this.parent ? this.parent.id : 0)
    };
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
    var extra = this._getSizeExtra();
    newBounds.width += extra.x;
    newBounds.height += extra.y;










      
      this.bounds = newBounds;
      this._updateDebugBounds();

  },
  
  
  setBounds: function(rect, immediately) {
    var $container = $(this.container);
    var $title = $('.tab-title', $container);
    var $thumb = $('.thumb', $container);
    var $close = $('.close', $container);
    var extra = this._getSizeExtra();
    var css = {};
    
    const minFontSize = 6;
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

    if(immediately) {
      $container.css(css);
      
    } else {
      TabMirror.pausePainting();
      $container.animate(css, {complete: function() {
        TabMirror.resumePainting();
      }}).dequeue();
    }

    if(css.fontSize) {
      if(css.fontSize < minFontSize)
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
  },

  
  setZ: function(value) {
    $(this.container).css({zIndex: value});
  },
    
  
  close: function() {
    this.tab.close();
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
  }
});


window.TabItems = {
  minTabWidth: 40, 
  tabWidth: 160,
  tabHeight: 120, 
  fontSize: 9,

  
  init: function() {
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
            var item = $(this).data('tabItem');
            if(!item.parent || !item.parent.childHit(item)) {
              
              var orig = {
                width: $(this).width(),
                height:  $(this).height(),
                pos: $(this).position()
              }
  
              var scale = window.innerWidth/orig.width;
              
              var tab = Tabs.tab(this);
              var mirror = tab.mirror;
              
              var overflow = $("body").css("overflow");
              $("body").css("overflow", "hidden");
              
              function onZoomDone(){
                UI.tabBar.show(false);              
                TabMirror.resumePainting();
                $(this).find("canvas").data("link").tab.focus();
                $(this).css({
                  top:   orig.pos.top,
                  left:  orig.pos.left,
                  width: orig.width,
                  height:orig.height,
                  })
                  .removeClass("front");  
                Navbar.show();    
              
                try{
                  var gID = self.getItemByTab(this).parent.id;
                  if(gID) {
                    var group = Groups.group(gID);
                    UI.tabBar.showOnlyTheseTabs( group._children );
                  }
                }
                catch(e){
                  Utils.log(e);                
                }
              
                $("body").css("overflow", overflow);              
              }
    
              TabMirror.pausePainting();
              $(this)
                .addClass("front")
                .animate({
                  top:    -10,
                  left:   0,
                  easing: "easein",
                  width:  orig.width*scale,
                  height: orig.height*scale
                  }, 200, onZoomDone);
            }
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
        var item = new TabItem(this, tab);
        $(this).data('tabItem', item);    
        
        if(TabItems.reconnect(item))
          reconnected = true;
      });
      
      if(!reconnected && $div.length == 1 && Groups)
        Groups.newTab($div.data('tabItem'));
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
    }
    
    window.TabMirror.customize(mod);
  },

  
  getItems: function() {
    var items = [];
    $('.tab').each(function() {
      items.push($(this).data('tabItem'));
    });
    
    return items;
  },
  
  
  getItemByTab: function(tab) {
    return $(tab).data("tabItem");
  },
  
  
  getStorageData: function() {
    var data = {tabs: []};
    var items = this.getItems();
    $.each(items, function(index, item) {
      data.tabs.push(item.getStorageData());
    });
    
    return data;
  },
  
  
  reconstitute: function(data) {
    this.storageData = data;
    var items = this.getItems();
    if(data && data.tabs) {
      var self = this;
      $.each(items, function(index, item) {
        if(!self.reconnect(item))
          Groups.newTab(item);
      });
    } else {
        var box = Items.getPageBounds();
        box.inset(20, 20);
        
        Items.arrange(items, box, {padding: 10, animate:false});
    }
  },
  
  
  reconnect: function(item) {
    var found = false;
    if(this.storageData && this.storageData.tabs) {
      $.each(this.storageData.tabs, function(index, tab) {
        if(item.getURL() == tab.url) {
          item.setBounds(tab.bounds, true);
          item.userSize = tab.userSize;
          if(tab.groupID) {
            var group = Groups.group(tab.groupID);
            group.add(item);
          }
          
          found = true;
          return false;
        }      
      });
    }   
    
    return found; 
  }
};

TabItems.init();