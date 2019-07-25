































































window.GroupItem = function GroupItem(listOfEls, options) {
  try {
  if (typeof(options) == 'undefined')
    options = {};

  this._inited = false;
  this._children = []; 
  this.defaultSize = new Point(TabItems.tabWidth * 1.5, TabItems.tabHeight * 1.5);
  this.isAGroupItem = true;
  this.id = options.id || GroupItems.getNextID();
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
    Utils.assert(Utils.isRect(options.bounds), "options.bounds must be a Rect");
    rectToBe = new Rect(options.bounds);
  }

  if (!rectToBe) {
    rectToBe = GroupItems.getBoundingBox(listOfEls);
    rectToBe.inset(-30, -30);
  }

  var $container = options.container;
  if (!$container) {
    $container = iQ('<div>')
      .addClass('groupItem')
      .css({position: 'absolute'})
      .css(rectToBe);
  }

  this.bounds = $container.bounds();

  this.isDragging = false;
  $container
    .css({zIndex: -100})
    .appendTo("body");


  
  this.$ntb = iQ("<div>")
    .appendTo($container);

  this.$ntb
    .addClass('newTabButton')
    .click(function() {
      self.newTab();
    });

  (this.$ntb)[0].title = 'New tab';

  
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
          easing: "tabviewBounce"
        });
    }
  };

  var handleKeyPress = function(e) {
    if (e.which == 13 || e.which == 27) { 
      (self.$title)[0].blur();
      self.$title
        .addClass("transparentBorder")
        .one("mouseout", function() {
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
        (self.$title)[0].blur();
        return;
      }
      (self.$title)[0].select();
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
          (self.$title)[0].focus();
        }
      });
  }

  
  this.$expander = iQ("<img/>")
    .attr("src", "chrome://browser/skin/tabview/stack-expander.png")
    .addClass("stackExpander")
    .appendTo($container)
    .hide();

  
  if (this.locked.bounds)
    $container.css({cursor: 'default'});

  if (this.locked.close)
    $close.hide();

  
  this._init($container[0]);

  if (this.$debug)
    this.$debug.css({zIndex: -1000});

  
  Array.prototype.forEach.call(listOfEls, function(el) {
    self.add(el, null, options);
  });

  
  this._addHandlers($container);

  if (!this.locked.bounds)
    this.setResizable(true);

  GroupItems.register(this);

  
  var immediately = $container ? true : false;
  this.setBounds(rectToBe, immediately);
  this.snap();
  if ($container)
    this.setBounds(rectToBe, immediately);

  
  if (!options.dontPush)
    this.pushAway();

  this._inited = true;
  this.save();
  } catch(e) {
    Utils.log("Error in GroupItem()");
    Utils.log(e.stack);
  }
};


