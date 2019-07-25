









































(function(){

window.Keys = {meta: false};


Navbar = {
  
  get el(){
    var win = Utils.getCurrentWindow();
    if(win) {
      var navbar = win.gBrowser.ownerDocument.getElementById("navigator-toolbox");
      return navbar;      
    }

    return null;
  },
  
  get urlBar(){
    var win = Utils.getCurrentWindow();
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
      iQ.timeout(function() {
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
      iQ.timeout(function() {
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
  
  
  
  
  
  
  
  showOnlyTheseTabs: function(tabs, options){
    try { 
      if(!options)
        options = {};
          
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
        
        if(!options.dontReorg)
          Utils.getCurrentWindow().gBrowser.moveTabTo(tab, self.el.children.length-1);
      });
    } catch(e) {
      Utils.log(e);
    }
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
        
    
    Utils.getCurrentWindow().document.getElementById("main-window").setAttribute("activetitlebarcolor", "#C4C4C4");
  },
  
  showChrome: function(){
    Tabbar.show();  
    Navbar.show();    
    window.statusbar.visible = true;
    
    
    Utils.getCurrentWindow().document.getElementById("main-window").removeAttribute("activetitlebarcolor");     
  },
  
  setupKeyHandlers: function(){
    var self = this;
    iQ(window).keyup(function(e){
      if( e.metaKey == false ) window.Keys.meta = false;
    });
    
    iQ(window).keydown(function(e){
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
          if( Keys.meta ) if( self.getActiveTab() ) self.getActiveTab().zoomIn();
          break;
      }
      
      if( norm != null && iQ(":focus").length == 0 ){
        var nextTab = getClosestTabBy(norm);
        if( nextTab ){
          if( nextTab.inStack() && !nextTab.parent.expanded){
            nextTab = nextTab.parent.getChild(0);
          }
          self.setActiveTab(nextTab);           
        }
        e.preventDefault();               
      }
      
      if((e.which == 27 || e.which == 13) && iQ(":focus").length == 0 )
        if( self.getActiveTab() ) self.getActiveTab().zoomIn();
      
      
       
    });
    
  },
    
  
  init: function() {
    var self = this;




        
    
    
    iQ(Utils.homeTab.contentDocument).mousedown(function(e){
      if( e.originalTarget.id == "bg" )
        Page.createGroupOnDrag(e)
    })

    this.setupKeyHandlers();
        
    Tabs.onClose(function(){
      iQ.timeout(function() { 
        
        
        var group = Groups.getActiveGroup();
        if( group && group._children.length == 0 )
          Page.show();
  
        
        
        
        if( group == null && Tabbar.getVisibleTabs().length == 0){
          Page.show();
        }
      }, 1);

      return false;
    });
    
    Tabs.onMove(function() {
      iQ.timeout(function() { 
        var activeGroup = Groups.getActiveGroup();
        if( activeGroup )
          activeGroup.reorderBasedOnTabOrder();                
      }, 1);
    });
    
    Tabs.onFocus(function() {
      var focusTab = this;
      var currentTab = UI.currentTab;
      
      
      if( focusTab.contentWindow == window ){
        UI.focused = true;
        Page.hideChrome();

        var item = null;
        if(currentTab && currentTab.mirror)
          item = TabItems.getItemByTabElement(currentTab.mirror.el);
        
        if(item) {
          
          
          
          
          
          item.zoomOut(function() {
            if(!currentTab.mirror) 
              item = null;
            
            self.setActiveTab(item);
            
            var activeGroup = Groups.getActiveGroup();
            if( activeGroup )
              activeGroup.setTopChild(item);        
    
            window.Groups.setActiveGroup(null);
            UI.resize(true);
          });
        }
      } else { 
        UI.focused = false;
        Page.showChrome();

        iQ.timeout(function() { 
          if(focusTab != UI.currentTab) 
            return;
            
          var newItem = null;
          if(focusTab && focusTab.mirror)
            newItem = TabItems.getItemByTabElement(focusTab.mirror.el);

          if(newItem) 
            Groups.setActiveGroup(newItem.parent);
            
          UI.tabBar.show();  
          
          
          var oldItem = null;
          if(currentTab && currentTab.mirror)
            oldItem = TabItems.getItemByTabElement(currentTab.mirror.el);
            
          if(newItem != oldItem) {
            if(oldItem)
              oldItem.setZoomPrep(false);

            if(newItem)
              newItem.setZoomPrep(true);
          }
        }, 1);
      }
      
      UI.currentTab = focusTab;
    });
  },
  
  
  createGroupOnDrag: function(e){

    const minSize = 60;
    const minMinSize = 15;
    
    var startPos = {x:e.clientX, y:e.clientY}
    var phantom = iQ("<div>")
      .addClass('group phantom')
      .css({
        position: "absolute",
        top: startPos.y,
        left: startPos.x,
        width: 0,
        height: 0,
        opacity: .7,
        zIndex: -1,
        cursor: "default"
      })
      .appendTo("body");
    
    function updateSize(e){
      var box = new Rect();
      box.left = Math.min(startPos.x, e.clientX);
      box.right = Math.max(startPos.x, e.clientX);
      box.top = Math.min(startPos.y, e.clientY);
      box.bottom = Math.max(startPos.y, e.clientY);

      var css = box.css();      
      if(css.width > minMinSize && css.height > minMinSize
          && (css.width > minSize || css.height > minSize)) 
        css.opacity = 1;
      else 
        css.opacity = .7
      
      phantom.css(css);
      e.preventDefault();     
    }
    
    function collapse(){
      phantom.animate({
        width: 0,
        height: 0,
        top: phantom.position().top + phantom.height()/2,
        left: phantom.position().left + phantom.width()/2
      }, {
        duration: 300,
        complete: function() {
          phantom.remove();
        }
      });
    }
    
    function finalize(e){
      iQ(window).unbind("mousemove", updateSize);
      if( phantom.css("opacity") != 1 ) 
        collapse();
      else{
        var bounds = phantom.bounds();

        
        
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
    
    iQ(window).mousemove(updateSize)
    iQ(window).one('mouseup', finalize);
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
      
      
      this.addDevMenu();

      iQ("#reset").click(function(){
        self.reset();
      });
      
      
      if(this.focused) {
        Page.hideChrome();
      }
      
      Tabs.onOpen(function(a, b) {
        iQ.timeout(function() { 
          self.navBar.show();
        }, 1);
      });
    
      iQ(window).bind('beforeunload', function() {
        
        if(self.showChrome)
          self.showChrome();  
          
        if(self.tabBar && self.tabBar.showAllTabs)
          self.tabBar.showAllTabs();
      });
      
      
      Page.init();
      
      
      var currentWindow = Utils.getCurrentWindow();
      
      var data = Storage.readUIData(currentWindow);
      this.storageSanity(data);
       
      var groupsData = Storage.readGroupsData(currentWindow);
      var firstTime = !groupsData || iQ.isEmptyObject(groupsData);
      var groupData = Storage.readGroupData(currentWindow);
      Groups.reconstitute(groupsData, groupData);
      
      TabItems.init();
      
      if(firstTime) {
        var items = TabItems.getItems();
        iQ.each(items, function(index, item) {
          if(item.parent)
            item.parent.remove(item);
        });
            
        var box = Items.getPageBounds();
        box.inset(10, 10);
        var options = {padding: 10};
        Items.arrange(items, box, options);
      } else
        TabItems.reconstitute();
      
      
      if(data.pageBounds) {
        this.pageBounds = data.pageBounds;
        this.resize(true);
      } else 
        this.pageBounds = Items.getPageBounds();    
      
      iQ(window).resize(function() {
        self.resize();
      });
            
      
      this.initialized = true;
      this.save(); 
    }catch(e) {
      Utils.log("Error in UIClass(): " + e);
      Utils.log(e.fileName);
      Utils.log(e.lineNumber);
      Utils.log(e.stack);
    }
  }, 
  
  
  resize: function(force) {
    if( typeof(force) == "undefined" ) force = false;

    
    
    var isAnimating = iQ.isAnimating();
    if( force == false){
      if( isAnimating || !Page.isTabCandyFocused() ) return;   }   
        
    var items = Items.getTopLevelItems();
    var itemBounds = new Rect(this.pageBounds);
    itemBounds.width = 1;
    itemBounds.height = 1;
    iQ.each(items, function(index, item) {
      if(item.locked.bounds)
        return;
        
      var bounds = item.getBounds();
      itemBounds = (itemBounds ? itemBounds.union(bounds) : new Rect(bounds));
    });

    var oldPageBounds = new Rect(this.pageBounds);
    
    var newPageBounds = Items.getPageBounds();
    if(newPageBounds.equals(oldPageBounds))
      return;
      
    Groups.repositionNewTabGroup(); 

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
    iQ.each(items, function(index, item) {
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
    
    iQ.each(pairs, function(index, pair) {
      pair.item.setBounds(pair.bounds, true);
    });

    this.pageBounds = Items.getPageBounds();
    this.save();
  },
  
  
  addDevMenu: function() {
    var self = this;
    
    var $select = iQ('<select>')
      .css({
        position: 'absolute',
        bottom: 5,
        right: 5,
        opacity: .2
      })
      .appendTo('body')
      .change(function () {
        var index = iQ(this).val();
        try {
          commands[index].code.apply(commands[index].element);
        } catch(e) {
          Utils.log('dev menu error', e);
        }
        iQ(this).val(0);
      });
      
    var commands = [{
      name: 'dev menu', 
      code: function() {
      }
    }, {
      name: 'show trenches', 
      code: function() {
        Trenches.toggleShown();
        iQ(this).html((Trenches.showDebug ? 'hide' : 'show') + ' trenches');
      }
    }, {
      name: 'code docs', 
      code: function() {
        location.href = 'doc/index.html';
      }
    }, {
      name: 'tests', 
      code: function() {
        location.href = 'test.html';
      }
    }, {
      name: 'save', 
      code: function() {
        self.saveAll();
      }
    }, {
      name: 'group sites', 
      code: function() {
        self.arrangeBySite();
      }
    }];
      
    var count = commands.length;
    var a;
    for(a = 0; a < count; a++) {
      commands[a].element = iQ('<option>')
        .val(a)
        .html(commands[a].name)
        .appendTo($select)
        .get(0);
    }
  },

  
  reset: function() {
    Storage.wipe();
    location.href = '';      
  },
    
  
  saveAll: function() {  
    this.save();
    Groups.saveAll();
    TabItems.saveAll();
  },

  
  save: function() {  
    if(!this.initialized) 
      return;
      
    var data = {
      pageBounds: this.pageBounds
    };
    
    if(this.storageSanity(data))
      Storage.saveUIData(Utils.getCurrentWindow(), data);
  },

  
  storageSanity: function(data) {
    if(iQ.isEmptyObject(data))
      return true;
      
    if(!isRect(data.pageBounds)) {
      Utils.log('UI.storageSanity: bad pageBounds', data.pageBounds);
      data.pageBounds = null;
      return false;
    }
      
    return true;
  },
  
  
  arrangeBySite: function() {
    function putInGroup(set, key) {
      var group = Groups.getGroupWithTitle(key);
      if(group) {
        iQ.each(set, function(index, el) {
          group.add(el);
        });
      } else 
        new Group(set, {dontPush: true, dontArrange: true, title: key});      
    }

    Groups.removeAll();
    
    var newTabsGroup = Groups.getNewTabGroup();
    var groups = [];
    var items = TabItems.getItems();
    iQ.each(items, function(index, item) {
      var url = item.getURL(); 
      var domain = url.split('/')[2]; 
      if(!domain)
        newTabsGroup.add(item);
      else {
        var domainParts = domain.split('.');
        var mainDomain = domainParts[domainParts.length - 2];
        if(groups[mainDomain]) 
          groups[mainDomain].push(item.container);
        else 
          groups[mainDomain] = [item.container];
      }
    });
    
    var leftovers = [];
    for(key in groups) {
      var set = groups[key];
      if(set.length > 1) {
        putInGroup(set, key);
      } else
        leftovers.push(set[0]);
    }
    
    putInGroup(leftovers, 'mixed');
    
    Groups.arrange();
  },
  
  
  newTab: function(url, inBackground) {
    Tabs.open(url, inBackground);
  }
};


window.UI = new UIClass();

})();

