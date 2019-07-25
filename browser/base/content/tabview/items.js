



























































function Item() {
  
  
  this.isAnItem = true;

  
  
  
  this.bounds = null;

  
  
  this.zIndex = 0;

  
  
  this.container = null;

  
  
  this.parent = null;

  
  
  
  this.userSize = null;

  
  
  
  
  
  
  
  
  this.dragOptions = null;

  
  
  
  
  
  
  
  
  this.dropOptions = null;

  
  
  
  
  
  
  
  
  
  
  this.resizeOptions = null;

  
  
  this.isDragging = false;
};

Item.prototype = {
  
  
  
  
  
  
  _init: function Item__init(container) {
    Utils.assert(typeof this.addSubscriber == 'function' && 
        typeof this.removeSubscriber == 'function' && 
        typeof this._sendToSubscribers == 'function',
        'Subclass must implement the Subscribable interface');
    Utils.assert(Utils.isDOMElement(container), 'container must be a DOM element');
    Utils.assert(typeof this.setBounds == 'function', 'Subclass must provide setBounds');
    Utils.assert(typeof this.setZ == 'function', 'Subclass must provide setZ');
    Utils.assert(typeof this.close == 'function', 'Subclass must provide close');
    Utils.assert(typeof this.save == 'function', 'Subclass must provide save');
    Utils.assert(Utils.isPoint(this.defaultSize), 'Subclass must provide defaultSize');
    Utils.assert(Utils.isRect(this.bounds), 'Subclass must provide bounds');

    this.container = container;
    this.$container = iQ(container);

    iQ(this.container).data('item', this);

    
    this.dragOptions = {
      cancelClass: 'close stackExpander',
      start: function(e, ui) {
        if (this.isAGroupItem) {
          UI.setActive(this);
          this._unfreezeItemSize();
        }
        
        else if (this.parent != null)
          this.parent._dropSpaceActive = true;
        drag.info = new Drag(this, e);
      },
      drag: function(e) {
        drag.info.drag(e);
      },
      stop: function() {
        drag.info.stop();
        drag.info = null;
        if (!this.isAGroupItem && !this.parent)
          gTabView.firstUseExperienced = true;
      },
      
      
      minDragDistance: 3
    };

    
    this.dropOptions = {
      over: function() {},
      out: function() {
        let groupItem = drag.info.item.parent;
        if (groupItem)
          groupItem.remove(drag.info.$el, {dontClose: true});
        iQ(this.container).removeClass("acceptsDrop");
      },
      drop: function(event) {
        iQ(this.container).removeClass("acceptsDrop");
      },
      
      
      
      accept: function dropAcceptFunction(item) {
        return (item && item.isATabItem && (!item.parent || !item.parent.expanded));
      }
    };

    
    var self = this;
    this.resizeOptions = {
      aspectRatio: self.keepProportional,
      minWidth: 90,
      minHeight: 90,
      start: function(e,ui) {
        if (this.isAGroupItem)
          UI.setActive(this);
        resize.info = new Drag(this, e);
      },
      resize: function(e,ui) {
        resize.info.snap(UI.rtl ? 'topright' : 'topleft', false, self.keepProportional);
      },
      stop: function() {
        self.setUserSize();
        self.pushAway();
        resize.info.stop();
        resize.info = null;
      }
    };
  },

  
  
  
  getBounds: function Item_getBounds() {
    Utils.assert(Utils.isRect(this.bounds), 'this.bounds should be a rect');
    return new Rect(this.bounds);
  },

  
  
  
  overlapsWithOtherItems: function Item_overlapsWithOtherItems() {
    var self = this;
    var items = Items.getTopLevelItems();
    var bounds = this.getBounds();
    return items.some(function(item) {
      if (item == self) 
        return false;
      var myBounds = item.getBounds();
      return myBounds.intersects(bounds);
    } );
  },

  
  
  
  
  
  
  
  
  
  setPosition: function Item_setPosition(left, top, immediately) {
    Utils.assert(Utils.isRect(this.bounds), 'this.bounds');
    this.setBounds(new Rect(left, top, this.bounds.width, this.bounds.height), immediately);
  },

  
  
  
  
  
  
  
  
  
  setSize: function Item_setSize(width, height, immediately) {
    Utils.assert(Utils.isRect(this.bounds), 'this.bounds');
    this.setBounds(new Rect(this.bounds.left, this.bounds.top, width, height), immediately);
  },

  
  
  
  setUserSize: function Item_setUserSize() {
    Utils.assert(Utils.isRect(this.bounds), 'this.bounds');
    this.userSize = new Point(this.bounds.width, this.bounds.height);
    this.save();
  },

  
  
  
  getZ: function Item_getZ() {
    return this.zIndex;
  },

  
  
  
  setRotation: function Item_setRotation(degrees) {
    var value = degrees ? "rotate(%deg)".replace(/%/, degrees) : null;
    iQ(this.container).css({"-moz-transform": value});
  },

  
  
  
  setParent: function Item_setParent(parent) {
    this.parent = parent;
    this.removeTrenches();
    this.save();
  },

  
  
  
  
  
  
  pushAway: function Item_pushAway(immediately) {
    var buffer = Math.floor(Items.defaultGutter / 2);

    var items = Items.getTopLevelItems();
    
    items.forEach(function pushAway_setupPushAwayData(item) {
      var data = {};
      data.bounds = item.getBounds();
      data.startBounds = new Rect(data.bounds);
      
      data.generation = Infinity;
      item.pushAwayData = data;
    });

    
    var itemsToPush = [this];
    this.pushAwayData.generation = 0;

    var pushOne = function Item_pushAway_pushOne(baseItem) {
      
      var baseData = baseItem.pushAwayData;
      var bb = new Rect(baseData.bounds);

      
      bb.inset(-buffer, -buffer);
      
      var bbc = bb.center();

      items.forEach(function Item_pushAway_pushOne_pushEach(item) {
        if (item == baseItem)
          return;

        var data = item.pushAwayData;
        
        
        if (data.generation <= baseData.generation)
          return;

        
        var bounds = data.bounds;
        var box = new Rect(bounds);
        box.inset(-buffer, -buffer);

        
        if (box.intersects(bb)) {

          

          
          var offset = new Point();
          
          var center = box.center();

          
          
          if (Math.abs(center.x - bbc.x) < Math.abs(center.y - bbc.y)) {
            
            if (center.y > bbc.y)
              offset.y = bb.bottom - box.top;
            else
              offset.y = bb.top - box.bottom;
          } else { 
            
            if (center.x > bbc.x)
              offset.x = bb.right - box.left;
            else
              offset.x = bb.left - box.right;
          }

          
          bounds.offset(offset);

          
          data.generation = baseData.generation + 1;
          
          data.pusher = baseItem;
          
          itemsToPush.push(item);
        }
      });
    };

    
    
    
    while (itemsToPush.length)
      pushOne(itemsToPush.shift());

    
    var pageBounds = Items.getSafeWindowBounds();
    items.forEach(function Item_pushAway_squish(item) {
      var data = item.pushAwayData;
      if (data.generation == 0)
        return;

      let apply = function Item_pushAway_squish_apply(item, posStep, posStep2, sizeStep) {
        var data = item.pushAwayData;
        if (data.generation == 0)
          return;

        var bounds = data.bounds;
        bounds.width -= sizeStep.x;
        bounds.height -= sizeStep.y;
        bounds.left += posStep.x;
        bounds.top += posStep.y;

        let validSize;
        if (item.isAGroupItem) {
          validSize = GroupItems.calcValidSize(
            new Point(bounds.width, bounds.height));
          bounds.width = validSize.x;
          bounds.height = validSize.y;
        } else {
          if (sizeStep.y > sizeStep.x) {
            validSize = TabItems.calcValidSize(new Point(-1, bounds.height));
            bounds.left += (bounds.width - validSize.x) / 2;
            bounds.width = validSize.x;
          } else {
            validSize = TabItems.calcValidSize(new Point(bounds.width, -1));
            bounds.top += (bounds.height - validSize.y) / 2;
            bounds.height = validSize.y;        
          }
        }

        var pusher = data.pusher;
        if (pusher) {
          var newPosStep = new Point(posStep.x + posStep2.x, posStep.y + posStep2.y);
          apply(pusher, newPosStep, posStep2, sizeStep);
        }
      }

      var bounds = data.bounds;
      var posStep = new Point();
      var posStep2 = new Point();
      var sizeStep = new Point();

      if (bounds.left < pageBounds.left) {
        posStep.x = pageBounds.left - bounds.left;
        sizeStep.x = posStep.x / data.generation;
        posStep2.x = -sizeStep.x;
      } else if (bounds.right > pageBounds.right) { 
        posStep.x = pageBounds.right - bounds.right;
        sizeStep.x = -posStep.x / data.generation;
        posStep.x += sizeStep.x;
        posStep2.x = sizeStep.x;
      }

      if (bounds.top < pageBounds.top) {
        posStep.y = pageBounds.top - bounds.top;
        sizeStep.y = posStep.y / data.generation;
        posStep2.y = -sizeStep.y;
      } else if (bounds.bottom > pageBounds.bottom) { 
        posStep.y = pageBounds.bottom - bounds.bottom;
        sizeStep.y = -posStep.y / data.generation;
        posStep.y += sizeStep.y;
        posStep2.y = sizeStep.y;
      }

      if (posStep.x || posStep.y || sizeStep.x || sizeStep.y)
        apply(item, posStep, posStep2, sizeStep);        
    });

    
    var pairs = [];
    items.forEach(function Item_pushAway_setupUnsquish(item) {
      var data = item.pushAwayData;
      pairs.push({
        item: item,
        bounds: data.bounds
      });
    });

    Items.unsquish(pairs);

    
    items.forEach(function Item_pushAway_setBounds(item) {
      var data = item.pushAwayData;
      var bounds = data.bounds;
      if (!bounds.equals(data.startBounds)) {
        item.setBounds(bounds, immediately);
      }
    });
  },

  
  
  
  setTrenches: function Item_setTrenches(rect) {
    if (this.parent !== null)
      return;

    if (!this.borderTrenches)
      this.borderTrenches = Trenches.registerWithItem(this,"border");

    var bT = this.borderTrenches;
    Trenches.getById(bT.left).setWithRect(rect);
    Trenches.getById(bT.right).setWithRect(rect);
    Trenches.getById(bT.top).setWithRect(rect);
    Trenches.getById(bT.bottom).setWithRect(rect);

    if (!this.guideTrenches)
      this.guideTrenches = Trenches.registerWithItem(this,"guide");

    var gT = this.guideTrenches;
    Trenches.getById(gT.left).setWithRect(rect);
    Trenches.getById(gT.right).setWithRect(rect);
    Trenches.getById(gT.top).setWithRect(rect);
    Trenches.getById(gT.bottom).setWithRect(rect);

  },

  
  
  
  removeTrenches: function Item_removeTrenches() {
    for (var edge in this.borderTrenches) {
      Trenches.unregister(this.borderTrenches[edge]); 
    }
    this.borderTrenches = null;
    for (var edge in this.guideTrenches) {
      Trenches.unregister(this.guideTrenches[edge]); 
    }
    this.guideTrenches = null;
  },

  
  
  
  
  
  
  snap: function Item_snap(immediately) {
    
    var defaultRadius = Trenches.defaultRadius;
    Trenches.defaultRadius = 2 * defaultRadius; 

    var event = {startPosition:{}}; 
    var FauxDragInfo = new Drag(this, event, true);
    
    FauxDragInfo.snap('none', false);
    FauxDragInfo.stop(immediately);

    Trenches.defaultRadius = defaultRadius;
  },

  
  
  
  draggable: function Item_draggable() {
    try {
      Utils.assert(this.dragOptions, 'dragOptions');

      var cancelClasses = [];
      if (typeof this.dragOptions.cancelClass == 'string')
        cancelClasses = this.dragOptions.cancelClass.split(' ');

      var self = this;
      var $container = iQ(this.container);
      var startMouse;
      var startPos;
      var startSent;
      var startEvent;
      var droppables;
      var dropTarget;

      
      let determineBestDropTarget = function (e, box) {
        
        var best = {
          dropTarget: null,
          score: 0
        };

        droppables.forEach(function(droppable) {
          var intersection = box.intersection(droppable.bounds);
          if (intersection && intersection.area() > best.score) {
            var possibleDropTarget = droppable.item;
            var accept = true;
            if (possibleDropTarget != dropTarget) {
              var dropOptions = possibleDropTarget.dropOptions;
              if (dropOptions && typeof dropOptions.accept == "function")
                accept = dropOptions.accept.apply(possibleDropTarget, [self]);
            }

            if (accept) {
              best.dropTarget = possibleDropTarget;
              best.score = intersection.area();
            }
          }
        });

        return best.dropTarget;
      }

      
      var handleMouseMove = function(e) {
        
        drag.lastMoveTime = Date.now();

        
        var mouse = new Point(e.pageX, e.pageY);
        if (!startSent) {
          if(Math.abs(mouse.x - startMouse.x) > self.dragOptions.minDragDistance ||
             Math.abs(mouse.y - startMouse.y) > self.dragOptions.minDragDistance) {
            if (typeof self.dragOptions.start == "function")
              self.dragOptions.start.apply(self,
                  [startEvent, {position: {left: startPos.x, top: startPos.y}}]);
            startSent = true;
          }
        }
        if (startSent) {
          
          var box = self.getBounds();
          box.left = startPos.x + (mouse.x - startMouse.x);
          box.top = startPos.y + (mouse.y - startMouse.y);
          self.setBounds(box, true);

          if (typeof self.dragOptions.drag == "function")
            self.dragOptions.drag.apply(self, [e]);

          let bestDropTarget = determineBestDropTarget(e, box);

          if (bestDropTarget != dropTarget) {
            var dropOptions;
            if (dropTarget) {
              dropOptions = dropTarget.dropOptions;
              if (dropOptions && typeof dropOptions.out == "function")
                dropOptions.out.apply(dropTarget, [e]);
            }

            dropTarget = bestDropTarget;

            if (dropTarget) {
              dropOptions = dropTarget.dropOptions;
              if (dropOptions && typeof dropOptions.over == "function")
                dropOptions.over.apply(dropTarget, [e]);
            }
          }
          if (dropTarget) {
            dropOptions = dropTarget.dropOptions;
            if (dropOptions && typeof dropOptions.move == "function")
              dropOptions.move.apply(dropTarget, [e]);
          }
        }

        e.preventDefault();
      };

      
      var handleMouseUp = function(e) {
        iQ(gWindow)
          .unbind('mousemove', handleMouseMove)
          .unbind('mouseup', handleMouseUp);

        if (startSent && dropTarget) {
          var dropOptions = dropTarget.dropOptions;
          if (dropOptions && typeof dropOptions.drop == "function")
            dropOptions.drop.apply(dropTarget, [e]);
        }

        if (startSent && typeof self.dragOptions.stop == "function")
          self.dragOptions.stop.apply(self, [e]);

        e.preventDefault();
      };

      
      $container.mousedown(function(e) {
        if (!Utils.isLeftClick(e))
          return;

        var cancel = false;
        var $target = iQ(e.target);
        cancelClasses.forEach(function(className) {
          if ($target.hasClass(className))
            cancel = true;
        });

        if (cancel) {
          e.preventDefault();
          return;
        }

        startMouse = new Point(e.pageX, e.pageY);
        let bounds = self.getBounds();
        startPos = bounds.position();
        startEvent = e;
        startSent = false;

        droppables = [];
        iQ('.iq-droppable').each(function(elem) {
          if (elem != self.container) {
            var item = Items.item(elem);
            droppables.push({
              item: item,
              bounds: item.getBounds()
            });
          }
        });

        dropTarget = determineBestDropTarget(e, bounds);

        iQ(gWindow)
          .mousemove(handleMouseMove)
          .mouseup(handleMouseUp);

        e.preventDefault();
      });
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  droppable: function Item_droppable(value) {
    try {
      var $container = iQ(this.container);
      if (value) {
        Utils.assert(this.dropOptions, 'dropOptions');
        $container.addClass('iq-droppable');
      } else
        $container.removeClass('iq-droppable');
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  resizable: function Item_resizable(value) {
    try {
      var $container = iQ(this.container);
      iQ('.iq-resizable-handle', $container).remove();

      if (!value) {
        $container.removeClass('iq-resizable');
      } else {
        Utils.assert(this.resizeOptions, 'resizeOptions');

        $container.addClass('iq-resizable');

        var self = this;
        var startMouse;
        var startSize;
        var startAspect;

        
        var handleMouseMove = function(e) {
          
          resize.lastMoveTime = Date.now();

          var mouse = new Point(e.pageX, e.pageY);
          var box = self.getBounds();
          if (UI.rtl) {
            var minWidth = (self.resizeOptions.minWidth || 0);
            var oldWidth = box.width;
            if (minWidth != oldWidth || mouse.x < startMouse.x) {
              box.width = Math.max(minWidth, startSize.x - (mouse.x - startMouse.x));
              box.left -= box.width - oldWidth;
            }
          } else {
            box.width = Math.max(self.resizeOptions.minWidth || 0, startSize.x + (mouse.x - startMouse.x));
          }
          box.height = Math.max(self.resizeOptions.minHeight || 0, startSize.y + (mouse.y - startMouse.y));

          if (self.resizeOptions.aspectRatio) {
            if (startAspect < 1)
              box.height = box.width * startAspect;
            else
              box.width = box.height / startAspect;
          }

          self.setBounds(box, true);

          if (typeof self.resizeOptions.resize == "function")
            self.resizeOptions.resize.apply(self, [e]);

          e.preventDefault();
          e.stopPropagation();
        };

        
        var handleMouseUp = function(e) {
          iQ(gWindow)
            .unbind('mousemove', handleMouseMove)
            .unbind('mouseup', handleMouseUp);

          if (typeof self.resizeOptions.stop == "function")
            self.resizeOptions.stop.apply(self, [e]);

          e.preventDefault();
          e.stopPropagation();
        };

        
        iQ('<div>')
          .addClass('iq-resizable-handle iq-resizable-se')
          .appendTo($container)
          .mousedown(function(e) {
            if (!Utils.isLeftClick(e))
              return;

            startMouse = new Point(e.pageX, e.pageY);
            startSize = self.getBounds().size();
            startAspect = startSize.y / startSize.x;

            if (typeof self.resizeOptions.start == "function")
              self.resizeOptions.start.apply(self, [e]);

            iQ(gWindow)
              .mousemove(handleMouseMove)
              .mouseup(handleMouseUp);

            e.preventDefault();
            e.stopPropagation();
          });
        }
    } catch(e) {
      Utils.log(e);
    }
  }
};




let Items = {
  
  
  
  toString: function Items_toString() {
    return "[Items]";
  },

  
  
  
  defaultGutter: 15,

  
  
  
  item: function Items_item(el) {
    return iQ(el).data('item');
  },

  
  
  
  getTopLevelItems: function Items_getTopLevelItems() {
    var items = [];

    iQ('.tab, .groupItem, .info-item').each(function(elem) {
      var $this = iQ(elem);
      var item = $this.data('item');
      if (item && !item.parent && !$this.hasClass('phantom'))
        items.push(item);
    });

    return items;
  },

  
  
  
  getPageBounds: function Items_getPageBounds() {
    var width = Math.max(100, window.innerWidth);
    var height = Math.max(100, window.innerHeight);
    return new Rect(0, 0, width, height);
  },

  
  
  
  getSafeWindowBounds: function Items_getSafeWindowBounds() {
    
    var gutter = Items.defaultGutter;
    
    
    
    var topGutter = 5;
    return new Rect(gutter, topGutter,
        window.innerWidth - 2 * gutter, window.innerHeight - gutter - topGutter);

  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  arrange: function Items_arrange(items, bounds, options) {
    if (!options)
      options = {};
    var animate = "animate" in options ? options.animate : true;
    var immediately = !animate;

    var rects = [];

    var count = options.count || (items ? items.length : 0);
    if (options.addTab)
      count++;
    if (!count) {
      let dropIndex = (Utils.isPoint(options.dropPos)) ? 0 : null;
      return {rects: rects, dropIndex: dropIndex};
    }

    var columns = options.columns || 1;
    
    
    var itemMargin = items && items.length ?
                       parseInt(iQ(items[0].container).css('margin-left')) : 0;
    var padding = itemMargin * 2;
    var rows;
    var tabWidth;
    var tabHeight;
    var totalHeight;

    function figure() {
      rows = Math.ceil(count / columns);
      let validSize = TabItems.calcValidSize(
        new Point((bounds.width - (padding * columns)) / columns, -1),
        options);
      tabWidth = validSize.x;
      tabHeight = validSize.y;

      totalHeight = (tabHeight * rows) + (padding * rows);    
    }

    figure();

    while (rows > 1 && totalHeight > bounds.height) {
      columns++;
      figure();
    }

    if (rows == 1) {
      let validSize = TabItems.calcValidSize(new Point(tabWidth,
        bounds.height - 2 * itemMargin), options);
      tabWidth = validSize.x;
      tabHeight = validSize.y;
    }
    
    if (options.return == 'widthAndColumns')
      return {childWidth: tabWidth, columns: columns};

    let initialOffset = 0;
    if (UI.rtl) {
      initialOffset = bounds.width - tabWidth - padding;
    }
    var box = new Rect(bounds.left + initialOffset, bounds.top, tabWidth, tabHeight);

    var column = 0;

    var dropIndex = false;
    var dropRect = false;
    if (Utils.isPoint(options.dropPos))
      dropRect = new Rect(options.dropPos.x, options.dropPos.y, 1, 1);
    for (let a = 0; a < count; a++) {
      
      if (dropRect) {
        let activeBox = new Rect(box);
        activeBox.inset(-itemMargin - 1, -itemMargin - 1);
        
        
        if (activeBox.contains(dropRect))
          dropIndex = a;
      }
      
      
      rects.push(new Rect(box));

      box.left += (UI.rtl ? -1 : 1) * (box.width + padding);
      column++;
      if (column == columns) {
        box.left = bounds.left + initialOffset;
        box.top += box.height + padding;
        column = 0;
      }
    }

    return {rects: rects, dropIndex: dropIndex, columns: columns};
  },

  
  
  
  
  
  
  
  
  
  unsquish: function Items_unsquish(pairs, ignore) {
    var pairsProvided = (pairs ? true : false);
    if (!pairsProvided) {
      var items = Items.getTopLevelItems();
      pairs = [];
      items.forEach(function(item) {
        pairs.push({
          item: item,
          bounds: item.getBounds()
        });
      });
    }

    var pageBounds = Items.getSafeWindowBounds();
    pairs.forEach(function(pair) {
      var item = pair.item;
      if (item == ignore)
        return;

      var bounds = pair.bounds;
      var newBounds = new Rect(bounds);

      var newSize;
      if (Utils.isPoint(item.userSize))
        newSize = new Point(item.userSize);
      else if (item.isAGroupItem)
        newSize = GroupItems.calcValidSize(
          new Point(GroupItems.minGroupWidth, -1));
      else
        newSize = TabItems.calcValidSize(
          new Point(TabItems.tabWidth, -1));

      if (item.isAGroupItem) {
          newBounds.width = Math.max(newBounds.width, newSize.x);
          newBounds.height = Math.max(newBounds.height, newSize.y);
      } else {
        if (bounds.width < newSize.x) {
          newBounds.width = newSize.x;
          newBounds.height = newSize.y;
        }
      }

      newBounds.left -= (newBounds.width - bounds.width) / 2;
      newBounds.top -= (newBounds.height - bounds.height) / 2;

      var offset = new Point();
      if (newBounds.left < pageBounds.left)
        offset.x = pageBounds.left - newBounds.left;
      else if (newBounds.right > pageBounds.right)
        offset.x = pageBounds.right - newBounds.right;

      if (newBounds.top < pageBounds.top)
        offset.y = pageBounds.top - newBounds.top;
      else if (newBounds.bottom > pageBounds.bottom)
        offset.y = pageBounds.bottom - newBounds.bottom;

      newBounds.offset(offset);

      if (!bounds.equals(newBounds)) {
        var blocked = false;
        pairs.forEach(function(pair2) {
          if (pair2 == pair || pair2.item == ignore)
            return;

          var bounds2 = pair2.bounds;
          if (bounds2.intersects(newBounds))
            blocked = true;
          return;
        });

        if (!blocked) {
          pair.bounds.copy(newBounds);
        }
      }
      return;
    });

    if (!pairsProvided) {
      pairs.forEach(function(pair) {
        pair.item.setBounds(pair.bounds);
      });
    }
  }
};
