(function(){

$.expr[':'].icontains = function(obj, index, meta, stack){
return (obj.textContent || obj.innerText || jQuery(obj).text() || '').toLowerCase().indexOf(meta[3].toLowerCase()) >= 0;
};

Navbar = {
  get el(){
    var win = Utils.activeWindow;
    var navbar = win.gBrowser.ownerDocument.getElementById("navigator-toolbox");
    return navbar;      
  },
  show: function(){ this.el.collapsed = false; },
  hide: function(){ this.el.collapsed = true;}
}

var Tabbar = {
  get el(){ return window.Tabs[0].raw.parentNode; },
  hide: function(){ this.el.collapsed = true },
  show: function(){ this.el.collapsed = false }
}

var Page = {
  init: function(){
    function mod($div, tab){
      if(window.Groups) {        
        $div.draggable(window.Groups.dragOptions);
        $div.droppable(window.Groups.dropOptions);
      }
              
      $div.mouseup(function(e){
        if( e.target.className == "close" ){
          $(this).find("canvas").data("link").tab.close(); }
        else {
          if(!$(this).data('isDragging')) {
            var ffVersion = parseFloat(navigator.userAgent.match(/\d{8}.*(\d\.\d)/)[1]);
            if( ffVersion < 3.7 ) Utils.error("css-transitions require Firefox 3.7+");
            
            
            var [w,h,z] = [$(this).width(), $(this).height(), $(this).css("zIndex")];
            var origPos = $(this).position();
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
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
    }
    
    window.TabMirror.customize(mod);
    
    Utils.homeTab.raw.maxWidth = 60;
    Utils.homeTab.raw.minWidth = 60;
    
    Tabs.onClose(function(){
      Utils.homeTab.focus();
      return false;
    });
    
    var lastTab = null;
    Tabs.onFocus(function(){
      
      if( this.contentWindow == window && lastTab != null){
        
        
        var $tab = $(lastTab.mirror.el);
        
        var [w,h, pos, z] = [$tab.width(), $tab.height(), $tab.position(), $tab.css("zIndex")];
        var scale = window.innerWidth / w;

        var overflow = $("body").css("overflow");
        $("body").css("overflow", "hidden");
        
        var mirror = lastTab.mirror;
        $tab.css({
            top: 0, left: 0,
            width: window.innerWidth,
            height: h * (window.innerWidth/w),
            zIndex: 999999,
        }).animate({
            top: pos.top, left: pos.left,
            width: w, height: h
        },250, '', function() {
          $tab.css("zIndex",z);
          $("body").css("overflow", overflow);
        });
      }
      lastTab = this;
    });
    
    $("#tabbar").toggle(
      function(){Tabbar.hide()},
      function(){Tabbar.show()}      
    );
    
    Page.initSearch();
  },
  
  initSearch: function(){
    $search = $(".search input");
    $search.val("").focus();
    $search.keydown(function(evt){
           
      if( evt.which == 13 ){
                
        Navbar.show();
        
        if ($(".tab:not(.unhighlight)").length == 1) {
          $(".tab:not(.unhighlight)").find("canvas").data("link").tab.focus();
        } else {
          Tabs.open( "http://google.com/search?q=" + $search.val() ).focus();
        }
        $search.val("")          
        $(".tab .name").parent().removeClass("unhighlight");
        
      }
    });
    
    $search.keyup(function(evt){
      if( $search.val().length > 0 )
        var $found = $(".tab .name:not(:icontains(" + $search.val() + "))").parent();
      else 
        var $found = [];
      
      if( $search.val().length > 1 ){
        $found.addClass("unhighlight");
        $(".tab .name:icontains(" + $search.val() + ")").parent().removeClass("unhighlight");      
      }
      else {
        $(".tab .name").parent().removeClass("unhighlight");
      }
      
      if ( $found.length == 1 ) {
        $found.click();
        
      }  
      
    });
    
    



    
    $(window).blur(function(){
      Navbar.show();
    })
  }
}


function ArrangeClass(name, func){ this.init(name, func); };
ArrangeClass.prototype = {
  init: function(name, func){
    this.$el = this._create(name);
    this.arrange = func;
    if(func) this.$el.click(func);
  },
  
  _create: function(name){
    return $("<a href='#'/>").text(name).appendTo("#actions");
  }
}

var grid = new ArrangeClass("Grid", function(){    
  var x = 10;
  var y = 100;
  $(".tab:visible").each(function(i){
    $el = $(this);
    
    var oldPos = $el.find("canvas").data("link").tab.raw.pos;
    if( oldPos ){
      $el.css({top:oldPos.top, left:oldPos.left});
      return;
    }
    
    $el.css({top: y,left: x});
    x += $el.width() + 10;
    if( x > window.innerWidth - $el.width() ){
      x = 10;
      y += $el.height() + 30;
    }
    
    
  });
});


var Arrange = {
  init: function(){
    grid.arrange();    
  }
}

function UIClass(){ this.init(); };
UIClass.prototype = {
  navbar: Navbar,
  tabbar: Tabbar,
  init: function(){
    Page.init();
    Arrange.init();

  }
}

var UI = new UIClass();
window.UI = UI;


})();