window.GroupItem.prototype = Utils.extend(new Item(), new Subscribable(), {
  
  
  
  defaultName: "name this groupItem...",

  
  
  
  setActiveTab: function(tab) {
    Utils.assert(tab && tab.isATabItem, 'tab must be a TabItem');
    this._activeTab = tab;
  },

  
  
  
  getActiveTab: function() {
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
    if (GroupItems.groupItemStorageSanity(data))
      Storage.saveGroupItem(gWindow, data);
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
    Utils.assert(this.bounds, 'bounds needs to have been set');
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

    box.height -= 33; 

    return box;
  },

  
  
  
  
  
  
  
  
  
  
  
  setBounds: function(rect, immediately, options) {
    if (!Utils.isRect(rect)) {
      Utils.trace('GroupItem.setBounds: rect is not a real rectangle!', rect);
      return;
    }

    if (!options)
      options = {};

    rect.width = Math.max(110, rect.width);
    rect.height = Math.max(125, rect.height);

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
      TabItems.pausePainting();
      iQ(this.container).animate(css, {
        duration: 350,
        easing: "tabviewBounce",
        complete: function() {
          TabItems.resumePainting();
        }
      });

      this.$titlebar.animate(titlebarCSS, {
        duration: 350
      });
    }

    this.adjustTitleSize();

    this._updateDebugBounds();
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
    GroupItems.unregister(this);
    this.removeTrenches();
    iQ(this.container).fadeOut(function() {
      iQ(this).remove();
      Items.unsquish();
    });

    Storage.deleteGroupItem(gWindow, this.id);
  },

  
  
  
  closeAll: function() {
    var self = this;
    if (this._children.length) {
      var toClose = this._children.concat();
      toClose.forEach(function(child) {
        child.removeSubscriber(self, "close");
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
      Utils.assertThrow(!item.parent || item.parent == this, 
          "shouldn't already be in another groupItem");

      item.removeTrenches();

      if (!dropPos)
        dropPos = {top:window.innerWidth, left:window.innerHeight};

      if (typeof(options) == 'undefined')
        options = {};

      var self = this;

      var wasAlreadyInThisGroupItem = false;
      var oldIndex = this._children.indexOf(item);
      if (oldIndex != -1) {
        this._children.splice(oldIndex, 1);
        wasAlreadyInThisGroupItem = true;
      }

      
      
      function findInsertionPoint(dropPos) {
        if (self.shouldStack(self._children.length + 1))
          return 0;

        var best = {dist: Infinity, item: null};
        var index = 0;
        var box;
        self._children.forEach(function(child) {
          box = child.getBounds();
          if (box.bottom < dropPos.top || box.top > dropPos.top)
            return;

          var dist = Math.sqrt(Math.pow((box.top+box.height/2)-dropPos.top,2)
              + Math.pow((box.left+box.width/2)-dropPos.left,2));

          if (dist <= best.dist) {
            best.item = child;
            best.dist = dist;
            best.index = index;
          }
        });

        if (self._children.length) {
          if (best.item) {
            box = best.item.getBounds();
            var insertLeft = dropPos.left <= box.left + box.width/2;
            if (!insertLeft)
              return best.index+1;
            return best.index;
          }
          return self._children.length;
        }

        return 0;
      }

      
      var index = findInsertionPoint(dropPos);
      this._children.splice(index, 0, item);

      item.setZ(this.getZ() + 1);
      $el.addClass("tabInGroupItem");

      if (!wasAlreadyInThisGroupItem) {
        item.droppable(false);
        item.groupItemData = {};

        item.addSubscriber(this, "close", function() {
          self.remove($el);
        });

        item.setParent(this);

        if (typeof(item.setResizable) == 'function')
          item.setResizable(false);

        if (item.tab == gBrowser.selectedTab)
          GroupItems.setActiveGroupItem(this);
      }

      if (!options.dontArrange) {
        this.arrange();
      }
      UI.setReorderTabsOnHide(this);

      if (this._nextNewTabCallback) {
        this._nextNewTabCallback.apply(this, [item])
        this._nextNewTabCallback = null;
      }
    } catch(e) {
      Utils.log('GroupItem.add error', e);
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
      item.removeClass("tabInGroupItem");
      item.removeClass("stacked");
      item.removeClass("stack-trayed");
      item.setRotation(0);

      item.droppable(true);
      item.removeSubscriber(this, "close");

      if (typeof(item.setResizable) == 'function')
        item.setResizable(true);

      if (!this._children.length && !this.locked.close && !this.getTitle() && !options.dontClose) {
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
    var toRemove = this._children.concat();
    toRemove.forEach(function(child) {
      self.remove(child, {dontArrange: true});
    });
  },

  
  
  
  setNewTabButtonBounds: function(box, immediately) {
    if (!immediately)
      this.$ntb.animate(box.css(), {
        duration: 320,
        easing: "tabviewBounce"
      });
    else
      this.$ntb.css(box.css());
  },

  
  
  
  hideExpandControl: function() {
    this.$expander.hide();
  },

  
  
  
  showExpandControl: function() {
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
      count: count
    };

    var rects = Items.arrange(null, bb, options);
    return (rects[0].width < TabItems.minTabWidth * 1.35);
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

        this._children.forEach(function(child) {
            child.removeClass("stacked")
        });

        this.topChild = null;

        var arrangeOptions = Utils.copy(options);
        Utils.extend(arrangeOptions, {
          pretend: true,
          count: count
        });

        if (!count) {
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

        this._isStacked = false;
      } else
        this._stackArrange(bb, options);
    }

    if (this._isStacked && !this.expanded) this.showExpandControl();
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

    self._isStacked = true;
  },

  
  
  
  _randRotate: function(spread, index) {
    if (index >= this._stackAngles.length) {
      var randAngle = 5*index + parseInt((Math.random()-.5)*1);
      this._stackAngles.push(randAngle);
      return randAngle;
    }

    if (index > 5) index = 5;

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

    
    




    GroupItems.setActiveGroupItem(self);
    return { shouldZoom: true };

    

  },

  expand: function() {
    var self = this;
    
    GroupItems.setActiveGroupItem(self);
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

    if (pos.top < 0)  pos.top = 20;
    if (pos.left < 0) pos.left = 20;
    if (pos.top+overlayHeight > window.innerHeight) pos.top = window.innerHeight-overlayHeight-20;
    if (pos.left+overlayWidth > window.innerWidth)  pos.left = window.innerWidth-overlayWidth-20;

    $tray
      .animate({
        width:  overlayWidth,
        height: overlayHeight,
        top: pos.top,
        left: pos.left
      }, {
        duration: 200,
        easing: "tabviewBounce"
      })
      .addClass("overlay");

    this._children.forEach(function(child) {
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

    
    
    
    
    
    setTimeout(function() {
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
          easing: "tabviewBounce",
          complete: function() {
            iQ(this).remove();
          }
        });

      this.expanded.$shield.remove();
      this.expanded = null;

      this._children.forEach(function(child) {
        child.removeClass("stack-trayed");
      });

      this.arrange({z: z + 2});
    }
  },

  
  
  
  _addHandlers: function(container) {
    var self = this;

    this.dropOptions.over = function() {
      iQ(this.container).addClass("acceptsDrop");
    };
    this.dropOptions.drop = function(event) {
      iQ(this.container).removeClass("acceptsDrop");
      this.add(drag.info.$el, {left:event.pageX, top:event.pageY});
      GroupItems.setActiveGroupItem(this);
    };

    if (!this.locked.bounds)
      this.draggable();

    iQ(container)
      .mousedown(function(e) {
        self._mouseDown = {
          location: new Point(e.clientX, e.clientY),
          className: e.target.className
        };
      })
      .mouseup(function(e) {
        if (!self._mouseDown || !self._mouseDown.location || !self._mouseDown.className)
          return;

        
        var className = self._mouseDown.className;
        if (className.indexOf('title-shield') != -1
            || className.indexOf('name') != -1
            || className.indexOf('close') != -1
            || className.indexOf('newTabButton') != -1
            || className.indexOf('stackExpander') != -1) {
          return;
        }

        var location = new Point(e.clientX, e.clientY);

        if (location.distance(self._mouseDown.location) > 1.0)
          return;

        
        
        var activeTab = self.getActiveTab();
        if (!self._isStacked) {
          if (activeTab)
            activeTab.zoomIn();
          else if (self.getChild(0))
            self.getChild(0).zoomIn();
        }

        self._mouseDown = null;
    });

    this.droppable(true);

    this.$expander.click(function() {
      self.expand();
    });
  },

  
  
  
  setResizable: function(value) {
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
    GroupItems.setActiveGroupItem(this);
    let newTab = gBrowser.loadOneTab(url || "about:blank", {inBackground: true});

    var self = this;
    var doNextTab = function(tab) {
      var groupItem = GroupItems.getActiveGroupItem();

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
              complete: function() {
                iQ(tab.container).css({opacity: 1});
                newTab.tabItem.zoomIn(!url);
                $anim.remove();
                
                
                
                
                
                setTimeout(function() {
                  self._sendToSubscribers("tabAdded", { groupItemId: self.id });
                }, 1);
              }
            });
          }
        });
    }

    
    
    
    
    self.onNextNewTab(doNextTab);
  },

  
  
  
  
  
  
  reorderTabItemsBasedOnTabOrder: function() {
    this._children.sort(function(a,b) a.tab._tPos - b.tab._tPos);

    this.arrange({animate: false});
    
  },

  
  
  
  reorderTabsBasedOnTabItemOrder: function() {
    var tabBarTabs = Array.slice(gBrowser.tabs);
    var currentIndex;

    
    this._children.forEach(function(tabItem) {
      tabBarTabs.some(function(tab, i) {
        if (tabItem.tab == tab) {
          if (!currentIndex)
            currentIndex = i;
          else if (tab.pinned)
            currentIndex++;
          else {
            var removed;
            if (currentIndex < i)
              currentIndex = i;
            else if (currentIndex > i) {
              removed = tabBarTabs.splice(i, 1);
              tabBarTabs.splice(currentIndex, 0, removed);
              gBrowser.moveTabTo(tabItem.tab, currentIndex);
            }
          }
          return true;
        }
      });
    });
  },

  
  
  
  setTopChild: function(topChild) {
    this.topChild = topChild;

    this.arrange({animate: false});
    
  },

  
  
  
  
  
  
  
  getChild: function(index) {
    if (index < 0)
      index = this._children.length + index;
    if (index >= this._children.length || index < 0)
      return null;
    return this._children[index];
  },

  
  
  
  getChildren: function() {
    return this._children;
  },

  
  
  
  
  
  
  
  
  
  onNextNewTab: function(callback) {
    this._nextNewTabCallback = callback;
  }
});




