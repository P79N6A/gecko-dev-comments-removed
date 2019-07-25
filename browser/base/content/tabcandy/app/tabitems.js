
window.TabItem = function(container, tab) {
  this._init(container);
  this.tab = tab;
  this.defaultSize = new Point(TabItems.tabWidth, TabItems.tabHeight);
};

window.TabItem.prototype = $.extend(new Item(), {
  
  reloadBounds: function() {
    this.bounds = Utils.getBounds(this.container);
    this._updateDebugBounds();
  },
  
  
  setBounds: function(rect, immediately) {
    var css = {};

    if(rect.left != this.bounds.left)
      css.left = rect.left;
      
    if(rect.top != this.bounds.top)
      css.top = rect.top;
      
    if(rect.width != this.bounds.width) {
      css.width = rect.width;
      var scale = rect.width / TabItems.tabWidth;
      css.fontSize = TabItems.fontSize * scale;
    }

    if(rect.height != this.bounds.height)
      css.height = rect.height; 
      
    if($.isEmptyObject(css))
      return;
      
    this.bounds.copy(rect);

    if(immediately) {
      $(this.container).css(css);
    } else {
      TabMirror.pausePainting();
      $(this.container).animate(css, {complete: function() {
        TabMirror.resumePainting();
      }}).dequeue();
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
  }
});


window.TabItems = {
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

            
            var [w,h,z] = [$(this).width(), $(this).height(), $(this).css("zIndex")];
            var origPos = $(this).position();
            var zIndex = $(this).css("zIndex");
            var scale = window.innerWidth/w;
            var big = 99999;
            
            var tab = Tabs.tab(this);
            var mirror = tab.mirror;
            
            
            var overflow = $("body").css("overflow");
            $("body").css("overflow", "hidden");
            
            function onZoomDone(){
              $(this).find("canvas").data("link").tab.focus();
              $(this)
                .css({top: origPos.top, left: origPos.left, width:w, height:h, zIndex:z});  
              Navbar.show();    
              $("body").css("overflow", overflow);              
            }
  
            $(this)
              .css("zIndex",big)
              .animate({
                top: -10,
                left: 0,
                easing: "easein",
                width:w*scale,
                height:h*scale
                }, 200, onZoomDone);
            
          } else {
            $(this).find("canvas").data("link").tab.raw.pos = $(this).position();
          }
        }
      });
      
      $("<div class='close'><img src='img/closetab.png'/></div>").appendTo($div);
      $("<div class='expander'><img src='img/expand.png'/></div>").appendTo($div);
  
      if($div.length == 1) {
        var p = Page.findOpenSpaceFor($div); 
        $div.css({left: p.x, top: p.y});
      }
      
      $div.each(function() {
        var tab = Tabs.tab(this);
        $(this).data('tabItem', new TabItem(this, tab));
      });
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
    }
    
    window.TabMirror.customize(mod);
  }
};

TabItems.init();