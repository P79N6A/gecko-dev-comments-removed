



































































function GroupItem(listOfEls, options) {
  if (!options)
    options = {};

  this._inited = false;
  this._uninited = false;
  this._children = []; 
  this.defaultSize = new Point(TabItems.tabWidth * 1.5, TabItems.tabHeight * 1.5);
  this.isAGroupItem = true;
  this.id = options.id || GroupItems.getNextID();
  this._isStacked = false;
  this.expanded = null;
  this.hidden = false;
  this.fadeAwayUndoButtonDelay = 15000;
  this.fadeAwayUndoButtonDuration = 300;

  this.keepProportional = false;
  this._frozenItemSizeData = {};

  
  this._lastClick = 0;
  this._lastClickPositions = null;

  
  
  this._activeTab = null;

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
      "<input class='name' placeholder='" + this.defaultName + "'/>" +
      "<div class='title-shield' />" +
    "</div>";

  this.$titlebar = iQ('<div>')
    .addClass('titlebar')
    .html(html)
    .appendTo($container);

  this.$closeButton = iQ('<div>')
    .addClass('close')
    .click(function() {
      self.closeAll();
    })
    .appendTo($container);

  
  this.$titleContainer = iQ('.title-container', this.$titlebar);
  this.$title = iQ('.name', this.$titlebar);
  this.$titleShield = iQ('.title-shield', this.$titlebar);
  this.setTitle(options.title);

  var handleKeyPress = function (e) {
    if (e.keyCode == KeyEvent.DOM_VK_ESCAPE ||
        e.keyCode == KeyEvent.DOM_VK_RETURN ||
        e.keyCode == KeyEvent.DOM_VK_ENTER) {
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
    .blur(function() {
      self._titleFocused = false;
      self.$titleShield.show();
      if (self.getTitle())
        gTabView.firstUseExperienced = true;
    })
    .focus(function() {
      self._unfreezeItemSize();
      if (!self._titleFocused) {
        (self.$title)[0].select();
        self._titleFocused = true;
      }
    })
    .mousedown(function(e) {
      e.stopPropagation();
    })
    .keypress(handleKeyPress)
    .keyup(handleKeyUp);

  this.$titleShield
    .mousedown(function(e) {
      self.lastMouseDownTarget = (Utils.isLeftClick(e) ? e.target : null);
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

  
  this.$expander = iQ("<div/>")
    .addClass("stackExpander")
    .appendTo($container)
    .hide();

  
  let appTabTrayContainer = iQ("<div/>")
    .addClass("appTabTrayContainer")
    .appendTo($container);
  this.$appTabTray = iQ("<div/>")
    .addClass("appTabTray")
    .appendTo(appTabTrayContainer);

  AllTabs.tabs.forEach(function(xulTab) {
    if (xulTab.pinned && xulTab.ownerDocument.defaultView == gWindow)
      self.addAppTab(xulTab, {dontAdjustTray: true});
  });

  
  this.$undoContainer = null;
  this._undoButtonTimeoutId = null;

  
  this._init($container[0]);

  
  Array.prototype.forEach.call(listOfEls, function(el) {
    self.add(el, options);
  });

  
  this._addHandlers($container);

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

  GroupItems.updateGroupCloseButtons();
};


GroupItem.prototype = Utils.extend(new Item(), new Subscribable(), {
  
  
  
  toString: function GroupItem_toString() {
    return "[GroupItem id=" + this.id + "]";
  },

  
  
  
  defaultName: tabviewString('groupItem.defaultName'),

  
  
  
  
  setActiveTab: function GroupItem_setActiveTab(tab) {
    Utils.assertThrow((!tab && this._children.length == 0) || tab.isATabItem,
        "tab must be null (if no children) or a TabItem");

    this._activeTab = tab;
    this.arrange({immediately: true});
  },

  
  
  
  
  getActiveTab: function GroupItem_getActiveTab() {
    return this._activeTab;
  },

  
  
  
  getStorageData: function GroupItem_getStorageData() {
    var data = {
      bounds: this.getBounds(),
      userSize: null,
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

  
  
  
  isStacked: function GroupItem_isStacked() {
    return this._isStacked;
  },

  
  
  
  
  
  isTopOfStack: function GroupItem_isTopOfStack(item) {
    return this.isStacked() && item == this.getTopChild();
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
    return this.$title ? this.$title.val() : '';
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

  
  
  
  
  
  
  
  
  adjustAppTabTray: function GroupItem_adjustAppTabTray(arrangeGroup) {
    let icons = iQ(".appTabIcon", this.$appTabTray);
    let container = iQ(this.$appTabTray[0].parentNode);
    if (!icons.length) {
      
      if (parseInt(container.css("width")) != 0) {
        this.$appTabTray.css("-moz-column-count", "auto");
        this.$appTabTray.css("height", 0);
        container.css("width", 0);
        container.css("height", 0);

        if (container.hasClass("appTabTrayContainerTruncated"))
          container.removeClass("appTabTrayContainerTruncated");

        if (arrangeGroup)
          this.arrange();
      }
      return;
    }

    let iconBounds = iQ(icons[0]).bounds();
    let boxBounds = this.getBounds();
    let contentHeight = boxBounds.height -
                        parseInt(container.css("top")) -
                        this.$resizer.height();
    let rows = Math.floor(contentHeight / iconBounds.height);
    let columns = Math.ceil(icons.length / rows);
    let columnsGap = parseInt(this.$appTabTray.css("-moz-column-gap"));
    let iconWidth = iconBounds.width + columnsGap;
    let maxColumns = Math.floor((boxBounds.width * 0.20) / iconWidth);

    Utils.assert(rows > 0 && columns > 0 && maxColumns > 0,
      "make sure the calculated rows, columns and maxColumns are correct");

    if (columns > maxColumns)
      container.addClass("appTabTrayContainerTruncated");
    else if (container.hasClass("appTabTrayContainerTruncated"))
      container.removeClass("appTabTrayContainerTruncated");

    
    
    if (parseInt(this.$appTabTray.css("-moz-column-count")) != columns)
      this.$appTabTray.css("-moz-column-count", columns);

    if (parseInt(this.$appTabTray.css("height")) != contentHeight) {
      this.$appTabTray.css("height", contentHeight + "px");
      container.css("height", contentHeight + "px");
    }

    let fullTrayWidth = iconWidth * columns - columnsGap;
    if (parseInt(this.$appTabTray.css("width")) != fullTrayWidth)
      this.$appTabTray.css("width", fullTrayWidth + "px");

    let trayWidth = iconWidth * Math.min(columns, maxColumns) - columnsGap;
    if (parseInt(container.css("width")) != trayWidth) {
      container.css("width", trayWidth + "px");

      
      if (arrangeGroup)
        this.arrange();
    }
  },

  
  
  
  getContentBounds: function GroupItem_getContentBounds() {
    var box = this.getBounds();
    var titleHeight = this.$titlebar.height();
    box.top += titleHeight;
    box.height -= titleHeight;

    let appTabTrayContainer = iQ(this.$appTabTray[0].parentNode);
    var appTabTrayWidth = appTabTrayContainer.width();
    if (appTabTrayWidth)
      appTabTrayWidth += parseInt(appTabTrayContainer.css(UI.rtl ? "left" : "right"));

    box.width -= appTabTrayWidth;
    if (UI.rtl) {
      box.left += appTabTrayWidth;
    }

    
    
    box.inset(6, 6);
    box.height -= 33; 

    return box;
  },

  
  
  
  
  
  
  
  
  
  
  
  setBounds: function GroupItem_setBounds(inRect, immediately, options) {
    if (!Utils.isRect(inRect)) {
      Utils.trace('GroupItem.setBounds: rect is not a real rectangle!', inRect);
      return;
    }

    
    let validSize = GroupItems.calcValidSize(
      new Point(inRect.width, inRect.height));
    let rect = new Rect(inRect.left, inRect.top, validSize.x, validSize.y);

    if (!options)
      options = {};

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

    
    if (css.width || css.height)
      this.adjustAppTabTray();

    
    if (css.width || css.height) {
      this.arrange({animate: !immediately}); 
    } else if (css.left || css.top) {
      this._children.forEach(function(child) {
        if (!child.getHidden()) {
          var box = child.getBounds();
          child.setPosition(box.left + offset.x, box.top + offset.y, immediately);
        }
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

    if (css.width) {      
      this.adjustTitleSize();
    }

    UI.clearShouldResizeItems();

    this.setTrenches(rect);

    this.save();
  },

  
  
  
  setZ: function GroupItem_setZ(value) {
    this.zIndex = value;

    iQ(this.container).css({zIndex: value});

    var count = this._children.length;
    if (count) {
      var topZIndex = value + count + 1;
      var zIndex = topZIndex;
      var self = this;
      this._children.forEach(function(child) {
        if (child == self.getTopChild())
          child.setZ(topZIndex + 1);
        else {
          child.setZ(zIndex);
          zIndex--;
        }
      });
    }
  },

  
  
  
  
  
  
  
  
  
  close: function GroupItem_close(options) {
    this.removeAll({dontClose: true});
    GroupItems.unregister(this);

    
    this._unfreezeItemSize({dontArrange: true});

    let self = this;
    let destroyGroup = function () {
      iQ(self.container).remove();
      if (self.$undoContainer) {
        self.$undoContainer.remove();
        self.$undoContainer = null;
      }
      self.removeTrenches();
      Items.unsquish();
      self._sendToSubscribers("close");
      GroupItems.updateGroupCloseButtons();
    }

    if (this.hidden || (options && options.immediately)) {
      destroyGroup();
    } else {
      iQ(this.container).animate({
        opacity: 0,
        "-moz-transform": "scale(.3)",
      }, {
        duration: 170,
        complete: destroyGroup
      });
    }

    this.deleteData();
  },

  
  
  
  closeAll: function GroupItem_closeAll() {
    if (this._children.length > 0) {
      this._unfreezeItemSize();
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

      this.droppable(false);
      this.removeTrenches();
      this._createUndoButton();
    } else
      this.close();
    
    this._makeClosestTabActive();
  },
  
  
  
  
  
  _makeClosestTabActive: function GroupItem__makeClosestTabActive() {
    let closeCenter = this.getBounds().center();
    
    let closestTabItem = UI.getClosestTab(closeCenter);
    UI.setActive(closestTabItem);
  },

  
  
  
  
  
  closeIfEmpty: function() {
    if (!this._children.length && !this.getTitle() &&
        !GroupItems.getUnclosableGroupItemId() &&
        !GroupItems._autoclosePaused) {
      this.close();
      return true;
    }
    return false;
  },

  
  
  
  _unhide: function GroupItem__unhide() {
    let self = this;

    this._cancelFadeAwayUndoButtonTimer();
    this.hidden = false;
    this.$undoContainer.remove();
    this.$undoContainer = null;
    this.droppable(true);
    this.setTrenches(this.bounds);

    iQ(this.container).show().animate({
      "-moz-transform": "scale(1)",
      "opacity": 1
    }, {
      duration: 170,
      complete: function() {
        self._children.forEach(function(child) {
          iQ(child.container).show();
        });

        UI.setActive(self);
        self._sendToSubscribers("groupShown", { groupItemId: self.id });
      }
    });

    GroupItems.updateGroupCloseButtons();
  },

  
  
  
  closeHidden: function GroupItem_closeHidden() {
    let self = this;

    this._cancelFadeAwayUndoButtonTimer();

    
    
    let remainingGroups = GroupItems.groupItems.filter(function (groupItem) {
      return (groupItem != self && groupItem.getChildren().length);
    });
    if (!gBrowser._numPinnedTabs && !GroupItems.getOrphanedTabs().length &&
        !remainingGroups.length) {
      let emptyGroups = GroupItems.groupItems.filter(function (groupItem) {
        return (groupItem != self && !groupItem.getChildren().length);
      });
      let group = (emptyGroups.length ? emptyGroups[0] : GroupItems.newGroup());
      group.newTab();
    }

    this.destroy();
  },

  
  
  
  
  
  
  
  
  
  
  destroy: function GroupItem_destroy(options) {
    let self = this;

    
    
    
    
    
    let shouldRemoveTabItems = [];
    let toClose = this._children.concat();
    toClose.forEach(function(child) {
      child.removeSubscriber(self, "close");

      let removed = child.close(true);
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
      this.close(options);
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
      .appendTo("body");
    iQ("<span/>")
      .text(tabviewString("groupItem.undoCloseGroup"))
      .appendTo(this.$undoContainer);
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
      
      if (e.target == undoClose[0])
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

    GroupItems.updateGroupCloseButtons();
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  add: function GroupItem_add(a, options) {
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

      
      if (item.parent && item.parent !== this)
        item.parent.remove(item);

      item.removeTrenches();

      if (!options)
        options = {};

      var self = this;

      var wasAlreadyInThisGroupItem = false;
      var oldIndex = this._children.indexOf(item);
      if (oldIndex != -1) {
        this._children.splice(oldIndex, 1);
        wasAlreadyInThisGroupItem = true;
      }

      
      var index = ("index" in options) ? options.index : this._children.length;
      this._children.splice(index, 0, item);

      item.setZ(this.getZ() + 1);
      $el.addClass("tabInGroupItem");

      if (!wasAlreadyInThisGroupItem) {
        item.droppable(false);
        item.groupItemData = {};

        item.addSubscriber(this, "close", function() {
          let count = self._children.length;
          let dontArrange = self.expanded || !self.shouldStack(count);
          let dontClose = !item.closedManually && gBrowser._numPinnedTabs > 0;
          self.remove(item, {dontArrange: dontArrange, dontClose: dontClose});

          if (dontArrange)
            self._freezeItemSize(count);

          if (self._children.length > 0 && self._activeTab)
            UI.setActive(self);
        });

        item.setParent(this);

        if (typeof item.setResizable == 'function')
          item.setResizable(false, options.immediately);

        
        if (iQ(item.container).hasClass("focus"))
          this.setActiveTab(item);

        
        
        if (item.tab == gBrowser.selectedTab ||
            (!GroupItems.getActiveGroupItem() && !item.tab.hidden))
          UI.setActive(this);
      }

      if (!options.dontArrange)
        this.arrange({animate: !options.immediately});

      this._unfreezeItemSize({dontArrange: true});
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

      if (!options)
        options = {};

      var index = this._children.indexOf(item);
      if (index != -1)
        this._children.splice(index, 1);

      if (item == this._activeTab || !this._activeTab) {
        if (this._children.length > 0)
          this._activeTab = this._children[0];
        else
          this._activeTab = null;
      }

      item.setParent(null);
      item.removeClass("tabInGroupItem");
      item.removeClass("stacked");
      item.isStacked = false;
      item.setHidden(false);
      item.removeClass("stack-trayed");
      item.setRotation(0);

      
      
      
      if (item.isDragging && this.isStacked())
        item.setBounds(item.getBounds(), true, {force: true});

      item.droppable(true);
      item.removeSubscriber(this, "close");

      if (typeof item.setResizable == 'function')
        item.setResizable(true, options.immediately);

      
      
      if (item.tab._tabViewTabIsRemovedAfterRestore)
        options.dontClose = true;

      let closed = options.dontClose ? false : this.closeIfEmpty();
      if (closed)
        this._makeClosestTabActive();
      else if (!options.dontArrange) {
        this.arrange({animate: !options.immediately});
        this._unfreezeItemSize({dontArrange: true});
      }

      this._sendToSubscribers("childRemoved",{ groupItemId: this.id, item: item });
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  
  removeAll: function GroupItem_removeAll(options) {
    let self = this;
    let newOptions = {dontArrange: true};
    if (options)
      Utils.extend(newOptions, options);
      
    let toRemove = this._children.concat();
    toRemove.forEach(function(child) {
      self.remove(child, newOptions);
    });
  },

  
  
  
  
  
  
  
  
  
  
  addAppTab: function GroupItem_addAppTab(xulTab, options) {
    let self = this;

    let iconUrl = GroupItems.getAppTabFavIconUrl(xulTab);
    let $appTab = iQ("<img>")
      .addClass("appTabIcon")
      .attr("src", iconUrl)
      .data("xulTab", xulTab)
      .appendTo(this.$appTabTray)
      .click(function(event) {
        if (!Utils.isLeftClick(event))
          return;

        UI.setActive(self, { dontSetActiveTabInGroup: true });
        UI.goToTab(iQ(this).data("xulTab"));
      });

    
    if (!options || !options.dontAdjustTray)
      this.adjustAppTabTray(true);
  },

  
  
  removeAppTab: function GroupItem_removeAppTab(xulTab) {
    
    iQ(".appTabIcon", this.$appTabTray).each(function(icon) {
      let $icon = iQ(icon);
      if ($icon.data("xulTab") != xulTab)
        return true;
        
      $icon.remove();
      return false;
    });
    
    
    this.adjustAppTabTray(true);
  },

  
  
  arrangeAppTab: function GroupItem_arrangeAppTab(xulTab) {
    let self = this;

    let elements = iQ(".appTabIcon", this.$appTabTray);
    let length = elements.length;

    elements.each(function(icon) {
      let $icon = iQ(icon);
      if ($icon.data("xulTab") != xulTab)
        return true;

      let targetIndex = xulTab._tPos;

      $icon.remove();
      if (targetIndex < (length - 1))
        self.$appTabTray[0].insertBefore(
          icon,
          iQ(".appTabIcon:nth-child(" + (targetIndex + 1) + ")", self.$appTabTray)[0]);
      else
        $icon.appendTo(self.$appTabTray);
      return false;
    });
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
          left: parentBB.width/2 - this.$expander.width()/2
        });
  },

  
  
  
  
  shouldStack: function GroupItem_shouldStack(count) {
    if (count <= 1)
      return false;

    var bb = this.getContentBounds();
    var options = {
      return: 'widthAndColumns',
      count: count || this._children.length,
      hideTitle: false
    };
    let arrObj = Items.arrange(this._children, bb, options);

    let shouldStack = arrObj.childWidth < TabItems.minTabWidth * 1.35;
    this._columns = shouldStack ? null : arrObj.columns;

    return shouldStack;
  },

  
  
  
  
  
  
  _freezeItemSize: function GroupItem__freezeItemSize(itemCount) {
    let data = this._frozenItemSizeData;

    if (!data.lastItemCount) {
      let self = this;
      data.lastItemCount = itemCount;

      
      data.onTabViewHidden = function () self._unfreezeItemSize();
      window.addEventListener('tabviewhidden', data.onTabViewHidden, false);

      
      
      if (self.expanded)
        return;

      
      data.onMouseMove = function (e) {
        let cursor = new Point(e.pageX, e.pageY);
        if (!self.bounds.contains(cursor))
          self._unfreezeItemSize();
      }
      iQ(window).mousemove(data.onMouseMove);
    }

    this.arrange({animate: true, count: data.lastItemCount});
  },

  
  
  
  
  
  
  
  
  
  _unfreezeItemSize: function GroupItem__unfreezeItemSize(options) {
    let data = this._frozenItemSizeData;
    if (!data.lastItemCount)
      return;

    if (!options || !options.dontArrange)
      this.arrange({animate: true});

    
    window.removeEventListener('tabviewhidden', data.onTabViewHidden, false);
    if (data.onMouseMove)
      iQ(window).unbind('mousemove', data.onMouseMove);

    
    this._frozenItemSizeData = {};
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  arrange: function GroupItem_arrange(options) {
    if (!options)
      options = {};

    let childrenToArrange = [];
    this._children.forEach(function(child) {
      if (child.isDragging)
        options.addTab = true;
      else
        childrenToArrange.push(child);
    });

    if (GroupItems._arrangePaused) {
      GroupItems.pushArrange(this, options);
      return false;
    }
    
    let shouldStack = this.shouldStack(childrenToArrange.length + (options.addTab ? 1 : 0));
    let box = this.getContentBounds();
    
    
    if (shouldStack && !this.expanded) {
      this.showExpandControl();
      this._stackArrange(childrenToArrange, box, options);
      return false;
    } else {
      this.hideExpandControl();
      
      return this._gridArrange(childrenToArrange, box, options);
    }
  },

  
  
  
  
  
  
  
  
  
  
  
  _stackArrange: function GroupItem__stackArrange(childrenToArrange, bb, options) {
    if (!options)
      options = {};
    var animate = "animate" in options ? options.animate : true;

    var count = childrenToArrange.length;
    if (!count)
      return;

    let itemAspect = TabItems.tabHeight / TabItems.tabWidth;
    let zIndex = this.getZ() + count + 1;
    let maxRotation = 35; 
    let scale = 0.7;
    let newTabsPad = 10;
    let bbAspect = bb.height / bb.width;
    let numInPile = 6;
    let angleDelta = 3.5; 

    
    let size;
    if (bbAspect > itemAspect) { 
      size = TabItems.calcValidSize(new Point(bb.width * scale, -1),
        {hideTitle:true});
     } else { 
      size = TabItems.calcValidSize(new Point(-1, bb.height * scale),
        {hideTitle:true});
     }

    
    
    var x = (bb.width - size.x) / 2;
    var y = Math.min(size.x, (bb.height - size.y) / 2);
    var box = new Rect(bb.left + x, bb.top + y, size.x, size.y);

    var self = this;
    var children = [];

    
    let topChild = this.getTopChild();
    let topChildPos = childrenToArrange.indexOf(topChild);
    if (topChildPos > 0) {
      childrenToArrange.splice(topChildPos, 1);
      childrenToArrange.unshift(topChild);
    }

    childrenToArrange.forEach(function GroupItem__stackArrange_order(child) {
      
      child.addClass("stacked");
      child.isStacked = true;
      if (numInPile-- > 0) {
        children.push(child);
      } else {
        child.setHidden(true);
      }
    });

    self._isStacked = true;

    let angleAccum = 0;
    children.forEach(function GroupItem__stackArrange_apply(child, index) {
      child.setZ(zIndex);
      zIndex--;

      
      
      child.setBounds(box, !animate || child.getHidden(), {force:true});
      child.setRotation((UI.rtl ? -1 : 1) * angleAccum);
      child.setHidden(false);
      angleAccum += angleDelta;
    });
  },
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  _gridArrange: function GroupItem__gridArrange(childrenToArrange, box, options) {
    let arrangeOptions;
    if (this.expanded) {
      
      box = new Rect(this.expanded.bounds);
      box.inset(8, 8);
      arrangeOptions = Utils.extend({}, options, {z: 99999});
    } else {
      this._isStacked = false;
      arrangeOptions = Utils.extend({}, options, {
        columns: this._columns
      });

      childrenToArrange.forEach(function(child) {
        child.removeClass("stacked");
        child.isStacked = false;
        child.setHidden(false);
      });
    }
  
    if (!childrenToArrange.length)
      return false;

    
    
    let result = Items.arrange(childrenToArrange, box, arrangeOptions);
    let {dropIndex, rects, columns} = result;
    if ("oldDropIndex" in options && options.oldDropIndex === dropIndex)
      return dropIndex;

    this._columns = columns;
    let index = 0;
    let self = this;
    childrenToArrange.forEach(function GroupItem_arrange_children_each(child, i) {
      
      
      
      if (self._dropSpaceActive && index === dropIndex)
        index++;
      child.setBounds(rects[index], !options.animate);
      child.setRotation(0);
      if (arrangeOptions.z)
        child.setZ(arrangeOptions.z);
      index++;
    });

    return dropIndex;
  },

  expand: function GroupItem_expand() {
    var self = this;
    
    UI.setActive(this.getTopChild());
    
    var startBounds = this.getChild(0).getBounds();
    var $tray = iQ("<div>").css({
      top: startBounds.top,
      left: startBounds.left,
      width: startBounds.width,
      height: startBounds.height,
      position: "absolute",
      zIndex: 99998
    }).appendTo("body");
    $tray[0].id = "expandedTray";

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
        easing: "tabviewBounce",
        complete: function GroupItem_expand_animate_complete() {
          self._sendToSubscribers("expanded");
        }
      })
      .addClass("overlay");

    this._children.forEach(function(child) {
      child.addClass("stack-trayed");
      child.setHidden(false);
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
      let self = this;
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
          complete: function GroupItem_collapse_animate_complete() {
            iQ(this).remove();
            self._sendToSubscribers("collapsed");
          }
        });

      this.expanded.$shield.remove();
      this.expanded = null;

      this._children.forEach(function(child) {
        child.removeClass("stack-trayed");
      });

      this.arrange({z: z + 2});
      this._unfreezeItemSize({dontArrange: true});
    }
  },

  
  
  
  _addHandlers: function GroupItem__addHandlers(container) {
    let self = this;

    
    container.mousedown(function(e) {
      if (!Utils.isLeftClick(e) || self.$titlebar[0] == e.target || 
          self.$titlebar.contains(e.target)) {
        self._lastClick = 0;
        self._lastClickPositions = null;
        return;
      }
      if (Date.now() - self._lastClick <= UI.DBLCLICK_INTERVAL &&
          (self._lastClickPositions.x - UI.DBLCLICK_OFFSET) <= e.clientX &&
          (self._lastClickPositions.x + UI.DBLCLICK_OFFSET) >= e.clientX &&
          (self._lastClickPositions.y - UI.DBLCLICK_OFFSET) <= e.clientY &&
          (self._lastClickPositions.y + UI.DBLCLICK_OFFSET) >= e.clientY) {
        self.newTab();
        self._lastClick = 0;
        self._lastClickPositions = null;
      } else {
        self._lastClick = Date.now();
        self._lastClickPositions = new Point(e.clientX, e.clientY);
      }
    });

    var dropIndex = false;
    var dropSpaceTimer = null;

    
    
    this._dropSpaceActive = false;

    this.dropOptions.over = function GroupItem_dropOptions_over(event) {
      iQ(this.container).addClass("acceptsDrop");
    };
    this.dropOptions.move = function GroupItem_dropOptions_move(event) {
      let oldDropIndex = dropIndex;
      let dropPos = drag.info.item.getBounds().center();
      let options = {dropPos: dropPos,
                     addTab: self._dropSpaceActive && drag.info.item.parent != self,
                     oldDropIndex: oldDropIndex};
      let newDropIndex = self.arrange(options);
      
      if (newDropIndex !== oldDropIndex) {
        dropIndex = newDropIndex;
        if (this._dropSpaceActive)
          return;
          
        if (dropSpaceTimer) {
          clearTimeout(dropSpaceTimer);
          dropSpaceTimer = null;
        }

        dropSpaceTimer = setTimeout(function GroupItem_arrange_evaluateDropSpace() {
          
          
          
          
          
          if (dropIndex === newDropIndex) {
            self._dropSpaceActive = true;
            dropIndex = self.arrange({dropPos: dropPos,
                                      addTab: drag.info.item.parent != self,
                                      animate: true});
          }
          dropSpaceTimer = null;
        }, 250);
      }
    };
    this.dropOptions.drop = function GroupItem_dropOptions_drop(event) {
      iQ(this.container).removeClass("acceptsDrop");
      let options = {};
      if (this._dropSpaceActive)
        this._dropSpaceActive = false;

      if (dropSpaceTimer) {
        clearTimeout(dropSpaceTimer);
        dropSpaceTimer = null;
        
        
        let dropPos = drag.info.item.getBounds().center();
        dropIndex = self.arrange({dropPos: dropPos,
                                  addTab: drag.info.item.parent != self,
                                  animate: true});
      }

      if (dropIndex !== false)
        options = {index: dropIndex};
      this.add(drag.info.$el, options);
      UI.setActive(this);
      dropIndex = false;
    };
    this.dropOptions.out = function GroupItem_dropOptions_out(event) {
      dropIndex = false;
      if (this._dropSpaceActive)
        this._dropSpaceActive = false;

      if (dropSpaceTimer) {
        clearTimeout(dropSpaceTimer);
        dropSpaceTimer = null;
      }
      self.arrange();
      var groupItem = drag.info.item.parent;
      if (groupItem)
        groupItem.remove(drag.info.$el, {dontClose: true});
      iQ(this.container).removeClass("acceptsDrop");
    }

    this.draggable();
    this.droppable(true);

    this.$expander.click(function() {
      self.expand();
    });
  },

  
  
  
  setResizable: function GroupItem_setResizable(value, immediately) {
    var self = this;

    this.resizeOptions.minWidth = GroupItems.minGroupWidth;
    this.resizeOptions.minHeight = GroupItems.minGroupHeight;

    let start = this.resizeOptions.start;
    this.resizeOptions.start = function (event) {
      start.call(self, event);
      self._unfreezeItemSize();
    }

    if (value) {
      immediately ? this.$resizer.show() : this.$resizer.fadeIn();
      this.resizable(true);
    } else {
      immediately ? this.$resizer.hide() : this.$resizer.fadeOut();
      this.resizable(false);
    }
  },

  
  
  
  newTab: function GroupItem_newTab(url) {
    UI.setActive(this, { dontSetActiveTabInGroup: true });
    let newTab = gBrowser.loadOneTab(url || "about:blank", {inBackground: true});

    
    
    
    newTab._tabViewTabItem.zoomIn(!url);
  },

  
  
  
  
  
  
  reorderTabItemsBasedOnTabOrder: function GroupItem_reorderTabItemsBasedOnTabOrder() {
    this._children.sort(function(a,b) a.tab._tPos - b.tab._tPos);

    this.arrange({animate: false});
    
  },

  
  
  
  reorderTabsBasedOnTabItemOrder: function GroupItem_reorderTabsBasedOnTabItemOrder() {
    let indices;
    let tabs = this._children.map(function (tabItem) tabItem.tab);

    tabs.forEach(function (tab, index) {
      if (!indices)
        indices = tabs.map(function (tab) tab._tPos);

      let start = index ? indices[index - 1] + 1 : 0;
      let end = index + 1 < indices.length ? indices[index + 1] - 1 : Infinity;
      let targetRange = new Range(start, end);

      if (!targetRange.contains(tab._tPos)) {
        gBrowser.moveTabTo(tab, start);
        indices = null;
      }
    });
  },

  
  
  
  getTopChild: function GroupItem_getTopChild() {
    if (!this.getChildren().length) {
      return null;
    }

    return this.getActiveTab() || this.getChild(0);
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
  _delayedModUpdates: [],
  _autoclosePaused: false,
  minGroupHeight: 110,
  minGroupWidth: 125,

  
  
  
  toString: function GroupItems_toString() {
    return "[GroupItems count=" + this.groupItems.length + "]";
  },

  
  
  init: function GroupItems_init() {
    let self = this;

    
    function handleAttrModified(xulTab) {
      self._handleAttrModified(xulTab);
    }

    
    function handleClose(xulTab) {
      let idx = self._delayedModUpdates.indexOf(xulTab);
      if (idx != -1)
        self._delayedModUpdates.splice(idx, 1);
    }

    AllTabs.register("attrModified", handleAttrModified);
    AllTabs.register("close", handleClose);
    this._cleanupFunctions.push(function() {
      AllTabs.unregister("attrModified", handleAttrModified);
      AllTabs.unregister("close", handleClose);
    });
  },

  
  
  uninit: function GroupItems_uninit() {
    
    this._cleanupFunctions.forEach(function(func) {
      func();
    });

    this._cleanupFunctions = [];

    
    this.groupItems = null;
  },

  
  
  
  newGroup: function GroupItems_newGroup() {
    let bounds = new Rect(20, 20, 250, 200);
    return new GroupItem([], {bounds: bounds, immediately: true});
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
    this._arrangePaused = false;
    for (let i = 0; i < this._arrangesPending.length; i++) {
      let g = this._arrangesPending[i];
      g.groupItem.arrange(g.options);
    }
    this._arrangesPending = [];
  },

  
  
  
  _handleAttrModified: function GroupItems__handleAttrModified(xulTab) {
    if (!UI.isTabViewVisible()) {
      if (this._delayedModUpdates.indexOf(xulTab) == -1) {
        this._delayedModUpdates.push(xulTab);
      }
    } else
      this._updateAppTabIcons(xulTab); 
  },

  
  
  
  
  flushAppTabUpdates: function GroupItems_flushAppTabUpdates() {
    let self = this;
    this._delayedModUpdates.forEach(function(xulTab) {
      self._updateAppTabIcons(xulTab);
    });
    this._delayedModUpdates = [];
  },

  
  
  
  _updateAppTabIcons: function GroupItems__updateAppTabIcons(xulTab) {
    if (xulTab.ownerDocument.defaultView != gWindow || !xulTab.pinned)
      return;

    let iconUrl = this.getAppTabFavIconUrl(xulTab);
    this.groupItems.forEach(function(groupItem) {
      iQ(".appTabIcon", groupItem.$appTabTray).each(function(icon) {
        let $icon = iQ(icon);
        if ($icon.data("xulTab") != xulTab)
          return true;

        if (iconUrl != $icon.attr("src"))
          $icon.attr("src", iconUrl);
        return false;
      });
    });
  },

  
  
  
  getAppTabFavIconUrl: function GroupItems__getAppTabFavIconUrl(xulTab) {
    let iconUrl;

    if (UI.shouldLoadFavIcon(xulTab.linkedBrowser))
      iconUrl = UI.getFavIconUrlForTab(xulTab);
    else
      iconUrl = gFavIconService.defaultFavicon.spec;

    return iconUrl;
  },

  
  
  
  addAppTab: function GroupItems_addAppTab(xulTab) {
    this.groupItems.forEach(function(groupItem) {
      groupItem.addAppTab(xulTab);
    });
    this.updateGroupCloseButtons();
  },

  
  
  
  removeAppTab: function GroupItems_removeAppTab(xulTab) {
    this.groupItems.forEach(function(groupItem) {
      groupItem.removeAppTab(xulTab);
    });
    this.updateGroupCloseButtons();
  },

  
  
  
  arrangeAppTab: function GroupItems_arrangeAppTab(xulTab) {
    this.groupItems.forEach(function(groupItem) {
      groupItem.arrangeAppTab(xulTab);
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
      gWindow,
      { nextID: this.nextID, activeGroupId: activeGroupId,
        totalNumber: this.groupItems.length });
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
          this.nextID = Math.max(this.nextID, groupItemsData.nextID);
        if (groupItemsData.activeGroupId)
          activeGroupId = groupItemsData.activeGroupId;
      }

      if (groupItemData) {
        var toClose = this.groupItems.concat();
        for (var id in groupItemData) {
          let data = groupItemData[id];
          if (this.groupItemStorageSanity(data)) {
            let groupItem = this.groupItem(data.id); 
            if (groupItem && !groupItem.hidden) {
              groupItem.userSize = data.userSize;
              groupItem.setTitle(data.title);
              groupItem.setBounds(data.bounds, true);
              
              let index = toClose.indexOf(groupItem);
              if (index != -1)
                toClose.splice(index, 1);
            } else {
              var options = {
                dontPush: true,
                immediately: true
              };
  
              new GroupItem([], Utils.extend({}, data, options));
            }
          }
        }

        toClose.forEach(function(groupItem) {
          
          
          
          groupItem.getChildren().forEach(function (tabItem) {
            if (tabItem.parent && tabItem.parent.hidden)
              iQ(tabItem.container).show();
            tabItem._reconnected = false;
            tabItem._reconnect();
          });
          groupItem.close({immediately: true});
        });
      }

      
      if (activeGroupId) {
        let activeGroupItem = this.groupItem(activeGroupId);
        if (activeGroupItem)
          UI.setActive(activeGroupItem);
      }

      this._inited = true;
      this._save(); 
    } catch(e) {
      Utils.log("error in recons: "+e);
    }
  },

  
  
  
  
  load: function GroupItems_load() {
    let groupItemsData = Storage.readGroupItemsData(gWindow);
    let groupItemData = Storage.readGroupItemData(gWindow);
    this.reconstitute(groupItemsData, groupItemData);
    
    return (groupItemsData && !Utils.isEmptyObject(groupItemsData));
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

    this._arrangesPending = this._arrangesPending.filter(function (pending) {
      return groupItem != pending.groupItem;
    });

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

  
  
  
  removeAll: function GroupItems_removeAll() {
    var toRemove = this.groupItems.concat();
    toRemove.forEach(function(groupItem) {
      groupItem.removeAll();
    });
  },

  
  
  
  newTab: function GroupItems_newTab(tabItem, options) {
    let activeGroupItem = this.getActiveGroupItem();

    
    
    
    
    
    
    
    
    

    if (activeGroupItem) {
      activeGroupItem.add(tabItem, options);
      return;
    }

    let orphanTabItem = UI.getActiveOrphanTab();
    if (!orphanTabItem) {
      let targetGroupItem;
      
      gBrowser.visibleTabs.some(function(tab) {
        if (!tab.pinned && tab != tabItem.tab) {
          if (tab._tabViewTabItem) {
            if (!tab._tabViewTabItem.parent) {
              
              
              orphanTabItem = tab._tabViewTabItem;
            } else if (!tab._tabViewTabItem.parent.hidden) {
              
              
              targetGroupItem = tab._tabViewTabItem.parent;
            }
          }
          return true;
        }
        return false;
      });

      let visibleGroupItems;
      if (!orphanTabItem) {
        if (targetGroupItem) {
          
          targetGroupItem.add(tabItem);
          UI.setActive(targetGroupItem);
          return;
        } else {
          
          visibleGroupItems = this.groupItems.filter(function(groupItem) {
            return (!groupItem.hidden);
          });
          if (visibleGroupItems.length > 0) {
            visibleGroupItems[0].add(tabItem);
            UI.setActive(visibleGroupItems[0]);
            return;
          }
        }
        let orphanedTabs = this.getOrphanedTabs();
        
        
        if (orphanedTabs.length > 0)
          orphanTabItem = orphanedTabs[0];
      }
    }

    
    let tabItems;
    let newGroupItemBounds;
    
    
    if (orphanTabItem && orphanTabItem.tab != tabItem.tab) {
      newGroupItemBounds = orphanTabItem.getBounds();
      tabItems = [orphanTabItem, tabItem];
    } else {
      tabItem.setPosition(60, 60, true);
      newGroupItemBounds = tabItem.getBounds();
      tabItems = [tabItem];
    }

    newGroupItemBounds.inset(-40,-40);
    let newGroupItem = new GroupItem(tabItems, { bounds: newGroupItemBounds });
    newGroupItem.snap();
    UI.setActive(newGroupItem);
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
    }

    this._activeGroupItem = groupItem;
    this._save();
  },

  
  
  
  
  _updateTabBar: function GroupItems__updateTabBar() {
    if (!window.UI)
      return; 

    let activeOrphanTab;
    if (!this._activeGroupItem) {
      activeOrphanTab = UI.getActiveOrphanTab();
      if (!activeOrphanTab) {
        Utils.assert(false, "There must be something to show in the tab bar!");
        return;
      }
    }

    let tabItems = this._activeGroupItem == null ?
      [activeOrphanTab] : this._activeGroupItem._children;
    gBrowser.showOnlyTheseTabs(tabItems.map(function(item) item.tab));
  },

  
  
  
  updateActiveGroupItemAndTabBar: function GroupItems_updateActiveGroupItemAndTabBar(tabItem) {
    Utils.assertThrow(tabItem && tabItem.isATabItem, "tabItem must be a TabItem");

    UI.setActive(tabItem);
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
    var activeOrphanTab = UI.getActiveOrphanTab();
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

    Utils.assertThrow(tab._tabViewTabItem, "tab must be linked to a TabItem");

    
    if (tab._tabViewTabItem.parent && tab._tabViewTabItem.parent.id == groupItemId)
      return;

    let shouldUpdateTabBar = false;
    let shouldShowTabView = false;
    let groupItem;

    
    if (gBrowser.selectedTab == tab) {
      if (gBrowser.visibleTabs.length > 1) {
        gBrowser._blurTab(tab);
        shouldUpdateTabBar = true;
      } else {
        shouldShowTabView = true;
      }
    } else {
      shouldUpdateTabBar = true
    }

    
    if (tab._tabViewTabItem.parent)
      tab._tabViewTabItem.parent.remove(tab._tabViewTabItem);

    
    if (groupItemId) {
      groupItem = GroupItems.groupItem(groupItemId);
      groupItem.add(tab._tabViewTabItem);
      UI.setReorderTabItemsOnShow(groupItem);
    } else {
      let pageBounds = Items.getPageBounds();
      pageBounds.inset(20, 20);

      let box = new Rect(pageBounds);
      box.width = 250;
      box.height = 200;

      new GroupItem([ tab._tabViewTabItem ], { bounds: box });
    }

    if (shouldUpdateTabBar)
      this._updateTabBar();
    else if (shouldShowTabView)
      UI.showTabView();
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
  },

  
  
  
  
  
  getUnclosableGroupItemId: function GroupItems_getUnclosableGroupItemId() {
    let unclosableGroupItemId = null;

    if (gBrowser._numPinnedTabs > 0) {
      let hiddenGroupItems = 
        this.groupItems.concat().filter(function(groupItem) {
          return !groupItem.hidden;
        });
      if (hiddenGroupItems.length == 1)
        unclosableGroupItemId = hiddenGroupItems[0].id;
    }

    return unclosableGroupItemId;
  },

  
  
  
  updateGroupCloseButtons: function GroupItems_updateGroupCloseButtons() {
    let unclosableGroupItemId = this.getUnclosableGroupItemId();

    if (unclosableGroupItemId) {
      let groupItem = this.groupItem(unclosableGroupItemId);

      if (groupItem) {
        groupItem.$closeButton.hide();
      }
    } else {
      this.groupItems.forEach(function(groupItem) {
        groupItem.$closeButton.show();
      });
    }
  },
  
  
  
  
  calcValidSize: function GroupItems_calcValidSize(size, options) {
    Utils.assert(Utils.isPoint(size), 'input is a Point');
    Utils.assert((size.x>0 || size.y>0) && (size.x!=0 && size.y!=0), 
      "dimensions are valid:"+size.x+","+size.y);
    return new Point(
      Math.max(size.x, GroupItems.minGroupWidth),
      Math.max(size.y, GroupItems.minGroupHeight));
  },

  
  
  
  
  
  pauseAutoclose: function GroupItems_pauseAutoclose() {
    this._autoclosePaused = true;
  },

  
  
  
  resumeAutoclose: function GroupItems_resumeAutoclose() {
    this._autoclosePaused = false;
  }
};
