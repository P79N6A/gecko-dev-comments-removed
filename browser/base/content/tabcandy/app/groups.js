










































(function(){





var numCmp = function(a,b){ return a-b; }





function min(list){ return list.slice().sort(numCmp)[0]; }





function max(list){ return list.slice().sort(numCmp).reverse()[0]; }






















window.Group = function Group(listOfEls, options) {
  try {
  if (typeof(options) == 'undefined')
    options = {};

  this._inited = false;
  this._children = []; 
  this.defaultSize = new Point(TabItems.tabWidth * 1.5, TabItems.tabHeight * 1.5);
  this.isAGroup = true;
  this.id = options.id || Groups.getNextID();
  this._isStacked = false;
  this._stackAngles = [0];
  this.expanded = null;
  this.locked = (options.locked ? Utils.copy(options.locked) : {});
  this.topChild = null;

  this.keepProportional = false;

  
  
  this._activeTab = null;

   
   
   
   
   
  this.xDensity = 0;
  this.yDensity = 0;

  if (Utils.isPoint(options.userSize))
    this.userSize = new Point(options.userSize);

  var self = this;

  var rectToBe;
  if (options.bounds) {
    Utils.assert("options.bounds must be a Rect",Utils.isRect(options.bounds));
    rectToBe = new Rect(options.bounds);
  }

  if (!rectToBe) {
    rectToBe = Groups.getBoundingBox(listOfEls);
    rectToBe.inset( -30, -30 );
  }

  var $container = options.container;
  if (!$container) {
    $container = iQ('<div>')
      .addClass('group')
      .css({position: 'absolute'})
      .css(rectToBe);

    if ( this.isNewTabsGroup() ) $container.addClass("newTabGroup");
  }

  this.bounds = $container.bounds();

  this.isDragging = false;
  $container
    .css({zIndex: -100})
    .appendTo("body");


  
  this.$ntb = iQ("<div>")
    .appendTo($container);

  this.$ntb
    .addClass(this.isNewTabsGroup() ? 'newTabButtonAlt' : 'newTabButton')
    .click(function(){
      self.newTab();
    });

  this.$ntb.get(0).title = 'New tab';

  if ( this.isNewTabsGroup() ) this.$ntb.html("<span>+</span>");

  
  this.$resizer = iQ("<div>")
    .addClass('resizer')
    .css({
      position: "absolute",
      width: 16, height: 16,
      bottom: 0, right: 0,
    })
    .appendTo($container)
    .hide();

  
  var html =
    "<div class='title-container'>" +
      "<input class='name' value='" + (options.title || "") + "'/>" +
      "<div class='title-shield' />" +
    "</div>";

  this.$titlebar = iQ('<div>')
    .addClass('titlebar')
    .html(html)
    .appendTo($container);

  this.$titlebar.css({
      position: "absolute",
    });

  var $close = iQ('<div>')
    .addClass('close')
    .click(function() {
      self.closeAll();
    })
    .appendTo($container);

  
  this.$titleContainer = iQ('.title-container', this.$titlebar);
  this.$title = iQ('.name', this.$titlebar);
  this.$titleShield = iQ('.title-shield', this.$titlebar);

  var titleUnfocus = function() {
    self.$titleShield.show();
    if (!self.getTitle()) {
      self.$title
        .addClass("defaultName")
        .val(self.defaultName);
    } else {
      self.$title
        .css({"background":"none"})
        .animate({
          "padding-left": "1px"
        }, {
          duration: 200,
          easing: 'tabcandyBounce'
        });
    }
  };

  var handleKeyPress = function(e){
    if ( e.which == 13 ) { 
      self.$title.get(0).blur();
      self.$title
        .addClass("transparentBorder")
        .one("mouseout", function(){
          self.$title.removeClass("transparentBorder");
        });
    } else
      self.adjustTitleSize();

    self.save();
  };

  this.$title
    .css({backgroundRepeat: 'no-repeat'})
    .blur(titleUnfocus)
    .focus(function() {
      if (self.locked.title) {
        self.$title.get(0).blur();
        return;
      }
      self.$title.get(0).select();
      if (!self.getTitle()) {
        self.$title
          .removeClass("defaultName")
          .val('');
      }
    })
    .keyup(handleKeyPress);

  titleUnfocus();

  if (this.locked.title)
    this.$title.addClass('name-locked');
  else {
    this.$titleShield
      .mousedown(function(e) {
        self.lastMouseDownTarget = (Utils.isRightClick(e) ? null : e.target);
      })
      .mouseup(function(e) {
        var same = (e.target == self.lastMouseDownTarget);
        self.lastMouseDownTarget = null;
        if (!same)
          return;

        if (!self.isDragging) {
          self.$titleShield.hide();
          self.$title.get(0).focus();
        }
      });
  }

  
  this.$expander = iQ("<img/>")
    .attr('src', 'chrome://browser/skin/tabcandy/stack-expander.png')
    .addClass("stackExpander")
    .appendTo($container)
    .hide();

  
  if (this.locked.bounds)
    $container.css({cursor: 'default'});

  if (this.locked.close)
    $close.hide();

  
  this._init($container.get(0));

  if (this.$debug)
    this.$debug.css({zIndex: -1000});

  
  Array.prototype.forEach.call(listOfEls, function(el) {
    self.add(el, null, options);
  });

  
  this._addHandlers($container);

  if (!this.locked.bounds)
    this.setResizable(true);

  Groups.register(this);

  
  var immediately = $container ? true : false;
  this.setBounds(rectToBe, immediately);
  this.snap();
  if ($container)
    this.setBounds(rectToBe, immediately);

  
  if (!options.dontPush)
    this.pushAway();

  this._inited = true;
  this.save();
  } catch(e){
    Utils.log("Error in Group(): " + e);
  }
};


window.Group.prototype = Utils.extend(new Item(), new Subscribable(), {
  
  
  
  defaultName: "name this group...",

  
  
  
  addOnClose: function(referenceObject, callback) {
    this.addSubscriber(referenceObject, "close", callback);
  },

  
  
  removeOnClose: function(referenceObject) {
    this.removeSubscriber(referenceObject, "close");
  },

  
  
  
  setActiveTab: function(tab){
    Utils.assert('tab must be a TabItem', tab && tab.isATabItem);
    this._activeTab = tab;
  },

  
  
  
  getActiveTab: function(){
    return this._activeTab;
  },

  
  
  
  getStorageData: function() {
    var data = {
      bounds: this.getBounds(),
      userSize: null,
      locked: Utils.copy(this.locked),
      title: this.getTitle(),
      id: this.id
    };

    if (Utils.isPoint(this.userSize))
      data.userSize = new Point(this.userSize);

    return data;
  },

  
  
  
  isEmpty: function() {
    return !this._children.length && !this.getTitle();
  },

  
  
  
  save: function() {
    if (!this._inited) 
      return;

    var data = this.getStorageData();
    if (Groups.groupStorageSanity(data))
      Storage.saveGroup(gWindow, data);
  },

  
  
  
  
  isNewTabsGroup: function() {
    return (this.locked.bounds && this.locked.title && this.locked.close);
  },

  
  
  
  getTitle: function() {
    var value = (this.$title ? this.$title.val() : '');
    return (value == this.defaultName ? '' : value);
  },

  
  
  
  setTitle: function(value) {
    this.$title.val(value);
    this.save();
  },

  
  
  
  adjustTitleSize: function() {
    Utils.assert('bounds needs to have been set', this.bounds);
    var w = Math.min(this.bounds.width - 35, Math.max(150, this.getTitle().length * 6));
    var css = {width: w};
    this.$title.css(css);
    this.$titleShield.css(css);
  },

  
  
  
  getContentBounds: function() {
    var box = this.getBounds();
    var titleHeight = this.$titlebar.height();
    box.top += titleHeight;
    box.height -= titleHeight;
    box.inset(6, 6);

    if (this.isNewTabsGroup())
      box.height -= 12; 
    else
      box.height -= 33; 

    return box;
  },

  
  
  
  
  
  
  
  
  
  
  
  setBounds: function(rect, immediately, options) {
    if (!Utils.isRect(rect)) {
      Utils.trace('Group.setBounds: rect is not a real rectangle!', rect);
      return;
    }

    if (!options)
      options = {};

    rect.width = Math.max( 110, rect.width );
    rect.height = Math.max( 125, rect.height);

    var titleHeight = this.$titlebar.height();

    
    var css = {};
    var titlebarCSS = {};
    var contentCSS = {};

    if (rect.left != this.bounds.left || options.force)
      css.left = rect.left;

    if (rect.top != this.bounds.top || options.force)
      css.top = rect.top;

    if (rect.width != this.bounds.width || options.force) {
      css.width = rect.width;
      titlebarCSS.width = rect.width;
      contentCSS.width = rect.width;
    }

    if (rect.height != this.bounds.height || options.force) {
      css.height = rect.height;
      contentCSS.height = rect.height - titleHeight;
    }

    if (Utils.isEmptyObject(css))
      return;

    var offset = new Point(rect.left - this.bounds.left, rect.top - this.bounds.top);
    this.bounds = new Rect(rect);

    
    if (css.width || css.height) {
      this.arrange({animate: !immediately}); 
    } else if (css.left || css.top) {
      this._children.forEach(function(child) {
        var box = child.getBounds();
        child.setPosition(box.left + offset.x, box.top + offset.y, immediately);
      });
    }

    
    if (immediately) {
      iQ(this.container).css(css);
      this.$titlebar.css(titlebarCSS);
    } else {
      TabMirror.pausePainting();
      iQ(this.container).animate(css, {
        duration: 350,
        easing: 'tabcandyBounce',
        complete: function() {
          TabMirror.resumePainting();
        }
      });

      this.$titlebar.animate(titlebarCSS, {
        duration: 350
      });
    }

    this.adjustTitleSize();

    this._updateDebugBounds();

    if (!this.isNewTabsGroup())
      this.setTrenches(rect);

    this.save();
  },

  
  
  
  setZ: function(value) {
    this.zIndex = value;

    iQ(this.container).css({zIndex: value});

    if (this.$debug)
      this.$debug.css({zIndex: value + 1});

    var count = this._children.length;
    if (count) {
      var topZIndex = value + count + 1;
      var zIndex = topZIndex;
      var self = this;
      this._children.forEach(function(child) {
        if (child == self.topChild)
          child.setZ(topZIndex + 1);
        else {
          child.setZ(zIndex);
          zIndex--;
        }
      });
    }
  },

  
  
  
  close: function() {
    this.removeAll();
    this._sendToSubscribers("close");
    Groups.unregister(this);
    this.removeTrenches();
    iQ(this.container).fadeOut(function() {
      iQ(this).remove();
      Items.unsquish();
    });

    Storage.deleteGroup(gWindow, this.id);
  },

  
  
  
  closeAll: function() {
    var self = this;
    if (this._children.length) {
      var toClose = Utils.merge([], this._children);
      toClose.forEach(function(child) {
        child.removeOnClose(self);
        child.close();
      });
    }

    if (!this.locked.close)
      this.close();
  },

  
  
  
  
  
  
  
  
  
  add: function(a, dropPos, options) {
    try {
      var item;
      var $el;
      if (a.isAnItem) {
        item = a;
        $el = iQ(a.container);
      } else {
        $el = iQ(a);
        item = Items.item($el);
      }

      Utils.assertThrow('shouldn\'t already be in another group', !item.parent || item.parent == this);

      item.removeTrenches();

      if (!dropPos)
        dropPos = {top:window.innerWidth, left:window.innerHeight};

      if (typeof(options) == 'undefined')
        options = {};

      var self = this;

      var wasAlreadyInThisGroup = false;
      var oldIndex = this._children.indexOf(item);
      if (oldIndex != -1) {
        this._children.splice(oldIndex, 1);
        wasAlreadyInThisGroup = true;
      }

      
      
      function findInsertionPoint(dropPos){
        if (self.shouldStack(self._children.length + 1))
          return 0;

        var best = {dist: Infinity, item: null};
        var index = 0;
        var box;
        self._children.forEach(function(child) {
          box = child.getBounds();
          if (box.bottom < dropPos.top || box.top > dropPos.top)
            return;

          var dist = Math.sqrt( Math.pow((box.top+box.height/2)-dropPos.top,2)
              + Math.pow((box.left+box.width/2)-dropPos.left,2) );

          if ( dist <= best.dist ){
            best.item = child;
            best.dist = dist;
            best.index = index;
          }
        });

        if ( self._children.length ){
          if (best.item) {
            box = best.item.getBounds();
            var insertLeft = dropPos.left <= box.left + box.width/2;
            if ( !insertLeft )
              return best.index+1;
            return best.index;
          }
          return self._children.length;
        }

        return 0;
      }

      
      var index = findInsertionPoint(dropPos);
      this._children.splice( index, 0, item );

      item.setZ(this.getZ() + 1);
      $el.addClass("tabInGroup");
      if ( this.isNewTabsGroup() ) $el.addClass("inNewTabGroup")

      if (!wasAlreadyInThisGroup) {
        item.droppable(false);
        item.groupData = {};

        item.addOnClose(this, function() {
          self.remove($el);
        });

        item.setParent(this);

        if (typeof(item.setResizable) == 'function')
          item.setResizable(false);

        if (item.tab == gBrowser.selectedTab)
          Groups.setActiveGroup(this);
      }

      if (!options.dontArrange)
        this.arrange();

      if ( this._nextNewTabCallback ){
        this._nextNewTabCallback.apply(this, [item])
        this._nextNewTabCallback = null;
      }
    } catch(e) {
      Utils.log('Group.add error', e);
    }
  },

  
  
  
  
  
  
  
  
  remove: function(a, options) {
    try {
      var $el;
      var item;

      if (a.isAnItem) {
        item = a;
        $el = iQ(item.container);
      } else {
        $el = iQ(a);
        item = Items.item($el);
      }

      if (typeof(options) == 'undefined')
        options = {};

      var index = this._children.indexOf(item);
      if (index != -1)
        this._children.splice(index, 1);

      item.setParent(null);
      item.removeClass("tabInGroup");
      item.removeClass("inNewTabGroup")
      item.removeClass("stacked");
      item.removeClass("stack-trayed");
      item.setRotation(0);
      item.setSize(item.defaultSize.x, item.defaultSize.y);

      item.droppable(true);
      item.removeOnClose(this);

      if (typeof(item.setResizable) == 'function')
        item.setResizable(true);

      if (!this._children.length && !this.locked.close && !this.getTitle() && !options.dontClose){
        this.close();
      } else if (!options.dontArrange) {
        this.arrange();
      }
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  removeAll: function() {
    var self = this;
    var toRemove = Utils.merge([], this._children);
    toRemove.forEach(function(child) {
      self.remove(child, {dontArrange: true});
    });
  },

  
  
  
  setNewTabButtonBounds: function(box, immediately) {
    if (!immediately)
      this.$ntb.animate(box.css(), {
        duration: 320,
        easing: 'tabcandyBounce'
      });
    else
      this.$ntb.css(box.css());
  },

  
  
  
  hideExpandControl: function(){
    this.$expander.hide();
  },

  
  
  
  showExpandControl: function(){
    var childBB = this.getChild(0).getBounds();
    var dT = childBB.top - this.getBounds().top;
    var dL = childBB.left - this.getBounds().left;

    this.$expander
        .show()
        .css({
          opacity: .2,
          top: dT + childBB.height + Math.min(7, (this.getBounds().bottom-childBB.bottom)/2),
          
          
          
          left: dL + childBB.width/2 - this.$expander.width()/2 - 6,
        });
  },

  
  
  
  shouldStack: function(count) {
    if (count <= 1)
      return false;

    var bb = this.getContentBounds();
    var options = {
      pretend: true,
      count: (this.isNewTabsGroup() ? count + 1 : count)
    };

    var rects = Items.arrange(null, bb, options);
    return (rects[0].width < TabItems.minTabWidth * 1.35 );
  },

  
  
  
  
  
  
  arrange: function(options) {
    if (this.expanded) {
      this.topChild = null;
      var box = new Rect(this.expanded.bounds);
      box.inset(8, 8);
      Items.arrange(this._children, box, Utils.extend({}, options, {padding: 8, z: 99999}));
    } else {
      var bb = this.getContentBounds();
      var count = this._children.length;
      if (!this.shouldStack(count)) {
        var animate;
        if (!options || typeof(options.animate) == 'undefined')
          animate = true;
        else
          animate = options.animate;

        if (typeof(options) == 'undefined')
          options = {};

        this._children.forEach(function(child){
            child.removeClass("stacked")
        });

        this.topChild = null;

        var arrangeOptions = Utils.copy(options);
        Utils.extend(arrangeOptions, {
          pretend: true,
          count: count
        });

        if (this.isNewTabsGroup()) {
          arrangeOptions.count++;
        } else if (!count) {
          this.xDensity = 0;
          this.yDensity = 0;
          return;
        }

        var rects = Items.arrange(this._children, bb, arrangeOptions);

        
        
        this.yDensity = (rects[rects.length - 1].bottom - bb.top) / (bb.height);

        
        

        
        
        var rightMostRight = 0;
        for each (var rect in rects) {
          if (rect.right > rightMostRight)
            rightMostRight = rect.right;
          else
            break;
        }
        this.xDensity = (rightMostRight - bb.left) / (bb.width);

        this._children.forEach(function(child, index) {
          if (!child.locked.bounds) {
            child.setBounds(rects[index], !animate);
            child.setRotation(0);
            if (options.z)
              child.setZ(options.z);
          }
        });

        if (this.isNewTabsGroup()) {
          var box = rects[rects.length - 1];
          box.left -= this.bounds.left;
          box.top -= this.bounds.top;
          this.setNewTabButtonBounds(box, !animate);
        }

        this._isStacked = false;
      } else
        this._stackArrange(bb, options);
    }

    if ( this._isStacked && !this.expanded) this.showExpandControl();
    else this.hideExpandControl();
  },

  
  
  
  
  
  
  
  
  
  
  _stackArrange: function(bb, options) {
    var animate;
    if (!options || typeof(options.animate) == 'undefined')
      animate = true;
    else
      animate = options.animate;

    if (typeof(options) == 'undefined')
      options = {};

    var count = this._children.length;
    if (!count)
      return;

    var zIndex = this.getZ() + count + 1;

    var Pi = Math.acos(-1);
    var maxRotation = 35; 
    var scale = 0.8;
    var newTabsPad = 10;
    var w;
    var h;
    var itemAspect = TabItems.tabHeight / TabItems.tabWidth;
    var bbAspect = bb.height / bb.width;

    
    
    if (bbAspect > itemAspect) { 
      w = bb.width * scale;
      h = w * itemAspect;
      
      this.xDensity = 1;
      this.yDensity = h / (bb.height * scale);
    } else { 
      h = bb.height * scale;
      w = h * (1 / itemAspect);
      this.yDensity = 1;
      this.xDensity = h / (bb.width * scale);
    }

    
    
    var x = (bb.width - w) / 2;
    if (this.isNewTabsGroup())
      x -= (w + newTabsPad) / 2;

    var y = Math.min(x, (bb.height - h) / 2);
    var box = new Rect(bb.left + x, bb.top + y, w, h);

    var self = this;
    var children = [];
    this._children.forEach(function(child) {
      if (child == self.topChild)
        children.unshift(child);
      else
        children.push(child);
    });

    children.forEach(function(child, index) {
      if (!child.locked.bounds) {
        child.setZ(zIndex);
        zIndex--;

        child.addClass("stacked");
        child.setBounds(box, !animate);
        child.setRotation(self._randRotate(maxRotation, index));
      }
    });

    if (this.isNewTabsGroup()) {
      box.left += box.width + newTabsPad;
      box.left -= this.bounds.left;
      box.top -= this.bounds.top;
      this.setNewTabButtonBounds(box, !animate);
    }

    self._isStacked = true;
  },

  
  
  
  _randRotate: function(spread, index){
    if ( index >= this._stackAngles.length ){
      var randAngle = 5*index + parseInt( (Math.random()-.5)*1 );
      this._stackAngles.push(randAngle);
      return randAngle;
    }

    if ( index > 5 ) index = 5;

    return this._stackAngles[index];
  },

  
  
  
  
  
  
  
  childHit: function(child) {
    var self = this;

    
    if (!this._isStacked || this.expanded) {
      return {
        shouldZoom: true,
        callback: function() {
          self.collapse();
        }
      };
    }

    
    




    Groups.setActiveGroup(self);
    return { shouldZoom: true };

    

  },

  expand: function(){
    var self = this;
    
    Groups.setActiveGroup(self);
    var startBounds = this.getChild(0).getBounds();
    var $tray = iQ("<div>").css({
      top: startBounds.top,
      left: startBounds.left,
      width: startBounds.width,
      height: startBounds.height,
      position: "absolute",
      zIndex: 99998
    }).appendTo("body");


    var w = 180;
    var h = w * (TabItems.tabHeight / TabItems.tabWidth) * 1.1;
    var padding = 20;
    var col = Math.ceil(Math.sqrt(this._children.length));
    var row = Math.ceil(this._children.length/col);

    var overlayWidth = Math.min(window.innerWidth - (padding * 2), w*col + padding*(col+1));
    var overlayHeight = Math.min(window.innerHeight - (padding * 2), h*row + padding*(row+1));

    var pos = {left: startBounds.left, top: startBounds.top};
    pos.left -= overlayWidth/3;
    pos.top  -= overlayHeight/3;

    if ( pos.top < 0 )  pos.top = 20;
    if ( pos.left < 0 ) pos.left = 20;
    if ( pos.top+overlayHeight > window.innerHeight ) pos.top = window.innerHeight-overlayHeight-20;
    if ( pos.left+overlayWidth > window.innerWidth )  pos.left = window.innerWidth-overlayWidth-20;

    $tray
      .animate({
        width:  overlayWidth,
        height: overlayHeight,
        top: pos.top,
        left: pos.left
      }, {
        duration: 200,
        easing: 'tabcandyBounce'
      })
      .addClass("overlay");

    this._children.forEach(function(child){
      child.addClass("stack-trayed");
    });

    var $shield = iQ('<div>')
      .css({
        left: 0,
        top: 0,
        width: window.innerWidth,
        height: window.innerHeight,
        position: 'absolute',
        zIndex: 99997
      })
      .appendTo('body')
      .click(function() { 
        self.collapse();
      });

    
    
    
    
    
    setTimeout(function(){
      $shield.mouseover(function() {
        self.collapse();
      });
    }, 200);

    this.expanded = {
      $tray: $tray,
      $shield: $shield,
      bounds: new Rect(pos.left, pos.top, overlayWidth, overlayHeight)
    };

    this.arrange();
  },

  
  
  
  collapse: function() {
    if (this.expanded) {
      var z = this.getZ();
      var box = this.getBounds();
      this.expanded.$tray
        .css({
          zIndex: z + 1
        })
        .animate({
          width:  box.width,
          height: box.height,
          top: box.top,
          left: box.left,
          opacity: 0
        }, {
          duration: 350,
          easing: 'tabcandyBounce',
          complete: function() {
            iQ(this).remove();
          }
        });

      this.expanded.$shield.remove();
      this.expanded = null;

      this._children.forEach(function(child){
        child.removeClass("stack-trayed");
      });

      this.arrange({z: z + 2});
    }
  },

  
  
  
  _addHandlers: function(container) {
    var self = this;

    this.dropOptions.over = function(){
      if ( !this.isNewTabsGroup() )
        iQ(this.container).addClass("acceptsDrop");
    };
    this.dropOptions.drop = function(event){
      iQ(this.container).removeClass("acceptsDrop");
      this.add( drag.info.$el, {left:event.pageX, top:event.pageY} );
    };

    if (!this.locked.bounds)
      this.draggable();

    iQ(container)
      .mousedown(function(e){
        self._mouseDown = {
          location: new Point(e.clientX, e.clientY),
          className: e.target.className
        };
      })
      .mouseup(function(e){
        if (!self._mouseDown || !self._mouseDown.location || !self._mouseDown.className)
          return;

        
        var className = self._mouseDown.className;
        if (className.indexOf('title-shield') != -1
            || className.indexOf('name') != -1
            || className.indexOf('close') != -1
            || className.indexOf('newTabButton') != -1
            || className.indexOf('stackExpander') != -1 ) {
          return;
        }

        var location = new Point(e.clientX, e.clientY);

        if (location.distance(self._mouseDown.location) > 1.0)
          return;

        
        if ( self.isNewTabsGroup() )
          return;

        
        
        var activeTab = self.getActiveTab();
        if( !self._isStacked ){
          if ( activeTab )
            activeTab.zoomIn();
          else if (self.getChild(0))
            self.getChild(0).zoomIn();
        }

        self._mouseDown = null;
    });

    this.droppable(true);

    this.$expander.click(function(){
      self.expand();
    });
  },

  
  
  
  setResizable: function(value){
    this.resizeOptions.minWidth = 90;
    this.resizeOptions.minHeight = 90;

    if (value) {
      this.$resizer.fadeIn();
      this.resizable(true);
    } else {
      this.$resizer.fadeOut();
      this.resizable(false);
    }
  },

  
  
  
  newTab: function(url) {
    Groups.setActiveGroup(this);
    let newTab = gBrowser.loadOneTab(url || "about:blank", {inBackground: true});

    
    
    
    
    
    Utils.timeout(function(){
      UI.showTabCandy()
    }, 1);

    var self = this;
    var doNextTab = function(tab){
      var group = Groups.getActiveGroup();

      iQ(tab.container).css({opacity: 0});
      var $anim = iQ("<div>")
        .addClass('newTabAnimatee')
        .css({
          top: tab.bounds.top+5,
          left: tab.bounds.left+5,
          width: tab.bounds.width-10,
          height: tab.bounds.height-10,
          zIndex: 999,
          opacity: 0
        })
        .appendTo("body")
        .animate({
          opacity: 1.0
        }, {
          duration: 500,
          complete: function() {
            $anim.animate({
              top: 0,
              left: 0,
              width: window.innerWidth,
              height: window.innerHeight
            }, {
              duration: 270,
              complete: function(){
                iQ(tab.container).css({opacity: 1});
                gBrowser.selectedTab = newTab;
                UI.hideTabCandy()
                gWindow.gURLBar.focus();
                $anim.remove();
                
                
                
                
                
                Utils.timeout(function(){
                  UI.showOnlyTheseTabs(Groups.getActiveGroup()._children);
                }, 400);
              }
            });
          }
        });
    }

    
    
    
    

    self.onNextNewTab(doNextTab);
  },

  
  
  
  
  
  
  reorderBasedOnTabOrder: function(){
    this._children.sort(function(a,b) a.tab._tPos - b.tab._tPos);

    this.arrange({animate: false});
    
  },

  
  
  
  setTopChild: function(topChild){
    this.topChild = topChild;

    this.arrange({animate: false});
    
  },

  
  
  
  
  
  
  
  getChild: function(index){
    if ( index < 0 )
      index = this._children.length + index;
    if ( index >= this._children.length || index < 0 )
      return null;
    return this._children[index];
  },

  
  
  
  getChildren: function(){
    return this._children;
  },

  
  
  
  
  
  
  
  
  
  onNextNewTab: function(callback){
    this._nextNewTabCallback = callback;
  }
});




window.Groups = {

  
  
  
  init: function() {
    this.groups = [];
    this.nextID = 1;
    this._inited = false;
  },

  
  
  
  getNextID: function() {
    var result = this.nextID;
    this.nextID++;
    this.save();
    return result;
  },

  
  
  
  getStorageData: function() {
    var data = {nextID: this.nextID, groups: []};
    this.groups.forEach(function(group) {
      data.groups.push(group.getStorageData());
    });

    return data;
  },

  
  
  
  saveAll: function() {
    this.save();
    this.groups.forEach(function(group) {
      group.save();
    });
  },

  
  
  
  save: function() {
    if (!this._inited) 
      return;

    Storage.saveGroupsData(gWindow, {nextID:this.nextID});
  },

  
  
  
  getBoundingBox: function Groups_getBoundingBox(els) {
    var el, b;
    var bounds = [iQ(el).bounds() for each (el in els)];
    var left   = min( [ b.left   for each (b in bounds) ] );
    var top    = min( [ b.top    for each (b in bounds) ] );
    var right  = max( [ b.right  for each (b in bounds) ] );
    var bottom = max( [ b.bottom for each (b in bounds) ] );

    return new Rect(left, top, right-left, bottom-top);
  },

  
  
  
  
  reconstitute: function(groupsData, groupData) {
    try {
      if (groupsData && groupsData.nextID)
        this.nextID = groupsData.nextID;

      if (groupData) {
        for (var id in groupData) {
          var group = groupData[id];
          if (this.groupStorageSanity(group)) {
            var isNewTabsGroup = (group.title == 'New Tabs');
            var options = {
              locked: {
                close: isNewTabsGroup,
                title: isNewTabsGroup,
                bounds: isNewTabsGroup
              },
              dontPush: true
            };

            new Group([], Utils.extend({}, group, options));
          }
        }
      }

      var group = this.getNewTabGroup();
      if (!group) {
        var box = this.getBoundsForNewTabGroup();
        var options = {
          locked: {
            close: true,
            title: true,
            bounds: true
          },
          dontPush: true,
          bounds: box,
          title: 'New Tabs'
        };

        new Group([], options);
      }

      this.repositionNewTabGroup();

      this._inited = true;
      this.save(); 
    }catch(e){
      Utils.log("error in recons: "+e);
    }
  },

  
  
  
  groupStorageSanity: function(groupData) {
    
    var sane = true;
    if (!Utils.isRect(groupData.bounds)) {
      Utils.log('Groups.groupStorageSanity: bad bounds', groupData.bounds);
      sane = false;
    }

    return sane;
  },

  
  
  
  
  
  getGroupWithTitle: function(title) {
    var result = null;
    this.groups.forEach(function(group) {
      if (group.getTitle() == title) {
        result = group;
        return false;
      }
    });

    return result;
  },

  
  
  
  getNewTabGroup: function() {
    var groupTitle = 'New Tabs';
    var array = this.groups.filter(function(group) {
      return group.getTitle() == groupTitle;
    });

    if (array.length)
      return array[0];

    return null;
  },

  
  
  
  getBoundsForNewTabGroup: function() {
    var pad = 0;
    var sw = window.innerWidth;
    var sh = window.innerHeight;
    var w = sw - (pad * 2);
    var h = TabItems.tabHeight * 0.9 + pad*2;
    return new Rect(pad, sh - (h + pad), w, h);
  },

  
  
  
  repositionNewTabGroup: function() {
    var box = this.getBoundsForNewTabGroup();
    var group = this.getNewTabGroup();
    group.setBounds(box, true);
  },

  
  
  
  register: function(group) {
    Utils.assert('group', group);
    Utils.assert('only register once per group', this.groups.indexOf(group) == -1);
    this.groups.push(group);
  },

  
  
  
  unregister: function(group) {
    var index = this.groups.indexOf(group);
    if (index != -1)
      this.groups.splice(index, 1);

    if (group == this._activeGroup)
      this._activeGroup = null;
  },

  
  
  
  
  group: function(a) {
    var result = null;
    this.groups.forEach(function(candidate) {
      if (candidate.id == a) {
        result = candidate;
        return false;
      }
    });

    return result;
  },

  
  
  
  arrange: function() {
    var bounds = Items.getPageBounds();
    var count = this.groups.length - 1;
    var columns = Math.ceil(Math.sqrt(count));
    var rows = ((columns * columns) - count >= columns ? columns - 1 : columns);
    var padding = 12;
    var startX = bounds.left + padding;
    var startY = bounds.top + padding;
    var totalWidth = bounds.width - padding;
    var totalHeight = bounds.height - padding;
    var box = new Rect(startX, startY,
        (totalWidth / columns) - padding,
        (totalHeight / rows) - padding);

    var i = 0;
    this.groups.forEach(function(group) {
      if (group.locked.bounds)
        return;

      group.setBounds(box, true);

      box.left += box.width + padding;
      i++;
      if (i % columns == 0) {
        box.left = startX;
        box.top += box.height + padding;
      }
    });
  },

  
  
  
  removeAll: function() {
    var toRemove = Utils.merge([], this.groups);
    toRemove.forEach(function(group) {
      group.removeAll();
    });
  },

  
  
  
  newTab: function(tabItem) {
    var group = this.getActiveGroup();
    if ( group == null )
      group = this.getNewTabGroup();

    var $el = iQ(tabItem.container);
    if (group) group.add($el);
  },

  
  
  
  
  
  getActiveGroup: function() {
    return this._activeGroup;
  },

  
  
  
  
  
  
  
  
  
  setActiveGroup: function(group) {
    this._activeGroup = group;
    this.updateTabBarForActiveGroup();
  },

  
  
  
  updateTabBarForActiveGroup: function() {
    if (!window.UI)
      return; 

    if (this._activeGroup)
      UI.showOnlyTheseTabs( this._activeGroup._children );
    else if ( this._activeGroup == null)
      UI.showOnlyTheseTabs( this.getOrphanedTabs(), {dontReorg: true});
  },

  
  
  
  getOrphanedTabs: function(){
    var tabs = TabItems.getItems();
    tabs = tabs.filter(function(tab){
      return tab.parent == null;
    });
    return tabs;
  },

  
  
  
  
  
  getNextGroupTab: function(reverse){
    var groups = Groups.groups.map(function(group) group);
    var activeGroup = Groups.getActiveGroup();
    var tabItem = null;

    if (!activeGroup) {
      if (groups.length > 0) {
        if (reverse)
          groups = groups.reverse();

        groups.some(function(group) {
          var child = group.getChild(0);
          if (child) {
            tabItem = child;
            return true;
          }
        });
      }
    } else {
      if (reverse)
        groups = groups.reverse();

      var currentIndex;
      groups.some(function(group, index) {
        if (group == activeGroup) {
          currentIndex = index;
          return true;
        }
      });
      var firstGroups = groups.slice(currentIndex + 1);
      firstGroups.some(function(group) {
        var child = group.getChild(0);
        if (child) {
          tabItem = child;
          return true;
        }
      });
      if (!tabItem) {
        var orphanedTabs = Groups.getOrphanedTabs();
        if (orphanedTabs.length > 0)
          tabItem = orphanedTabs[0];
      }
      if (!tabItem) {
        var secondGroups = groups.slice(0, currentIndex);
        secondGroups.some(function(group) {
          var child = group.getChild(0);
          if (child) {
            tabItem = child;
            return true;
          }
        });
      }
    }
    return tabItem;
  }
};


Groups.init();

})();
