

(function(){

window.Keys = {meta: false};


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
  
  
  get el() {
    return window.Tabs[0].raw.parentNode; 
  },

  
  get height() {
    return window.Tabs[0].raw.parentNode.getBoundingClientRect().height;
  },

  
  hide: function() {
    var self = this;
    self.el.collapsed = true;    
  },

  
  show: function() {
    this.el.collapsed = false;
  },
  
  
  
  
  
  getVisibleTabs: function(){
    var visibleTabs = [];
    
    
    for( var i=0; i<this.el.children.length; i++ ){
      var tab = this.el.children[i];
      if( tab.collapsed == false )
        visibleTabs.push();
    }
    
    return visibleTabs;
  },
  
  
  
  
  
  
  
  showOnlyTheseTabs: function(tabs){
    var visibleTabs = [];
    
    
    var tabBarTabs = [];
    for( var i=0; i<this.el.children.length; i++ ){
      tabBarTabs.push(this.el.children[i]);
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
    
    
    
    
    
    var self = this;
    visibleTabs.forEach(function(tab){
      tab.collapsed = false;
      Utils.activeWindow.gBrowser.moveTabTo(tab, self.el.children.length-1);
    });
  },

  
  
  
  showAllTabs: function(){
    for( var i=0; i<this.el.children.length; i++ ){
      var tab = this.el.children[i];
      tab.collapsed = false;
    }
  },

  
  get isHidden(){ return this.el.collapsed; }
}




window.Page = {
  startX: 30, 
  startY: 70,
  
  show: function(){
    Utils.homeTab.focus();
    this.hideChrome();
  },
  
  isTabCandyFocused: function(){
    return Utils.homeTab.contentDocument == UI.currentTab.contentDocument;    
  },
  
  hideChrome: function(){
    Tabbar.hide();
    Navbar.hide();
    window.statusbar.visible = false;  
        
    
    Utils.activeWindow.document.getElementById("main-window").setAttribute("activetitlebarcolor", "#C4C4C4");
  },
  
  showChrome: function(){
    Tabbar.show();  
    Navbar.show();    
    window.statusbar.visible = true;
    
    
    Utils.activeWindow.document.getElementById("main-window").removeAttribute("activetitlebarcolor");     
  },
  
  setupKeyHandlers: function(){
    var self = this;
    $(window).keyup(function(e){
      if( e.metaKey == false ) window.Keys.meta = false;
    });
    
    $(window).keydown(function(e){
      if( e.metaKey == true ) window.Keys.meta = true;
      
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
        case 49: 
        case 69: 
          if( Keys.meta ) if( self.getActiveTab() ) self.getActiveTab().zoom();
          break;
      }
      
      if( norm != null && $(":focus").length == 0 ){
        var nextTab = getClosestTabBy(norm);
        if( nextTab ){
          if( nextTab.inStack() && !nextTab.parent.expanded){
            nextTab = nextTab.parent.getChild(0);
          }
          self.setActiveTab(nextTab);           
        }
        e.preventDefault();               
      }
      
      if((e.which == 27 || e.which == 13) && $(":focus").length == 0 )
        if( self.getActiveTab() ) self.getActiveTab().zoom();
      
      
       
    });
    
  },
    
  
  init: function() {
    var self = this;
    Utils.homeTab.raw.maxWidth = 60;
    Utils.homeTab.raw.minWidth = 60;
        
    
    
    $(Utils.homeTab.contentDocument).mousedown(function(e){
      if( e.originalTarget.id == "bg" )
        Page.createGroupOnDrag(e)
    })

    this.setupKeyHandlers();
        
    Tabs.onClose(function(){
      setTimeout(function() { 
        
        
        var group = Groups.getActiveGroup();
        if( group && group._children.length == 0 )
          Page.show();
  
        
        
        
        if( group == null && Tabbar.getVisibleTabs().length == 0){
          Page.show();
        }
      }, 1);

      return false;
    });
    
    Tabs.onFocus(function() {
      var focusTab = this;

      
      if( focusTab.contentWindow == window ){
        var currentTab = UI.currentTab;
        if(currentTab != null && currentTab.mirror != null) {
          
          
          
          UI.resize(true);
          
          
          var mirror = currentTab.mirror;
          var $tab = $(mirror.el);
          var item = $tab.data().tabItem;
          self.setActiveTab(item);
          
          var rotation = $tab.css("-moz-transform");
          var [w,h, pos, z] = [$tab.width(), $tab.height(), $tab.position(), $tab.css("zIndex")];
          var scale = window.innerWidth / w;
  
          var overflow = $("body").css("overflow");
          $("body").css("overflow", "hidden");
          
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
            var activeGroup = Groups.getActiveGroup();
            if( activeGroup ) activeGroup.reorderBasedOnTabOrder(item);        
    
            
            window.Groups.setActiveGroup(null);
            TabMirror.resumePainting();            
          });
        }
      } else { 
        setTimeout(function() { 
          var item = TabItems.getItemByTab(Utils.activeTab);
          if(item) 
            Groups.setActiveGroup(item.parent);
            
          UI.tabBar.show();        
        }, 1);
      }
      
      UI.currentTab = focusTab;
    });
  },
  
  
  createGroupOnDrag: function(e){

    const minSize = 60;
    
    var startPos = {x:e.clientX, y:e.clientY}
    var phantom = $("<div class='group phantom'>").css({
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
      $("#bg, .phantom").unbind("mousemove");
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
    
    $("#bg, .phantom").mousemove(updateSize)
    $(window).one('mouseup', finalize);
    e.preventDefault();  
    return false;
  },
  
  
  
  
  
  
  
  
  
  
  
  setActiveTab: function(tab){
    if(tab == this._activeTab)
      return;
      
    if(this._activeTab) { 
      this._activeTab.makeDeactive();
      this._activeTab.removeOnClose(this);
    }
      
    this._activeTab = tab;
    
    if(this._activeTab) {
      var self = this;
      this._activeTab.addOnClose(this, function() {
        self._activeTab = null;
      });

      this._activeTab.makeActive();
    }
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
  if(window.Tabs) 
    this.init();
  else { 
    var self = this;
    TabsManager.addSubscriber(this, 'load', function() {
      self.init();
    });
  }
};


UIClass.prototype = {
  
  init: function() {
    try {
      
      
      this.navBar = Navbar;
      
      
      
      this.tabBar = Tabbar;
      
      
      
      
      this.devMode = false;
      
      
      
      
      this.currentTab = Utils.activeTab;
      
      
      
      this.focused = (Utils.activeTab == Utils.homeTab);
      
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
      
      
      if(this.focused) {
        Page.hideChrome();
      }
      
      Tabs.onFocus(function() {
        var me = this;
        setTimeout(function() { 
          try{
            if(me.contentWindow.location.host == "tabcandy") {
              self.focused = true;
              Page.hideChrome();
            } else {
              self.focused = false;
              Page.showChrome();
            }
          }catch(e){
            Utils.log(e)
          }
        }, 1);
      });
    
      Tabs.onOpen(function(a, b) {
        setTimeout(function() { 
          self.navBar.show();
        }, 1);
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
       
      var groupsData = Storage.readGroupsData(Utils.activeWindow);
      var groupData = Storage.readGroupData(Utils.activeWindow);
        






      Groups.reconstitute(groupsData, groupData);
      TabItems.init();
      TabItems.reconstitute();
      
      $(window).bind('beforeunload', function() {
        if(self.initialized) 
          self.save();
          
        self.showChrome();  
        self.tabBar.showAllTabs();
      });
      
      
      data.pageBounds = null;
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
    }catch(e) {
      Utils.log("Error in UIClass(): " + e);
      Utils.log(e.fileName);
      Utils.log(e.lineNumber);
      Utils.log(e.stack);
    }
  }, 
  
  
  resize: function(force) {

    if( typeof(force) == "undefined" ) force = false;

    
    
    var isAnimating = $(":animated").length > 0;
    if( force == false){
      if( isAnimating || !Page.isTabCandyFocused() ) return;   }   
        
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
    if(newPageBounds.equals(oldPageBounds))
      return;
      

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
    return;
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

