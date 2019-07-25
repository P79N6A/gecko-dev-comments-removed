


































































function GroupItem(listOfEls, options) {
  if (typeof options == 'undefined')
    options = {};

  this._inited = false;
  this._uninited = false;
  this._children = []; 
  this.defaultSize = new Point(TabItems.tabWidth * 1.5, TabItems.tabHeight * 1.5);
  this.isAGroupItem = true;
  this.id = options.id || GroupItems.getNextID();
  this._isStacked = false;
  this._stackAngles = [0];
  this.expanded = null;
  this.locked = (options.locked ? Utils.copy(options.locked) : {});
  this.topChild = null;
  this.hidden = false;
  this.fadeAwayUndoButtonDelay = 15000;
  this.fadeAwayUndoButtonDuration = 300;

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
  let immediately = options.immediately || $container ? true : false;
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
    .addClass('newTabButton')
    .click(function() {
      self.newTab();
    })
    .attr('title', tabviewString('groupItem.newTabButton'))
    .appendTo($container);

  
  this.$resizer = iQ("<div>")
    .addClass('resizer')
    .appendTo($container)
    .hide();

  
  var html =
    "<div class='title-container'>" +
      "<input class='name'/>" +
      "<div class='title-shield' />" +
    "</div>";

  this.$titlebar = iQ('<div>')
    .addClass('titlebar')
    .html(html)
    .appendTo($container);

  var $close = iQ('<div>')
    .addClass('close')
    .click(function() {
      self.closeAll();
    })
    .appendTo($container);

  
  this.$titleContainer = iQ('.title-container', this.$titlebar);
  this.$title = iQ('.name', this.$titlebar);
  this.$titleShield = iQ('.title-shield', this.$titlebar);
  this.setTitle(options.title || this.defaultName);

  var titleUnfocus = function(immediately) {
    self.$titleShield.show();
    if (!self.getTitle()) {
      self.$title
        .addClass("defaultName")
        .val(self.defaultName)
        .css({"background-image":null, "-moz-padding-start":null});
    } else {
      self.$title.css({"background-image":"none"});
      if (immediately) {
        self.$title.css({
            "-moz-padding-start": "1px"
          });
      } else {
        self.$title.animate({
            "-moz-padding-start": "1px"
          }, {
            duration: 200,
            easing: "tabviewBounce"
          });
      }
    }
  };

  var handleKeyDown = function(e) {
    if (e.which == 13 || e.which == 27) { 
      (self.$title)[0].blur();
      self.$title
        .addClass("transparentBorder")
        .one("mouseout", function() {
          self.$title.removeClass("transparentBorder");
        });
      e.stopPropagation();
      e.preventDefault();
    }
  };

  var handleKeyUp = function(e) {
    
    
    
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
    .keydown(handleKeyDown)
    .keyup(handleKeyUp);

  titleUnfocus(immediately);

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

  
  this.$expander = iQ("<div/>")
    .addClass("stackExpander")
    .appendTo($container)
    .hide();

  
  this.$appTabTray = iQ("<div/>")
    .addClass("appTabTray")
    .appendTo($container);

  AllTabs.tabs.forEach(function(xulTab) {
    if (xulTab.pinned && xulTab.ownerDocument.defaultView == gWindow)
      self.addAppTab(xulTab);
  });

  
  if (this.locked.bounds)
    $container.css({cursor: 'default'});

  if (this.locked.close)
    $close.hide();

  
  this.$undoContainer = null;
  this._undoButtonTimeoutId = null;

  
  this._init($container[0]);

  if (this.$debug)
    this.$debug.css({zIndex: -1000});

  
  Array.prototype.forEach.call(listOfEls, function(el) {
    self.add(el, null, options);
  });

  
  this._addHandlers($container);

  if (!this.locked.bounds)
    this.setResizable(true, immediately);

  GroupItems.register(this);

  
  this.setBounds(rectToBe, immediately);
  if (options.dontPush) {
    this.setZ(drag.zIndex);
    drag.zIndex++; 
  } else
    
    this.snap(immediately);
  if ($container)
    this.setBounds(rectToBe, immediately);

  this._inited = true;
  this.save();
};


GroupItem.prototype = Utils.extend(new Item(), new Subscribable(), {
  
  
  
  defaultName: tabviewString('groupItem.defaultName'),

  
  
  
  
  setActiveTab: function GroupItem_setActiveTab(tab) {
    Utils.assertThrow((!tab && this._children.length == 0) || tab.isATabItem,
        "tab must be null (if no children) or a TabItem");

    this._activeTab = tab;
  },

  
  
  
  
  getActiveTab: function GroupItem_getActiveTab() {
    return this._activeTab;
  },

  
  
  
  getStorageData: function GroupItem_getStorageData() {
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

  
  
  
  isEmpty: function GroupItem_isEmpty() {
    return !this._children.length && !this.getTitle();
  },

  
  
  
  save: function GroupItem_save() {
    if (!this._inited || this._uninited) 
      return;

    var data = this.getStorageData();
    if (GroupItems.groupItemStorageSanity(data))
      Storage.saveGroupItem(gWindow, data);
  },

  
  
  
  deleteData: function GroupItem_deleteData() {
    this._uninited = true;
    Storage.deleteGroupItem(gWindow, this.id);
  },

  
  
  
  getTitle: function GroupItem_getTitle() {
    var value = (this.$title ? this.$title.val() : '');
    return (value == this.defaultName ? '' : value);
  },

  
  
  
  setTitle: function GroupItem_setTitle(value) {
    this.$title.val(value);
    this.save();
  },

  
  
  
  adjustTitleSize: function GroupItem_adjustTitleSize() {
    Utils.assert(this.bounds, 'bounds needs to have been set');
    let closeButton = iQ('.close', this.container);
    var dimension = UI.rtl ? 'left' : 'right';
    var w = Math.min(this.bounds.width - parseInt(closeButton.width()) - parseInt(closeButton.css(dimension)),
                     Math.max(150, this.getTitle().length * 6));
    
    
    var css = {width: w};
    this.$title.css(css);
    this.$titleShield.css(css);
  },

  
  
  
  getContentBounds: function GroupItem_getContentBounds() {
    var box = this.getBounds();
    var titleHeight = this.$titlebar.height();
    box.top += titleHeight;
    box.height -= titleHeight;

    var appTabTrayWidth = this.$appTabTray.width();
    box.width -= appTabTrayWidth;
    if (UI.rtl) {
      box.left += appTabTrayWidth;
    }

    
    
    box.inset(6, 6);
    box.height -= 33; 

    return box;
  },

  
  
  
  
  
  
  
  
  
  
  
  setBounds: function GroupItem_setBounds(rect, immediately, options) {
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

  
  
  
  setZ: function GroupItem_setZ(value) {
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

  
  
  
  close: function GroupItem_close() {
    this.removeAll();
    GroupItems.unregister(this);
    this._sendToSubscribers("close");
    this.removeTrenches();

    if (this.hidden) {
      iQ(this.container).remove();
      if (this.$undoContainer) {
        this.$undoContainer.remove();
        this.$undoContainer = null;
       }
      Items.unsquish();
    } else {
      iQ(this.container).animate({
        opacity: 0,
        "-moz-transform": "scale(.3)",
      }, {
        duration: 170,
        complete: function() {
          iQ(this).remove();
          Items.unsquish();
        }
      });
    }
    this.deleteData();
  },

  
  
  
  closeAll: function GroupItem_closeAll() {
    let closeCenter = this.getBounds().center();
    if (this._children.length > 0) {
      this._children.forEach(function(child) {
        iQ(child.container).hide();
      });

      iQ(this.container).animate({
         opacity: 0,
         "-moz-transform": "scale(.3)",
      }, {
        duration: 170,
        complete: function() {
          iQ(this).hide();
        }
      });

      this._createUndoButton();
    } else {
      if (!this.locked.close)
        this.close();
    }
    
    UI.setActiveTab( UI.getClosestTab(closeCenter) );
  },

  
  
  
  _unhide: function GroupItem__unhide() {
    let self = this;

    this._cancelFadeAwayUndoButtonTimer();
    this.hidden = false;
    this.$undoContainer.remove();
    this.$undoContainer = null;

    iQ(this.container).show().animate({
      "-moz-transform": "scale(1)",
      "opacity": 1
    }, {
      duration: 170,
      complete: function() {
        self._children.forEach(function(child) {
          iQ(child.container).show();
        });
      }
    });

    self._sendToSubscribers("groupShown", { groupItemId: self.id });
  },

  
  
  
  closeHidden: function GroupItem_closeHidden() {
    let self = this;

    this._cancelFadeAwayUndoButtonTimer();

    
    
    
    
    
    let shouldRemoveTabItems = [];
    let toClose = this._children.concat();
    toClose.forEach(function(child) {
      child.removeSubscriber(self, "close");

      let removed = child.close();
      if (removed) {
        shouldRemoveTabItems.push(child);
      } else {
        
        
        child.addSubscriber(self, "close", function() {
          self.remove(child);
        });
      }
    });

    if (shouldRemoveTabItems.length != toClose.length) {
      
      shouldRemoveTabItems.forEach(function(child) {
        self.remove(child, { dontArrange: true });
      });

      this.$undoContainer.fadeOut(function() { self._unhide() });
    } else {
      this.close();
    }
  },

  
  
  
  _fadeAwayUndoButton: function GroupItem__fadeAwayUdoButton() {
    let self = this;

    if (this.$undoContainer) {
      
      
      let shouldFadeAway = GroupItems.getOrphanedTabs().length > 0;
      
      if (!shouldFadeAway && GroupItems.groupItems.length > 1) {
        shouldFadeAway = 
          GroupItems.groupItems.some(function(groupItem) {
            return (groupItem != self && groupItem.getChildren().length > 0);
          });
      }
      if (shouldFadeAway) {
        self.$undoContainer.animate({
          color: "transparent",
          opacity: 0
        }, {
          duration: this._fadeAwayUndoButtonDuration,
          complete: function() { self.closeHidden(); }
        });
      }
    }
  },

  
  
  
  _createUndoButton: function GroupItem__createUndoButton() {
    let self = this;
    this.$undoContainer = iQ("<div/>")
      .addClass("undo")
      .attr("type", "button")
      .text(tabviewString("groupItem.undoCloseGroup"))
      .appendTo("body");
    let undoClose = iQ("<span/>")
      .addClass("close")
      .appendTo(this.$undoContainer);

    this.$undoContainer.css({
      left: this.bounds.left + this.bounds.width/2 - iQ(self.$undoContainer).width()/2,
      top:  this.bounds.top + this.bounds.height/2 - iQ(self.$undoContainer).height()/2,
      "-moz-transform": "scale(.1)",
      opacity: 0
    });
    this.hidden = true;

    
    setTimeout(function() {
      self.$undoContainer.animate({
        "-moz-transform": "scale(1)",
        "opacity": 1
      }, {
        easing: "tabviewBounce",
        duration: 170,
        complete: function() {
          self._sendToSubscribers("groupHidden", { groupItemId: self.id });
        }
      });
    }, 50);

    
    this.$undoContainer.click(function(e) {
      
      if (e.target.nodeName != self.$undoContainer[0].nodeName)
        return;

      self.$undoContainer.fadeOut(function() { self._unhide(); });
    });

    undoClose.click(function() {
      self.$undoContainer.fadeOut(function() { self.closeHidden(); });
    });

    this.setupFadeAwayUndoButtonTimer();
    
    
    this.$undoContainer.mouseover(function() { 
      self._cancelFadeAwayUndoButtonTimer();
    });
    this.$undoContainer.mouseout(function() {
      self.setupFadeAwayUndoButtonTimer();
    });
  },

  
  
  setupFadeAwayUndoButtonTimer: function() {
    let self = this;

    if (!this._undoButtonTimeoutId) {
      this._undoButtonTimeoutId = setTimeout(function() { 
        self._fadeAwayUndoButton(); 
      }, this.fadeAwayUndoButtonDelay);
    }
  },
  
  
  
  _cancelFadeAwayUndoButtonTimer: function() {
    clearTimeout(this._undoButtonTimeoutId);
    this._undoButtonTimeoutId = null;
  }, 

  
  
  
  
  
  
  
  
  
  
  add: function GroupItem_add(a, dropPos, options) {
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

      if (typeof options == 'undefined')
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

      
      var index = dropPos ? findInsertionPoint(dropPos) : this._children.length;
      this._children.splice(index, 0, item);

      item.setZ(this.getZ() + 1);
      $el.addClass("tabInGroupItem");

      if (!wasAlreadyInThisGroupItem) {
        item.droppable(false);
        item.groupItemData = {};

        item.addSubscriber(this, "close", function() {
          self.remove(item);
        });

        item.setParent(this);

        if (typeof item.setResizable == 'function')
          item.setResizable(false, options.immediately);

        
        if (iQ(item.container).hasClass("focus"))
          this.setActiveTab(item);

        
        
        if (item.tab == gBrowser.selectedTab || 
            (!GroupItems.getActiveGroupItem() && !item.tab.hidden))
          GroupItems.setActiveGroupItem(this);
      }

      if (!options.dontArrange)
        this.arrange({animate: !options.immediately});

      this._sendToSubscribers("childAdded",{ groupItemId: this.id, item: item });

      UI.setReorderTabsOnHide(this);
    } catch(e) {
      Utils.log('GroupItem.add error', e);
    }
  },

  
  
  
  
  
  
  
  
  
  remove: function GroupItem_remove(a, options) {
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

      if (typeof options == 'undefined')
        options = {};

      var index = this._children.indexOf(item);
      if (index != -1)
        this._children.splice(index, 1);

      if (item == this._activeTab) {
        if (this._children.length > 0)
          this._activeTab = this._children[0];
        else
          this._activeTab = null;
      }

      item.setParent(null);
      item.removeClass("tabInGroupItem");
      item.removeClass("stacked");
      item.removeClass("stack-trayed");
      item.setRotation(0);

      item.droppable(true);
      item.removeSubscriber(this, "close");

      if (typeof item.setResizable == 'function')
        item.setResizable(true, options.immediately);

      if (!this._children.length && !this.locked.close && !this.getTitle() && !options.dontClose) {
        this.close();
      } else if (!options.dontArrange) {
        this.arrange({animate: !options.immediately});
      }

      this._sendToSubscribers("childRemoved",{ groupItemId: this.id, item: item });

    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  removeAll: function GroupItem_removeAll() {
    var self = this;
    var toRemove = this._children.concat();
    toRemove.forEach(function(child) {
      self.remove(child, {dontArrange: true});
    });
  },

  
  
  addAppTab: function GroupItem_addAppTab(xulTab) {
    let self = this;

    
    let icon = xulTab.image || Utils.defaultFaviconURL;
    let $appTab = iQ("<img>")
      .addClass("appTabIcon")
      .attr("src", icon)
      .data("xulTab", xulTab)
      .appendTo(this.$appTabTray)
      .click(function(event) {
        if (Utils.isRightClick(event))
          return;

        GroupItems.setActiveGroupItem(self);
        UI.goToTab(iQ(this).data("xulTab"));
      });

    
    let columnWidth = $appTab.width();
    if (parseInt(this.$appTabTray.css("width")) != columnWidth) {
      this.$appTabTray.css({width: columnWidth});
      this.arrange();
    }
  },

  
  
  removeAppTab: function GroupItem_removeAppTab(xulTab) {
    
    iQ(".appTabIcon", this.$appTabTray).each(function(icon) {
      let $icon = iQ(icon);
      if ($icon.data("xulTab") != xulTab)
        return;
        
      $icon.remove();
    });
    
    
    if (!iQ(".appTabIcon", this.$appTabTray).length) {
      this.$appTabTray.css({width: 0});
      this.arrange();
    }
  },

  
  
  
  hideExpandControl: function GroupItem_hideExpandControl() {
    this.$expander.hide();
  },

  
  
  
  showExpandControl: function GroupItem_showExpandControl() {
    let parentBB = this.getBounds();
    let childBB = this.getChild(0).getBounds();
    let padding = 7;
    this.$expander
        .show()
        .css({
          opacity: .2,
          top: childBB.top + childBB.height - parentBB.top + padding,
          left: parentBB.width/2 - this.$expander.width()/2
        });
  },

  
  
  
  shouldStack: function GroupItem_shouldStack(count) {
    if (count <= 1)
      return false;

    var bb = this.getContentBounds();
    var options = {
      return: 'widthAndColumns',
      count: count || this._children.length
    };
    let {childWidth, columns} = Items.arrange(null, bb, options);

    let shouldStack = childWidth < TabItems.minTabWidth * 1.35;
    this._columns = shouldStack ? null : columns;

    return shouldStack;
  },

  
  
  
  
  
  
  arrange: function GroupItem_arrange(options) {
    if (GroupItems._arrangePaused) {
      GroupItems.pushArrange(this, options);
      return;
    }
    if (this.expanded) {
      this.topChild = null;
      var box = new Rect(this.expanded.bounds);
      box.inset(8, 8);
      Items.arrange(this._children, box, Utils.extend({}, options, {z: 99999}));
    } else {
      var bb = this.getContentBounds();
      if (!this.shouldStack()) {
        if (!options)
          options = {};

        this._children.forEach(function(child) {
            child.removeClass("stacked")
        });

        this.topChild = null;

        if (!this._children.length) {
          this.xDensity = 0;
          this.yDensity = 0;
          return;
        }

        var arrangeOptions = Utils.copy(options);
        Utils.extend(arrangeOptions, {
          columns: this._columns
        });

        
        

        var rects = Items.arrange(this._children, bb, arrangeOptions);

        
        
        this.yDensity = (rects[rects.length - 1].bottom - bb.top) / (bb.height);

        
        

        
        var rightMostRight = 0;
        if (UI.rtl) {
          rightMostRight = rects[0].right;
        } else {
          for each (var rect in rects) {
            if (rect.right > rightMostRight)
              rightMostRight = rect.right;
            else
              break;
          }
        }
        this.xDensity = (rightMostRight - bb.left) / (bb.width);

        this._isStacked = false;
      } else
        this._stackArrange(bb, options);
    }

    if (this._isStacked && !this.expanded) this.showExpandControl();
    else this.hideExpandControl();
  },

  
  
  
  
  
  
  
  
  
  
  _stackArrange: function GroupItem__stackArrange(bb, options) {
    var animate;
    if (!options || typeof options.animate == 'undefined')
      animate = true;
    else
      animate = options.animate;

    if (typeof options == 'undefined')
      options = {};

    var count = this._children.length;
    if (!count)
      return;

    var zIndex = this.getZ() + count + 1;

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
        child.setRotation((UI.rtl ? -1 : 1) * self._randRotate(maxRotation, index));
      }
    });

    self._isStacked = true;
  },

  
  
  
  _randRotate: function GroupItem__randRotate(spread, index) {
    if (index >= this._stackAngles.length) {
      var randAngle = 5*index + parseInt((Math.random()-.5)*1);
      this._stackAngles.push(randAngle);
      return randAngle;
    }

    if (index > 5) index = 5;

    return this._stackAngles[index];
  },

  
  
  
  
  
  
  
  childHit: function GroupItem_childHit(child) {
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

  expand: function GroupItem_expand() {
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
    pos.left -= overlayWidth / 3;
    pos.top  -= overlayHeight / 3;

    if (pos.top < 0)
      pos.top = 20;
    if (pos.left < 0)
      pos.left = 20;
    if (pos.top + overlayHeight > window.innerHeight)
      pos.top = window.innerHeight - overlayHeight - 20;
    if (pos.left + overlayWidth > window.innerWidth)
      pos.left = window.innerWidth - overlayWidth - 20;

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
      .addClass('shield')
      .css({
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

  
  
  
  collapse: function GroupItem_collapse() {
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

  
  
  
  _addHandlers: function GroupItem__addHandlers(container) {
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

    this.droppable(true);

    this.$expander.click(function() {
      self.expand();
    });
  },

  
  
  
  setResizable: function GroupItem_setResizable(value, immediately) {
    this.resizeOptions.minWidth = 110;
    this.resizeOptions.minHeight = 125;

    if (value) {
      immediately ? this.$resizer.show() : this.$resizer.fadeIn();
      this.resizable(true);
    } else {
      immediately ? this.$resizer.hide() : this.$resizer.fadeOut();
      this.resizable(false);
    }
  },

  
  
  
  newTab: function GroupItem_newTab(url) {
    GroupItems.setActiveGroupItem(this);
    let newTab = gBrowser.loadOneTab(url || "about:blank", {inBackground: true});

    
    
    
    newTab.tabItem.zoomIn(!url);
  },

  
  
  
  
  
  
  reorderTabItemsBasedOnTabOrder: function GroupItem_reorderTabItemsBasedOnTabOrder() {
    this._children.sort(function(a,b) a.tab._tPos - b.tab._tPos);

    this.arrange({animate: false});
    
  },

  
  
  
  reorderTabsBasedOnTabItemOrder: function GroupItem_reorderTabsBasedOnTabItemOrder() {
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
        return false;
      });
    });
  },

  
  
  
  setTopChild: function GroupItem_setTopChild(topChild) {
    this.topChild = topChild;

    this.arrange({animate: false});
    
  },

  
  
  
  
  
  
  
  getChild: function GroupItem_getChild(index) {
    if (index < 0)
      index = this._children.length + index;
    if (index >= this._children.length || index < 0)
      return null;
    return this._children[index];
  },

  
  
  
  getChildren: function GroupItem_getChildren() {
    return this._children;
  }
});




let GroupItems = {
  groupItems: [],
  nextID: 1,
  _inited: false,
  _activeGroupItem: null,
  _activeOrphanTab: null,
  _cleanupFunctions: [],
  _arrangePaused: false,
  _arrangesPending: [],
  _removingHiddenGroups: false,

  
  
  init: function GroupItems_init() {
    let self = this;

    
    function handleAttrModified(xulTab) {
      self._handleAttrModified(xulTab);
    }

    AllTabs.register("attrModified", handleAttrModified);
    this._cleanupFunctions.push(function() {
      AllTabs.unregister("attrModified", handleAttrModified);
    });
  },

  
  
  uninit : function GroupItems_uninit () {
    
    this._cleanupFunctions.forEach(function(func) {
      func();
    });

    this._cleanupFunctions = [];

    
    this.groupItems = null;
  },

  
  
  
  
  pauseArrange: function GroupItems_pauseArrange() {
    Utils.assert(this._arrangePaused == false, 
      "pauseArrange has been called while already paused");
    Utils.assert(this._arrangesPending.length == 0, 
      "There are bypassed arrange() calls that haven't been resolved");
    this._arrangePaused = true;
  },

  
  
  
  
  pushArrange: function GroupItems_pushArrange(groupItem, options) {
    Utils.assert(this._arrangePaused, 
      "Ensure pushArrange() called while arrange()s aren't paused"); 
    let i;
    for (i = 0; i < this._arrangesPending.length; i++)
      if (this._arrangesPending[i].groupItem === groupItem)
        break;
    let arrangeInfo = {
      groupItem: groupItem,
      options: options
    };
    if (i < this._arrangesPending.length)
      this._arrangesPending[i] = arrangeInfo;
    else
      this._arrangesPending.push(arrangeInfo);
  },

  
  
  
  resumeArrange: function GroupItems_resumeArrange() {
    for (let i = 0; i < this._arrangesPending.length; i++) {
      let g = this._arrangesPending[i];
      g.groupItem.arrange(g.options);
    }
    this._arrangesPending = [];
    this._arrangePaused = false;
  },

  
  
  
  _handleAttrModified: function GroupItems__handleAttrModified(xulTab) {
    if (xulTab.ownerDocument.defaultView != gWindow || !xulTab.pinned)
      return;

    let iconUrl = xulTab.image || Utils.defaultFaviconURL;
    this.groupItems.forEach(function(groupItem) {
      iQ(".appTabIcon", groupItem.$appTabTray).each(function(icon) {
        let $icon = iQ(icon);
        if ($icon.data("xulTab") != xulTab)
          return;

        if (iconUrl != $icon.attr("src"))
          $icon.attr("src", iconUrl);
      });
    });
  },

  
  
  
  addAppTab: function GroupItems_addAppTab(xulTab) {
    this.groupItems.forEach(function(groupItem) {
      groupItem.addAppTab(xulTab);
    });
  },

  
  
  
  removeAppTab: function GroupItems_removeAppTab(xulTab) {
    this.groupItems.forEach(function(groupItem) {
      groupItem.removeAppTab(xulTab);
    });
  },

  
  
  
  getNextID: function GroupItems_getNextID() {
    var result = this.nextID;
    this.nextID++;
    this._save();
    return result;
  },

  
  
  
  getStorageData: function GroupItems_getStorageData() {
    var data = {nextID: this.nextID, groupItems: []};
    this.groupItems.forEach(function(groupItem) {
      data.groupItems.push(groupItem.getStorageData());
    });

    return data;
  },

  
  
  
  saveAll: function GroupItems_saveAll() {
    this._save();
    this.groupItems.forEach(function(groupItem) {
      groupItem.save();
    });
  },

  
  
  
  _save: function GroupItems__save() {
    if (!this._inited) 
      return;

    let activeGroupId = this._activeGroupItem ? this._activeGroupItem.id : null;
    Storage.saveGroupItemsData(
      gWindow, { nextID: this.nextID, activeGroupId: activeGroupId });
  },

  
  
  
  getBoundingBox: function GroupItems_getBoundingBox(els) {
    var bounds = [iQ(el).bounds() for each (el in els)];
    var left   = Math.min.apply({},[ b.left   for each (b in bounds) ]);
    var top    = Math.min.apply({},[ b.top    for each (b in bounds) ]);
    var right  = Math.max.apply({},[ b.right  for each (b in bounds) ]);
    var bottom = Math.max.apply({},[ b.bottom for each (b in bounds) ]);

    return new Rect(left, top, right-left, bottom-top);
  },

  
  
  
  
  reconstitute: function GroupItems_reconstitute(groupItemsData, groupItemData) {
    try {
      let activeGroupId;

      if (groupItemsData) {
        if (groupItemsData.nextID)
          this.nextID = groupItemsData.nextID;
        if (groupItemsData.activeGroupId)
          activeGroupId = groupItemsData.activeGroupId;
      }

      if (groupItemData) {
        for (var id in groupItemData) {
          var groupItem = groupItemData[id];
          if (this.groupItemStorageSanity(groupItem)) {
            var options = {
              dontPush: true,
              immediately: true
            };

            new GroupItem([], Utils.extend({}, groupItem, options));
          }
        }
      }
      
      if (activeGroupId) {
        let activeGroupItem = this.groupItem(activeGroupId);
        if (activeGroupItem)
          this.setActiveGroupItem(activeGroupItem);
      }

      this._inited = true;
      this._save(); 
    } catch(e) {
      Utils.log("error in recons: "+e);
    }
  },

  
  
  
  groupItemStorageSanity: function GroupItems_groupItemStorageSanity(groupItemData) {
    
    
    var sane = true;
    if (!Utils.isRect(groupItemData.bounds)) {
      Utils.log('GroupItems.groupItemStorageSanity: bad bounds', groupItemData.bounds);
      sane = false;
    }

    return sane;
  },

  
  
  
  register: function GroupItems_register(groupItem) {
    Utils.assert(groupItem, 'groupItem');
    Utils.assert(this.groupItems.indexOf(groupItem) == -1, 'only register once per groupItem');
    this.groupItems.push(groupItem);
    UI.updateTabButton();
  },

  
  
  
  unregister: function GroupItems_unregister(groupItem) {
    var index = this.groupItems.indexOf(groupItem);
    if (index != -1)
      this.groupItems.splice(index, 1);

    if (groupItem == this._activeGroupItem)
      this._activeGroupItem = null;

    UI.updateTabButton();
  },

  
  
  
  
  groupItem: function GroupItems_groupItem(a) {
    var result = null;
    this.groupItems.forEach(function(candidate) {
      if (candidate.id == a)
        result = candidate;
    });

    return result;
  },

  
  
  
  arrange: function GroupItems_arrange() {
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

  
  
  
  removeAll: function GroupItems_removeAll() {
    var toRemove = this.groupItems.concat();
    toRemove.forEach(function(groupItem) {
      groupItem.removeAll();
    });
  },

  
  
  
  newTab: function GroupItems_newTab(tabItem, options) {
    let activeGroupItem = this.getActiveGroupItem();

    
    
    
    
    
    
    
    
    

    if (activeGroupItem) {
      activeGroupItem.add(tabItem, null, options);
      return;
    }

    let orphanTabItem = this.getActiveOrphanTab();
    if (!orphanTabItem) {
      let otherTab;
      
      gBrowser.visibleTabs.some(function(tab) {
        if (!tab.pinned && tab != tabItem.tab) {
          otherTab = tab;
          return true;
        }
        return false;
      });

      if (otherTab) {
        
        
        if (otherTab.tabItem.parent) {
          let groupItem = otherTab.tabItem.parent;
          groupItem.add(tabItem);
          this.setActiveGroupItem(groupItem);
          return;
        }
        
        
        orphanTabItem = otherTab.tabItem;
      }

      if (!orphanTabItem) {
        
        if (this.groupItems.length > 0) {
          let groupItem = this.groupItems[0];
          groupItem.add(tabItem);
          this.setActiveGroupItem(groupItem);
          return;
        }
        
        
        let orphanedTabs = this.getOrphanedTabs();
        if (orphanedTabs.length > 0)
          orphanTabItem = orphanedTabs[0];
      }
    }

    
    let tabItems;
    let newGroupItemBounds; 
    
    
    if (orphanTabItem && orphanTabItem.tab != tabItem.tab) {
      newGroupItemBounds = orphanTabItem.getBoundsWithTitle();
      tabItems = [orphanTabItem, tabItem];
    } else {
      tabItem.setPosition(60, 60, true);
      newGroupItemBounds = tabItem.getBounds();
      tabItems = [tabItem];
    }

    newGroupItemBounds.inset(-40,-40);
    let newGroupItem = 
      new GroupItem(tabItems, { bounds: newGroupItemBounds });
    newGroupItem.snap();
    this.setActiveGroupItem(newGroupItem);
  },

  
  
  
  
  getActiveGroupItem: function GroupItems_getActiveGroupItem() {
    return this._activeGroupItem;
  },

  
  
  
  
  
  
  
  
  setActiveGroupItem: function GroupItems_setActiveGroupItem(groupItem) {
    if (this._activeGroupItem)
      iQ(this._activeGroupItem.container).removeClass('activeGroupItem');

    if (groupItem !== null) {
      if (groupItem)
        iQ(groupItem.container).addClass('activeGroupItem');
      
      this.setActiveOrphanTab(null);
    }

    this._activeGroupItem = groupItem;
    this._save();
  },

  
  
  
  getActiveOrphanTab: function GroupItems_getActiveOrphanTab() {
    return this._activeOrphanTab;
  },

  
  
  
  
  
  
  
  setActiveOrphanTab: function GroupItems_setActiveOrphanTab(tabItem) {
    this._activeOrphanTab = tabItem;
  },

  
  
  
  
  _updateTabBar: function GroupItems__updateTabBar() {
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

  
  
  
  updateActiveGroupItemAndTabBar: function GroupItems_updateActiveGroupItemAndTabBar(tabItem) {
    Utils.assertThrow(tabItem && tabItem.isATabItem, "tabItem must be a TabItem");

    let groupItem = tabItem.parent;
    this.setActiveGroupItem(groupItem);

    if (groupItem)
      groupItem.setActiveTab(tabItem);
    else
      this.setActiveOrphanTab(tabItem);

    this._updateTabBar();
  },

  
  
  
  getOrphanedTabs: function GroupItems_getOrphanedTabs() {
    var tabs = TabItems.getItems();
    tabs = tabs.filter(function(tab) {
      return tab.parent == null;
    });
    return tabs;
  },

  
  
  
  
  
  getNextGroupItemTab: function GroupItems_getNextGroupItemTab(reverse) {
    var groupItems = Utils.copy(GroupItems.groupItems);
    var activeGroupItem = GroupItems.getActiveGroupItem();
    var activeOrphanTab = GroupItems.getActiveOrphanTab();
    var tabItem = null;

    if (reverse)
      groupItems = groupItems.reverse();

    if (!activeGroupItem) {
      if (groupItems.length > 0) {
        groupItems.some(function(groupItem) {
          if (!groupItem.hidden) {
            
            let activeTab = groupItem.getActiveTab();
            if (activeTab) {
              tabItem = activeTab;
              return true;
            }
            
            var child = groupItem.getChild(0);
            if (child) {
              tabItem = child;
              return true;
            }
          }
          return false;
        });
      }
    } else {
      var currentIndex;
      groupItems.some(function(groupItem, index) {
        if (!groupItem.hidden && groupItem == activeGroupItem) {
          currentIndex = index;
          return true;
        }
        return false;
      });
      var firstGroupItems = groupItems.slice(currentIndex + 1);
      firstGroupItems.some(function(groupItem) {
        if (!groupItem.hidden) {
          
          let activeTab = groupItem.getActiveTab();
          if (activeTab) {
            tabItem = activeTab;
            return true;
          }
          
          var child = groupItem.getChild(0);
          if (child) {
            tabItem = child;
            return true;
          }
        }
        return false;
      });
      if (!tabItem) {
        var orphanedTabs = GroupItems.getOrphanedTabs();
        if (orphanedTabs.length > 0)
          tabItem = orphanedTabs[0];
      }
      if (!tabItem) {
        var secondGroupItems = groupItems.slice(0, currentIndex);
        secondGroupItems.some(function(groupItem) {
          if (!groupItem.hidden) {
            
            let activeTab = groupItem.getActiveTab();
            if (activeTab) {
              tabItem = activeTab;
              return true;
            }
            
            var child = groupItem.getChild(0);
            if (child) {
              tabItem = child;
              return true;
            }
          }
          return false;
        });
      }
    }
    return tabItem;
  },

  
  
  
  
  
  
  
  moveTabToGroupItem : function GroupItems_moveTabToGroupItem (tab, groupItemId) {
    if (tab.pinned)
      return;

    Utils.assertThrow(tab.tabItem, "tab must be linked to a TabItem");

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
        shouldUpdateTabBar = true;
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
      this._updateTabBar();
    else if (shouldShowTabView) {
      tab.tabItem.setZoomPrep(false);
      UI.showTabView();
    }
  },

  
  
  
  killNewTabGroup: function GroupItems_killNewTabGroup() {
    
    
    let newTabGroupTitle = "New Tabs";
    this.groupItems.forEach(function(groupItem) {
      if (groupItem.getTitle() == newTabGroupTitle && groupItem.locked.title) {
        groupItem.removeAll();
        groupItem.close();
      }
    });
  },

  
  
  
  removeHiddenGroups: function GroupItems_removeHiddenGroups() {
    if (this._removingHiddenGroups)
      return;
    this._removingHiddenGroups = true;

    let groupItems = this.groupItems.concat();
    groupItems.forEach(function(groupItem) {
      if (groupItem.hidden)
        groupItem.closeHidden();
     });

    this._removingHiddenGroups = false;
  }
};
