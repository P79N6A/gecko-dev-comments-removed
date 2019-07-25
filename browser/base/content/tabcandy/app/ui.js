

(function(){


Navbar = {
  
  get el(){
    var win = Utils.activeWindow;
    if(win) {
      var navbar = win.gBrowser.ownerDocument.getElementById("navigator-toolbox");
      return navbar;      
    }

    return null;
  },
  
  get urlBar(){
    var win = Utils.activeWindow;
    if(win) {
      var navbar = win.gBrowser.ownerDocument.getElementById("urlbar");
      return navbar;      
    }

    return null;    
  },

  
  show: function() {
    var el = this.el;
    if(el)
      el.collapsed = false; 
    else { 
      var self = this;
      setTimeout(function() {
        self.show();
      }, 300); 
    }
  },

  
  hide: function() {
    var el = this.el;
    if(el)
      el.collapsed = true; 
    else { 
      var self = this;
      setTimeout(function() {
        self.hide();
      }, 300); 
    }
  },
}


var Tabbar = {
  
  
  
  
  
  _hidden: false, 
  get el(){ return window.Tabs[0].raw.parentNode; },
  height: window.Tabs[0].raw.parentNode.getBoundingClientRect().height,
  hide: function(animate) {
    var self = this;
    this._hidden = true;
    
    if( animate == false ) speed = 0;
    else speed = 150;
    
    $(self.el).animate({"marginTop":-self.height}, speed, function(){
      self.el.collapsed = true;
    });
  },
  show: function(animate) {
    this._hidden = false;

    if( animate == false ) speed = 0;
    else speed = 150;
        
    this.el.collapsed = false;
    $(this.el).animate({"marginTop":0}, speed);
  },
  
  
  
  
  
  getVisibleTabs: function(){
    var visibleTabs = [];
    
    
    for( var i=0; i<UI.tabBar.el.children.length; i++ ){
      var tab = UI.tabBar.el.children[i];
      if( tab.collapsed == false )
        visibleTabs.push();
    }
    
    return visibleTabs;
  },
  
  
  
  
  
  
  
  showOnlyTheseTabs: function(tabs){
    var visibleTabs = [];
    
    
    var tabBarTabs = [];
    for( var i=0; i<UI.tabBar.el.children.length; i++ ){
      tabBarTabs.push(UI.tabBar.el.children[i]);
    }
    
    for each( var tab in tabs ){
      var rawTab = tab.tab.raw;
      var toShow = tabBarTabs.filter(function(testTab){
        return testTab == rawTab;
      }); 
      visibleTabs = visibleTabs.concat( toShow );
    }

    tabBarTabs.forEach(function(tab){
      tab.collapsed = true;
    });
    
    
    
    
    
    visibleTabs.forEach(function(tab){
      tab.collapsed = false;
      Utils.activeWindow.gBrowser.moveTabTo(tab, UI.tabBar.el.children.length-1);
    });
    
  },
  get isHidden(){ return this._hidden; }
}


window.Page = {
  startX: 30, 
  startY: 70,
  
  show: function(){
    Utils.homeTab.focus();
    UI.tabBar.hide(false);
  },
  
  setupKeyHandlers: function(){
    var self = this;
    $(window).keydown(function(e){
      if( !self.getActiveTab() ) return;
      
      var centers = [[item.bounds.center(), item] for each(item in TabItems.getItems())];
      myCenter = self.getActiveTab().bounds.center();

      function getClosestTabBy(norm){
        var matches = centers
          .filter(function(item){return norm(item[0], myCenter)})
          .sort(function(a,b){
            return myCenter.distance(a[0]) - myCenter.distance(b[0]);
          });
        if( matches.length > 0 ) return matches[0][1];
        else return null;
      }

      var norm = null;
      switch(e.which){
        case 39: 
          norm = function(a, me){return a.x > me.x};
          break;
        case 37: 
          norm = function(a, me){return a.x < me.x};
          break;
        case 40: 
          norm = function(a, me){return a.y > me.y};
          break;
        case 38: 
          norm = function(a, me){return a.y < me.y}
          break;
      }
      
      if( norm != null ){
        var nextTab = getClosestTabBy(norm);
        if( nextTab ){
          if( nextTab.inStack() && !nextTab.parent.expanded){
            nextTab = nextTab.parent.getChild(0);
          }
          self.setActiveTab(nextTab);           
        }
        e.preventDefault();               
      }      
    });
    
    $(window).keyup(function(e){
      
      
      if((e.which == 27 || e.which == 13) && $(":focus").length == 0 )
        if( self.getActiveTab() ) self.getActiveTab().zoom();
    });
  },
    
  
  init: function() {
    var self = this;
    Utils.homeTab.raw.maxWidth = 60;
    Utils.homeTab.raw.minWidth = 60;

    
    
    $(Utils.homeTab.contentDocument).mousedown(function(e){
      if( e.originalTarget.nodeName == "HTML" )
        Page.createGroupOnDrag(e)
    })

    this.setupKeyHandlers();
        
    Tabs.onClose(function(){
      
      
      var group = Groups.getActiveGroup();
      if( group && group._children.length == 0 )
        Page.show();

      
      
      
      if( group == null && Tabbar.getVisibleTabs().length == 0){
        Page.show();
      }

      return false;
    });
    
    var lastTab = null;
    Tabs.onFocus(function(){
      
      if( this.contentWindow == window && lastTab != null && lastTab.mirror != null){
        var activeGroup = Groups.getActiveGroup();
        if( activeGroup ) activeGroup.reorderBasedOnTabOrder();        

        UI.tabBar.hide(false);
        
        
        
        var $tab = $(lastTab.mirror.el);
        self.setActiveTab($(lastTab.mirror.el).data().tabItem);
        
        var rotation = $tab.css("-moz-transform");
        var [w,h, pos, z] = [$tab.width(), $tab.height(), $tab.position(), $tab.css("zIndex")];
        var scale = window.innerWidth / w;

        var overflow = $("body").css("overflow");
        $("body").css("overflow", "hidden");
        
        var mirror = lastTab.mirror;
        TabMirror.pausePainting();
        $tab.css({
            top: 0, left: 0,
            width: window.innerWidth,
            height: h * (window.innerWidth/w),
            zIndex: 999999,
            '-moz-transform': 'rotate(0deg)'
        }).animate({
            top: pos.top, left: pos.left,
            width: w, height: h
        },350, '', function() {
          $tab.css({
            zIndex: z,
            '-moz-transform': rotation
          });
          $("body").css("overflow", overflow);
          window.Groups.setActiveGroup(null);
          TabMirror.resumePainting();
        });
      }
      lastTab = this;
    });
  },
  
  
  createGroupOnDrag: function(e){

    const minSize = 60;
    
    var startPos = {x:e.clientX, y:e.clientY}
    var phantom = $("<div class='group'>").css({
      position: "absolute",
      top: startPos.y,
      left: startPos.x,
      width: 0,
      height: 0,
      opacity: .7,
      zIndex: -1,
      cursor: "default"
    }).appendTo("body");
    
    function updateSize(e){
      var css = {width: e.clientX-startPos.x, height:e.clientY-startPos.y}
      if( css.width > minSize || css.height > minSize ) css.opacity = 1;
      else css.opacity = .7
      
      phantom.css(css);
      e.preventDefault();     
    }
    
    function collapse(){
      phantom.animate({
        width: 0,
        height: 0,
        top: phantom.position().top + phantom.height()/2,
        left: phantom.position().left + phantom.width()/2
      }, 300, function(){
        phantom.remove();
      })
    }
    
    function finalize(e){
      $("html").unbind("mousemove");
      if( phantom.css("opacity") != 1 ) collapse();
      else{
        var bounds = new Rect(startPos.x, startPos.y, phantom.width(), phantom.height())

        
        
        var tabs = Groups.getOrphanedTabs();
        var insideTabs = [];
        for each( tab in tabs ){
          if( bounds.contains( tab.bounds ) ){
            insideTabs.push(tab);
          }
        }
        
        var group = new Group(insideTabs,{bounds:bounds});
        phantom.remove();
      }
    }
    
    $("html").mousemove(updateSize)
    $("html").one('mouseup',finalize);
    return false;
  },
  
  
  
  
  
  
  
  
  
  
  
  setActiveTab: function(tab){
    if( this._activeTab ) this._activeTab.makeDeactive();
    this._activeTab = tab;
    tab.makeActive();
  },
  
  
  
  
  
  getActiveTab: function(){
    return this._activeTab;
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
    return $("<a class='action' href='#'/>").text(name).css({margin:5}).appendTo("#actions");
  }
}


function UIClass(){ 
  this.navBar = Navbar;
  this.tabBar = Tabbar;
  this.devMode = false;
  this.focused = true;
  
  var self = this;
  
  
  var params = document.location.search.replace('?', '').split('&');
  $.each(params, function(index, param) {
    var parts = param.split('=');
    if(parts[0] == 'dev' && parts[1] == '1') 
      self.devMode = true;
  });
  
  
  if(this.devMode) {
    Switch.insert('body', '');
    $('<br><br>').appendTo("#actions");
    this._addArrangements();
  }
  
  
  this.navBar.hide();
  
  Tabs.onFocus(function() {
    try{
      if(this.contentWindow.location.host == "tabcandy") {
        self.focused = true;
        self.navBar.hide();
      } else {
        self.focused = false;
        self.navBar.show();      
      }
    }catch(e){
      Utils.log()
    }
  });

  Tabs.onOpen(function(a, b) {
    self.navBar.show();
  });

  
  Page.init();
  
  
  var data = Storage.read();
  var sane = this.storageSanity(data);
  if(!sane || data.dataVersion < 2) {
    data.groups = null;
    data.tabs = null;
    data.pageBounds = null;
    
    if(!sane)
      alert('storage data is bad; starting fresh');
  }
   
  Groups.reconstitute(data.groups);
  TabItems.reconstitute(data.tabs);
  
  $(window).bind('beforeunload', function() {
    if(self.initialized) 
      self.save();
  });
  
  
  if(data.pageBounds) {
    this.pageBounds = data.pageBounds;
    this.resize();
  } else 
    this.pageBounds = Items.getPageBounds();    
  
  $(window).resize(function() {
    self.resize();
  });
  
  
  this.addDevMenu();
  
  
  this.initialized = true;
};


UIClass.prototype = {
  
  resize: function() {

    
    var items = Items.getTopLevelItems();
    var itemBounds = new Rect(this.pageBounds);
    itemBounds.width = 1;
    itemBounds.height = 1;
    $.each(items, function(index, item) {
      if(item.locked.bounds)
        return;
        
      var bounds = item.getBounds();
      itemBounds = (itemBounds ? itemBounds.union(bounds) : new Rect(bounds));
    });

    var oldPageBounds = new Rect(this.pageBounds);
    
    var newPageBounds = Items.getPageBounds();
    if(newPageBounds.width < this.pageBounds.width && newPageBounds.width > itemBounds.width)
      newPageBounds.width = this.pageBounds.width;

    if(newPageBounds.height < this.pageBounds.height && newPageBounds.height > itemBounds.height)
      newPageBounds.height = this.pageBounds.height;

    var wScale;
    var hScale;
    if(Math.abs(newPageBounds.width - this.pageBounds.width)
        > Math.abs(newPageBounds.height - this.pageBounds.height)) {
      wScale = newPageBounds.width / this.pageBounds.width;
      hScale = newPageBounds.height / itemBounds.height;
    } else {
      wScale = newPageBounds.width / itemBounds.width;
      hScale = newPageBounds.height / this.pageBounds.height;
    }
    
    var scale = Math.min(hScale, wScale);
    var self = this;
    var pairs = [];
    $.each(items, function(index, item) {
      if(item.locked.bounds)
        return;
        
      var bounds = item.getBounds();

      bounds.left += newPageBounds.left - self.pageBounds.left;
      bounds.left *= scale;
      bounds.width *= scale;

      bounds.top += newPageBounds.top - self.pageBounds.top;            
      bounds.top *= scale;
      bounds.height *= scale;
      
      pairs.push({
        item: item,
        bounds: bounds
      });
    });
    
    Items.unsquish(pairs);
    
    $.each(pairs, function(index, pair) {
      pair.item.setBounds(pair.bounds, true);
    });

    this.pageBounds = Items.getPageBounds();
  },
  
  
  addDevMenu: function() {
    var self = this;
    
    var html = '<select style="position:absolute; bottom:5px; right:5px; opacity:.2;">'; 
    var $select = $(html)
      .appendTo('body')
      .change(function () {
        var index = $(this).val();
        commands[index].code();
        $(this).val(0);
      });
      
    var commands = [{
      name: 'dev menu', 
      code: function() {
      }
    }, {
      name: 'home', 
      code: function() {
        location.href = '../../index.html';
      }
    }, {
      name: 'save', 
      code: function() {
        self.save();
      }
    }];
      
    var count = commands.length;
    var a;
    for(a = 0; a < count; a++) {
      html = '<option value="'
        + a
        + '">'
        + commands[a].name
        + '</option>';
        
      $select.append(html);
    }
  },

  
  save: function() {  
    var data = {
      dataVersion: 2,
      groups: Groups.getStorageData(),
      tabs: TabItems.getStorageData(), 
      pageBounds: Items.getPageBounds()
    };
    
    if(this.storageSanity(data))
      Storage.write(data);
    else
      alert('storage data is bad; reverting to previous version');
  },

  
  storageSanity: function(data) {
    if($.isEmptyObject(data))
      return true;
      
    var sane = true;
    sane = sane && typeof(data.dataVersion) == 'number';
    sane = sane && isRect(data.pageBounds);
    
    if(data.tabs)
      sane = sane && TabItems.storageSanity(data.tabs);
      
    if(data.groups)
      sane = sane && Groups.storageSanity(data.groups);
    
    return sane;
  },
  
  
  _addArrangements: function() {
    this.grid = new ArrangeClass("Grid", function(value) {
      if(typeof(Groups) != 'undefined')
        Groups.removeAll();
    
      var immediately = false;
      if(typeof(value) == 'boolean')
        immediately = value;
    
      var box = new Rect(Page.startX, Page.startY, TabItems.tabWidth, TabItems.tabHeight); 
      $(".tab:visible").each(function(i){
        var item = Items.item(this);
        item.setBounds(box, immediately);
        
        box.left += box.width + 30;
        if( box.left > window.innerWidth - (box.width + Page.startX)){ 
          box.left = Page.startX;
          box.top += box.height + 30;
        }
      });
    });
        
    this.site = new ArrangeClass("Site", function() {
      Groups.removeAll();
      
      var groups = [];
      $(".tab:visible").each(function(i) {
        $el = $(this);
        var tab = Tabs.tab(this);
        
        var url = tab.url; 
        var domain = url.split('/')[2]; 
        var domainParts = domain.split('.');
        var mainDomain = domainParts[domainParts.length - 2];
        if(groups[mainDomain]) 
          groups[mainDomain].push($(this));
        else 
          groups[mainDomain] = [$(this)];
      });
      
      var createOptions = {dontPush: true, dontArrange: true};
      
      var leftovers = [];
      for(key in groups) {
        var set = groups[key];
        if(set.length > 1) {
          group = new Group(set, createOptions);
        } else
          leftovers.push(set[0]);
      }
      
    
        group = new Group(leftovers, createOptions);
    
      
      Groups.arrange();
    });
  },
  
  
  newTab: function(url, inBackground) {
    Tabs.open(url, inBackground);
  }
};


window.UI = new UIClass();

})();