window.GroupItems = {
  groupItems: [],
  nextID: 1,
  _inited: false,
  _activeGroupItem: null,
  _activeOrphanTab: null,

  
  
  init: function() {




  },

  
  
  
  getNextID: function() {
    var result = this.nextID;
    this.nextID++;
    this.save();
    return result;
  },

  
  
  
  getStorageData: function() {
    var data = {nextID: this.nextID, groupItems: []};
    this.groupItems.forEach(function(groupItem) {
      data.groupItems.push(groupItem.getStorageData());
    });

    return data;
  },

  
  
  
  saveAll: function() {
    this.save();
    this.groupItems.forEach(function(groupItem) {
      groupItem.save();
    });
  },

  
  
  
  save: function() {
    if (!this._inited) 
      return;

    Storage.saveGroupItemsData(gWindow, {nextID:this.nextID});
  },

  
  
  
  getBoundingBox: function GroupItems_getBoundingBox(els) {
    var el, b;
    var bounds = [iQ(el).bounds() for each (el in els)];
    var left   = Math.min.apply({},[ b.left   for each (b in bounds) ]);
    var top    = Math.min.apply({},[ b.top    for each (b in bounds) ]);
    var right  = Math.max.apply({},[ b.right  for each (b in bounds) ]);
    var bottom = Math.max.apply({},[ b.bottom for each (b in bounds) ]);

    return new Rect(left, top, right-left, bottom-top);
  },

  
  
  
  
  reconstitute: function(groupItemsData, groupItemData) {
    try {
      if (groupItemsData && groupItemsData.nextID)
        this.nextID = groupItemsData.nextID;

      if (groupItemData) {
        for (var id in groupItemData) {
          var groupItem = groupItemData[id];
          if (this.groupItemStorageSanity(groupItem)) {
            var options = {
              dontPush: true
            };

            new GroupItem([], Utils.extend({}, groupItem, options));
          }
        }
      }

      this._inited = true;
      this.save(); 
    } catch(e) {
      Utils.log("error in recons: "+e);
    }
  },

  
  
  
  groupItemStorageSanity: function(groupItemData) {
    
    var sane = true;
    if (!Utils.isRect(groupItemData.bounds)) {
      Utils.log('GroupItems.groupItemStorageSanity: bad bounds', groupItemData.bounds);
      sane = false;
    }

    return sane;
  },

  
  
  
  
  
  getGroupItemWithTitle: function(title) {
    var result = null;
    this.groupItems.forEach(function(groupItem) {
      if (groupItem.getTitle() == title) {
        result = groupItem;
        return false;
      }
    });

    return result;
  },

  
  
  
  register: function(groupItem) {
    Utils.assert(groupItem, 'groupItem');
    Utils.assert(this.groupItems.indexOf(groupItem) == -1, 'only register once per groupItem');
    this.groupItems.push(groupItem);
  },

  
  
  
  unregister: function(groupItem) {
    var index = this.groupItems.indexOf(groupItem);
    if (index != -1)
      this.groupItems.splice(index, 1);

    if (groupItem == this._activeGroupItem)
      this._activeGroupItem = null;

  },

  
  
  
  
  groupItem: function(a) {
    var result = null;
    this.groupItems.forEach(function(candidate) {
      if (candidate.id == a) {
        result = candidate;
        return false;
      }
    });

    return result;
  },

  
  
  
  arrange: function() {
    var bounds = Items.getPageBounds();
    bounds.bottom -= 20; 

    var count = this.groupItems.length - 1;
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
    this.groupItems.forEach(function(groupItem) {
      if (groupItem.locked.bounds)
        return;

      groupItem.setBounds(box, true);

      box.left += box.width + padding;
      i++;
      if (i % columns == 0) {
        box.left = startX;
        box.top += box.height + padding;
      }
    });
  },

  
  
  
  removeAll: function() {
    var toRemove = this.groupItems.concat();
    toRemove.forEach(function(groupItem) {
      groupItem.removeAll();
    });
  },

  
  
  
  newTab: function(tabItem) {
    let activeGroupItem = this.getActiveGroupItem();
    let orphanTab = this.getActiveOrphanTab();

    if (activeGroupItem) {
      activeGroupItem.add(tabItem);
    } else if (orphanTab) {
      let newGroupItemBounds = orphanTab.getBoundsWithTitle();
      newGroupItemBounds.inset(-40,-40);
      let newGroupItem = new GroupItem([orphanTab, tabItem], {bounds: newGroupItemBounds});
      newGroupItem.snap();
      this.setActiveGroupItem(newGroupItem);
    } else {
      this.positionNewTabAtBottom(tabItem);
    }
  },

  
  
  
  
  
  positionNewTabAtBottom: function(tabItem) {
    let windowBounds = Items.getSafeWindowBounds();

    let itemBounds = new Rect(
      windowBounds.right - TabItems.tabWidth,
      windowBounds.bottom - TabItems.tabHeight,
      TabItems.tabWidth,
      TabItems.tabHeight
    );

    tabItem.setBounds(itemBounds);
  },

  
  
  
  
  getActiveGroupItem: function() {
    return this._activeGroupItem;
  },

  
  
  
  
  
  
  
  
  setActiveGroupItem: function(groupItem) {

    if (this._activeGroupItem)
      iQ(this._activeGroupItem.container).removeClass('activeGroupItem');

    if (groupItem !== null) {
      if (groupItem)
        iQ(groupItem.container).addClass('activeGroupItem');
      
      this.setActiveOrphanTab(null);
    }

    this._activeGroupItem = groupItem;
  },

  
  
  
  getActiveOrphanTab: function() {
    return this._activeOrphanTab;
  },

  
  
  
  
  
  
  
  setActiveOrphanTab: function(tabItem) {
    this._activeOrphanTab = tabItem;
  },

  
  
  
  
  updateTabBar: function() {
    if (!window.UI)
      return; 



    if (!this._activeGroupItem && !this._activeOrphanTab) {
      Utils.assert(false, "There must be something to show in the tab bar!");
      return;
    }

    let tabItems = this._activeGroupItem == null ?
      [this._activeOrphanTab] : this._activeGroupItem._children;
    gBrowser.showOnlyTheseTabs(tabItems.map(function(item) item.tab));
  },

  
  
  
  getOrphanedTabs: function() {
    var tabs = TabItems.getItems();
    tabs = tabs.filter(function(tab) {
      return tab.parent == null;
    });
    return tabs;
  },

  
  
  
  
  
  getNextGroupItemTab: function(reverse) {
    var groupItems = Utils.copy(GroupItems.groupItems);
    if (reverse)
      groupItems = groupItems.reverse();
    var activeGroupItem = GroupItems.getActiveGroupItem();
    var activeOrphanTab = GroupItems.getActiveOrphanTab();
    var tabItem = null;

    if (!activeGroupItem) {
      if (groupItems.length > 0) {

        groupItems.some(function(groupItem) {
          var child = groupItem.getChild(0);
          if (child) {
            tabItem = child;
            return true;
          }
        });
      }
    } else {
      if (reverse)
        groupItems = groupItems.reverse();

      var currentIndex;
      groupItems.some(function(groupItem, index) {
        if (groupItem == activeGroupItem) {
          currentIndex = index;
          return true;
        }
      });
      var firstGroupItems = groupItems.slice(currentIndex + 1);
      firstGroupItems.some(function(groupItem) {
        var child = groupItem.getChild(0);
        if (child) {
          tabItem = child;
          return true;
        }
      });
      if (!tabItem) {
        var orphanedTabs = GroupItems.getOrphanedTabs();
        if (orphanedTabs.length > 0)
          tabItem = orphanedTabs[0];
      }
      if (!tabItem) {
        var secondGroupItems = groupItems.slice(0, currentIndex);
        secondGroupItems.some(function(groupItem) {
          var child = groupItem.getChild(0);
          if (child) {
            tabItem = child;
            return true;
          }
        });
      }
    }
    return tabItem;
  },

  
  
  
  
  
  moveTabToGroupItem : function(tab, groupItemId) {
    let shouldUpdateTabBar = false;
    let shouldShowTabView = false;
    let groupItem;

    
    if (gBrowser.selectedTab == tab) {
      let list = gBrowser.visibleTabs;
      let listLength = list.length;

      if (listLength > 1) {
        let index = list.indexOf(tab);
        if (index == 0 || (index + 1) < listLength)
          gBrowser.selectTabAtIndex(index + 1);
        else
          gBrowser.selectTabAtIndex(index - 1);
      } else {
        shouldShowTabView = true;
      }
    } else
      shouldUpdateTabBar = true

    
    if (tab.tabItem.parent)
      tab.tabItem.parent.remove(tab.tabItem);

    
    if (groupItemId) {
      groupItem = GroupItems.groupItem(groupItemId);
      groupItem.add(tab.tabItem);
      UI.setReorderTabItemsOnShow(groupItem);
    } else {
      let pageBounds = Items.getPageBounds();
      pageBounds.inset(20, 20);

      let box = new Rect(pageBounds);
      box.width = 250;
      box.height = 200;

      new GroupItem([ tab.tabItem ], { bounds: box });
    }

    if (shouldUpdateTabBar)
      this.updateTabBar();
    else if (shouldShowTabView) {
      tab.tabItem.setZoomPrep(false);
      UI.showTabView();
    }
  },

  
  
  
  killNewTabGroup: function() {
    this.groupItems.forEach(function(groupItem) {
      if (groupItem.getTitle() == 'New Tabs' && groupItem.locked.title) {
        groupItem.removeAll();
        groupItem.close();
      }
    });
  }
};
