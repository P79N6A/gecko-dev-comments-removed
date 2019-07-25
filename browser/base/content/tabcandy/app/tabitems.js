
window.TabItem = function(container, tab) {
  this.container = container;
  this.tab = tab;
  
  $(this.container).data('item', this);
};

window.TabItem.prototype = $.extend(new Item(), {
  
  getContainer: function() {
    return this.container;
  },
  
  
  getBounds: function() {
    return Utils.getBounds(this.container);
  },
  
  
  setBounds: function(rect, immediately) {
    this.setPosition(rect.left, rect.top, immediately);
    this.setSize(rect.width, rect.height, immediately);
  },
  
  
  setPosition: function(left, top, immediately) {
    if(immediately) 
      $(this.container).css({left: left, top: top});
    else {
      TabMirror.pausePainting();
      $(this.container).animate({left: left, top: top}, {complete: function() {
        TabMirror.resumePainting();
      }});
    }
  },

  
  setSize: function(width, height, immediately) {
    if(immediately)
      $(this.container).css({width: width, height: height});
    else {
      TabMirror.pausePainting();
      $(this.container).animate({width: width, height: height}, {complete: function() {
        TabMirror.resumePainting();
      }});
    }
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
  init: function() {
    var self = this;
    
    function mod($div){
      Utils.log('mod');
      if(window.Groups) {        
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
            var ffVersion = parseFloat(navigator.userAgent.match(/\d{8}.*(\d\.\d)/)[1]);
            if( ffVersion < 3.7 ) Utils.error("css-transitions require Firefox 3.7+");
            
            
            var [w,h,z] = [$(this).width(), $(this).height(), $(this).css("zIndex")];
            var origPos = $(this).position();
            var zIndex = $(this).css("zIndex");
            var scale = window.innerWidth/w;
            
            var tab = Tabs.tab(this);
            var mirror = tab.mirror;
            
            
            var overflow = $("body").css("overflow");
            $("body").css("overflow", "hidden");
  
            $(this).css("zIndex",99999).animate({
              top: -10, left: 0, easing: "easein",
              width:w*scale, height:h*scale}, 200, function(){
                $(this).find("canvas").data("link").tab.focus();
                $(this)
                  .css({top: origPos.top, left: origPos.left, width:w, height:h, zIndex:z});  
                Navbar.show();    
                $("body").css("overflow", overflow);          
              });
            
          } else {
            $(this).find("canvas").data("link").tab.raw.pos = $(this).position();
          }
        }
      });
      
      $("<div class='close'>x</div>").appendTo($div);
  
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