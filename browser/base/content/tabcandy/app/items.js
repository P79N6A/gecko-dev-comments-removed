


























































window.Item = function() {
  
  
  this.isAnItem = true;
  
  
  
  this.bounds = null;
  
  
  
  
  this.debug = false;
  
  
  
  this.$debug = null;
  
  
  
  this.container = null;
  
  
  
  
  
  
  
  
  this.locked = null;
  
  
  
  this.parent = null;
  
  
  
  
  this.userSize = null;
  
  
  
  
  
  
  
  
  
  this.dragOptions = null;
  
  
  
  
  
  
  
  
  
  this.dropOptions = null;
  
  
  
  
  
  
  
  
  
  
  
  this.resizeOptions = null;
  
  
  
  this.isDragging = false;
};

window.Item.prototype = { 
  
  
  
  
  
  
  _init: function(container) {
    Utils.assert('container must be a DOM element', Utils.isDOMElement(container));
    Utils.assert('Subclass must provide reloadBounds', typeof(this.reloadBounds) == 'function');
    Utils.assert('Subclass must provide setBounds', typeof(this.setBounds) == 'function');
    Utils.assert('Subclass must provide setZ', typeof(this.setZ) == 'function');
    Utils.assert('Subclass must provide close', typeof(this.close) == 'function');
    Utils.assert('Subclass must provide addOnClose', typeof(this.addOnClose) == 'function');
    Utils.assert('Subclass must provide removeOnClose', typeof(this.removeOnClose) == 'function');
    Utils.assert('Subclass must provide save', typeof(this.save) == 'function');
    Utils.assert('Subclass must provide defaultSize', isPoint(this.defaultSize));
    Utils.assert('Subclass must provide locked', this.locked);
    
    this.container = container;
    
    if(this.debug) {
      this.$debug = iQ('<div>')
        .css({
          border: '2px solid green',
          zIndex: -10,
          position: 'absolute'
        })
        .appendTo('body');
    }
    
    this.reloadBounds();        
    Utils.assert('reloadBounds must set up this.bounds', this.bounds);

    iQ(this.container).data('item', this);

    
    this.dragOptions = {
      cancelClass: 'close stackExpander',
      start: function(e, ui) {
        drag.info = new Drag(this, e);
      },
      drag: function(e, ui) {
        drag.info.drag(e, ui);
      },
      stop: function() {
        drag.info.stop();
        drag.info = null;
      }
    };
    
    
    this.dropOptions = {
  		over: function(){},
  		out: function(){
  			var group = drag.info.item.parent;
  			if(group) {
  				group.remove(drag.info.$el, {dontClose: true});
  			}
  				
  			iQ(this.container).removeClass("acceptsDrop");
  		},
  		drop: function(event){
  			iQ(this.container).removeClass("acceptsDrop");
  		},
  		
  		
  		
  		accept: function dropAcceptFunction(item) {
				return (item && item.isATabItem && (!item.parent || !item.parent.expanded));
  		}
  	};
  	
  	
  	var self = this;
  	var resizeInfo = null;
    this.resizeOptions = {
      aspectRatio: self.keepProportional,
      minWidth: 90,
      minHeight: 90,
      start: function(e,ui){
      	resizeInfo = new Drag(this, e, true); 
      },
      resize: function(e,ui){
        resizeInfo.snap(e,ui, false, self.keepProportional);
      },
      stop: function(){
        self.setUserSize();
        self.pushAway();
        resizeInfo.stop();
        resizeInfo = null;
      } 
    };
  	
  },
  
  
  
  
  getBounds: function() {
    Utils.assert('this.bounds', isRect(this.bounds));
    return new Rect(this.bounds);    
  },

  
  
  
  overlapsWithOtherItems: function() {
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
  
  
  
  
  
  
  
  
  
  
  setPosition: function(left, top, immediately) {
    Utils.assert('this.bounds', isRect(this.bounds));
    this.setBounds(new Rect(left, top, this.bounds.width, this.bounds.height), immediately);
  },

  
  
  
  
  
  
  
  
  
  setSize: function(width, height, immediately) {
    Utils.assert('this.bounds', isRect(this.bounds));
    this.setBounds(new Rect(this.bounds.left, this.bounds.top, width, height), immediately);
  },

  
  
  
  setUserSize: function() {
    Utils.assert('this.bounds', isRect(this.bounds));
    this.userSize = new Point(this.bounds.width, this.bounds.height);
    this.save();
  },
  
  
  
  
  getZ: function() {
    return parseInt(iQ(this.container).css('zIndex'));
  },

  
  
  
  setRotation: function(degrees) {
    var value = "rotate(%deg)".replace(/%/, degrees);
    iQ(this.container).css({"-moz-transform": value});
  },
    
  
  
  
  setParent: function(parent) {
    this.parent = parent;
    this.removeTrenches();
    this.save();
  },

  
  
  
  pushAway: function() {
    var buffer = Math.floor( Items.defaultGutter / 2 );
    
    var items = Items.getTopLevelItems();
		
    iQ.each(items, function pushAway_setupPushAwayData(index, item) {
      var data = {};
      data.bounds = item.getBounds();
      data.startBounds = new Rect(data.bounds);
			
      data.generation = Infinity;
      item.pushAwayData = data;
    });
    
    
    var itemsToPush = [this];
    this.pushAwayData.generation = 0;

    var pushOne = function(baseItem) {
    	
      var baseData = baseItem.pushAwayData;
      var bb = new Rect(baseData.bounds);

			
      bb.inset(-buffer, -buffer);
			
      var bbc = bb.center();
    
      iQ.each(items, function(index, item) {
        if(item == baseItem || item.locked.bounds)
          return;
          
        var data = item.pushAwayData;
        
        
        if(data.generation <= baseData.generation)
          return;
        
        
        var bounds = data.bounds;
        var box = new Rect(bounds);
        box.inset(-buffer, -buffer);
        
        
        if(box.intersects(bb)) {
        
        	
        	
        	
          var offset = new Point();
          
          var center = box.center();
          
          
          
          if(Math.abs(center.x - bbc.x) < Math.abs(center.y - bbc.y)) {
          	
            if(center.y > bbc.y)
              offset.y = bb.bottom - box.top; 
            else
              offset.y = bb.top - box.bottom;
          } else { 
          	
            if(center.x > bbc.x)
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
    
    
    
    
    while(itemsToPush.length)
      pushOne(itemsToPush.shift());         

    
    var pageBounds = Items.getSafeWindowBounds();
		iQ.each(items, function(index, item) {
			var data = item.pushAwayData;
			if(data.generation == 0 || item.locked.bounds)
				return;

			function apply(item, posStep, posStep2, sizeStep) {
				var data = item.pushAwayData;
				if(data.generation == 0)
					return;
					
				var bounds = data.bounds;
				bounds.width -= sizeStep.x; 
				bounds.height -= sizeStep.y;
				bounds.left += posStep.x;
				bounds.top += posStep.y;
				
				if(!item.isAGroup) {
					if(sizeStep.y > sizeStep.x) {
						var newWidth = bounds.height * (TabItems.tabWidth / TabItems.tabHeight);
						bounds.left += (bounds.width - newWidth) / 2;
						bounds.width = newWidth;
					} else {
						var newHeight = bounds.width * (TabItems.tabHeight / TabItems.tabWidth);
						bounds.top += (bounds.height - newHeight) / 2;
						bounds.height = newHeight;
					}
				}
				
				var pusher = data.pusher;
				if(pusher)  
					apply(pusher, posStep.plus(posStep2), posStep2, sizeStep);
			}

			var bounds = data.bounds;
			var posStep = new Point();
			var posStep2 = new Point();
			var sizeStep = new Point();

			if(bounds.left < pageBounds.left) {      
				posStep.x = pageBounds.left - bounds.left;
				sizeStep.x = posStep.x / data.generation;
				posStep2.x = -sizeStep.x;                
			} else if(bounds.right > pageBounds.right) {      
				posStep.x = pageBounds.right - bounds.right;
				sizeStep.x = -posStep.x / data.generation;
				posStep.x += sizeStep.x;
				posStep2.x = sizeStep.x;
			}

			if(bounds.top < pageBounds.top) {      
				posStep.y = pageBounds.top - bounds.top;
				sizeStep.y = posStep.y / data.generation;
				posStep2.y = -sizeStep.y;                
			} else if(bounds.bottom > pageBounds.bottom) {      
				posStep.y = pageBounds.bottom - bounds.bottom;
				sizeStep.y = -posStep.y / data.generation;
				posStep.y += sizeStep.y;
				posStep2.y = sizeStep.y;
			}

			if(posStep.x || posStep.y || sizeStep.x || sizeStep.y) 
				apply(item, posStep, posStep2, sizeStep);
		});

    
    var pairs = [];
    iQ.each(items, function(index, item) {
      var data = item.pushAwayData;
      pairs.push({
        item: item,
        bounds: data.bounds
      });
    });
    
    Items.unsquish(pairs);

    
    iQ.each(items, function(index, item) {
      var data = item.pushAwayData;
      var bounds = data.bounds;
      if(!bounds.equals(data.startBounds)) {
        item.setBounds(bounds);
      }
    });
  },
  
  
  
  
  
  _updateDebugBounds: function() {
    if(this.$debug) {
      this.$debug.css({
        left: this.bounds.left,
        top: this.bounds.top,
        width: this.bounds.width,
        height: this.bounds.height
      });
    }
  },
  
  setTrenches: function(rect) {

		if (this.parent !== null)
			return;

		var container = this.container;

		if (!this.borderTrenches) {
			var bT = this.borderTrenches = {};
			bT.left = Trenches.register(container,"x","border","left");
			bT.right = Trenches.register(container,"x","border","right");
			bT.top = Trenches.register(container,"y","border","top");
			bT.bottom = Trenches.register(container,"y","border","bottom");
		}
		var bT = this.borderTrenches;
		Trenches.getById(bT.left).setWithRect(rect);
		Trenches.getById(bT.right).setWithRect(rect);
		Trenches.getById(bT.top).setWithRect(rect);
		Trenches.getById(bT.bottom).setWithRect(rect);
				
		if (!this.guideTrenches) {
			var gT = this.guideTrenches = {};
			gT.left = Trenches.register(container,"x","guide","left");
			gT.right = Trenches.register(container,"x","guide","right");
			gT.top = Trenches.register(container,"y","guide","top");
			gT.bottom = Trenches.register(container,"y","guide","bottom");
		}
		var gT = this.guideTrenches;
		Trenches.getById(gT.left).setWithRect(rect);
		Trenches.getById(gT.right).setWithRect(rect);
		Trenches.getById(gT.top).setWithRect(rect);
		Trenches.getById(gT.bottom).setWithRect(rect);

  },
  removeTrenches: function() {
		for (let edge in this.borderTrenches) {
			Trenches.unregister(this.borderTrenches[edge]); 
		}
		this.borderTrenches = null;
		for (let edge in this.guideTrenches) {
			Trenches.unregister(this.guideTrenches[edge]); 
		}
		this.guideTrenches = null;
  },
  
  
  
  
  draggable: function() {
    try {
      Utils.assert('dragOptions', this.dragOptions);
        
      var cancelClasses = [];
      if(typeof(this.dragOptions.cancelClass) == 'string')
        cancelClasses = this.dragOptions.cancelClass.split(' ');
        
      var self = this;
      var $container = iQ(this.container);
      var startMouse;
      var startPos;
      var startSent;
      var startEvent;
      var droppables;
      var dropTarget;
      
      
      var handleMouseMove = function(e) {
        
        var mouse = new Point(e.pageX, e.pageY);
        var box = self.getBounds();
        box.left = startPos.x + (mouse.x - startMouse.x);
        box.top = startPos.y + (mouse.y - startMouse.y);
        
        self.setBounds(box, true);

        
        if(!startSent) {
          if(iQ.isFunction(self.dragOptions.start)) {
            self.dragOptions.start.apply(self, 
                [startEvent, {position: {left: startPos.x, top: startPos.y}}]);
          }
          
          startSent = true;
        }

        if(iQ.isFunction(self.dragOptions.drag))
          self.dragOptions.drag.apply(self, [e, {position: box.position()}]);
          
        
        var newDropTarget = null;
        iQ.each(droppables, function(index, droppable) {
          if(box.intersects(droppable.bounds)) {
            var possibleDropTarget = droppable.item;
            var accept = true;
            if(possibleDropTarget != dropTarget) {
              var dropOptions = possibleDropTarget.dropOptions;
              if(dropOptions && iQ.isFunction(dropOptions.accept))
                accept = dropOptions.accept.apply(possibleDropTarget, [self]);
            }
            
            if(accept) {
              newDropTarget = possibleDropTarget;
              return false;
            }
          }
        });

        if(newDropTarget != dropTarget) {
          var dropOptions;
          if(dropTarget) {
            dropOptions = dropTarget.dropOptions;
            if(dropOptions && iQ.isFunction(dropOptions.out))
              dropOptions.out.apply(dropTarget, [e]);
          }
          
          dropTarget = newDropTarget; 

          if(dropTarget) {
            dropOptions = dropTarget.dropOptions;
            if(dropOptions && iQ.isFunction(dropOptions.over))
              dropOptions.over.apply(dropTarget, [e]);
          }
        }
          
        e.preventDefault();
      };
        
      
      var handleMouseUp = function(e) {
        iQ(window)
          .unbind('mousemove', handleMouseMove)
          .unbind('mouseup', handleMouseUp);
          
        if(dropTarget) {
          var dropOptions = dropTarget.dropOptions;
          if(dropOptions && iQ.isFunction(dropOptions.drop))
            dropOptions.drop.apply(dropTarget, [e]);
        }

        if(startSent && iQ.isFunction(self.dragOptions.stop))
          self.dragOptions.stop.apply(self, [e]);
          
        e.preventDefault();    
      };
      
      
      $container.mousedown(function(e) {
        if(Utils.isRightClick(e))
          return;
        
        var cancel = false;
        var $target = iQ(e.target);
        iQ.each(cancelClasses, function(index, class) {
          if($target.hasClass(class)) {
            cancel = true;
            return false;
          }
        });
        
        if(cancel) {
          e.preventDefault();
          return;
        }
          
        startMouse = new Point(e.pageX, e.pageY);
        startPos = self.getBounds().position();
        startEvent = e;
        startSent = false;
        dropTarget = null;
        
        droppables = [];
        iQ('.iq-droppable').each(function() {
          if(this != self.container) {
            var item = Items.item(this);
            droppables.push({
              item: item, 
              bounds: item.getBounds()
            });
          }
        });

        iQ(window)
          .mousemove(handleMouseMove)
          .mouseup(handleMouseUp);          
                    
        e.preventDefault();
      });
    } catch(e) {
      Utils.log(e);
    }  
  },

  
  
  
  droppable: function(value) {
    try {
      var $container = iQ(this.container);
      if(value)
        $container.addClass('iq-droppable');
      else {
        Utils.assert('dropOptions', this.dropOptions);
        
        $container.removeClass('iq-droppable');
      }
    } catch(e) {
      Utils.log(e);
    }
  },
  
  
  
  
  resizable: function(value) {
    try {
      var $container = iQ(this.container);
      iQ('.iq-resizable-handle', $container).remove();

      if(!value) {
        $container.removeClass('iq-resizable');
      } else {
        Utils.assert('resizeOptions', this.resizeOptions);
        
        $container.addClass('iq-resizable');

        var self = this;
        var startMouse;
        var startSize;
        
        
        var handleMouseMove = function(e) {
          var mouse = new Point(e.pageX, e.pageY);
          var box = self.getBounds();
          box.width = Math.max(self.resizeOptions.minWidth || 0, startSize.x + (mouse.x - startMouse.x));
          box.height = Math.max(self.resizeOptions.minHeight || 0, startSize.y + (mouse.y - startMouse.y));

          if(self.resizeOptions.aspectRatio) {
            if(startAspect < 1)
              box.height = box.width * startAspect;
            else
              box.width = box.height / startAspect;
          }
                        
          self.setBounds(box, true);
  
          if(iQ.isFunction(self.resizeOptions.resize))
            self.resizeOptions.resize.apply(self, [e]);
            
          e.preventDefault();
          e.stopPropagation();
        };
          
        
        var handleMouseUp = function(e) {
          iQ(window)
            .unbind('mousemove', handleMouseMove)
            .unbind('mouseup', handleMouseUp);
            
          if(iQ.isFunction(self.resizeOptions.stop))
            self.resizeOptions.stop.apply(self, [e]);
            
          e.preventDefault();    
          e.stopPropagation();
        };
        
        
        iQ('<div>')
          .addClass('iq-resizable-handle iq-resizable-se')
          .appendTo($container)
          .mousedown(function(e) {
            if(Utils.isRightClick(e))
              return;
            
            startMouse = new Point(e.pageX, e.pageY);
            startSize = self.getBounds().size();
            startAspect = startSize.y / startSize.x;
            
						if(iQ.isFunction(self.resizeOptions.start))
							self.resizeOptions.start.apply(self, [e]);
            
            iQ(window)
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




window.Items = {
  
  
  
  defaultGutter: 15,
  
  
  
  
  init: function() {
  },
  
  
  
  
  item: function(el) {
    return iQ(el).data('item');
  },
  
  
  
  
  getTopLevelItems: function() {
    var items = [];
    
    iQ('.tab, .group').each(function() {
      var $this = iQ(this);
      var item = $this.data('item');  
      if(item && !item.parent && !$this.hasClass('phantom'))
        items.push(item);
    });
    
    return items;
  }, 

  
  
  
  getPageBounds: function() {
    var top = 0;
    var bottom = TabItems.tabHeight + 10; 
    var width = Math.max(100, window.innerWidth);
    var height = Math.max(100, window.innerHeight - (top + bottom));
    return new Rect(0, top, width, height);
  },
  
  
  
  
  getSafeWindowBounds: function( dontCountNewTabGroup ) {
    
    var gutter = Items.defaultGutter;
    var newTabGroupBounds = Groups.getBoundsForNewTabGroup();
    
    
    
    var topGutter = 5;
    if (dontCountNewTabGroup)
			return new Rect( gutter, topGutter, window.innerWidth - 2 * gutter, window.innerHeight - gutter - topGutter );
		else
			return new Rect( gutter, topGutter, window.innerWidth - 2 * gutter, newTabGroupBounds.top -  gutter - topGutter );

  },
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  arrange: function(items, bounds, options) {
    var animate;
    if(!options || typeof(options.animate) == 'undefined') 
      animate = true;
    else 
      animate = options.animate;

    if(typeof(options) == 'undefined')
      options = {};
    
    var rects = null;
    if(options.pretend)
      rects = [];
      
    var tabAspect = TabItems.tabHeight / TabItems.tabWidth;
    var count = options.count || (items ? items.length : 0);
    if(!count)
      return rects;
      
    var columns = 1;
    var padding = options.padding || 0;
    var yScale = 1.1; 
    var rows;
    var tabWidth;
    var tabHeight;
    var totalHeight;

    function figure() {
      rows = Math.ceil(count / columns);          
      tabWidth = (bounds.width - (padding * (columns - 1))) / columns;
      tabHeight = tabWidth * tabAspect; 
      totalHeight = (tabHeight * yScale * rows) + (padding * (rows - 1)); 
    } 
    
    figure();
    
    while(rows > 1 && totalHeight > bounds.height) {
      columns++; 
      figure();
    }
    
    if(rows == 1) {
      var maxWidth = Math.max(TabItems.tabWidth, bounds.width / 2);
      tabWidth = Math.min(Math.min(maxWidth, bounds.width / count), bounds.height / tabAspect);
      tabHeight = tabWidth * tabAspect;
    }
    
    var box = new Rect(bounds.left, bounds.top, tabWidth, tabHeight);
    var row = 0;
    var column = 0;
    var immediately;
    
    var a;
    for(a = 0; a < count; a++) {





        immediately = !animate;
        
      if(rects)
        rects.push(new Rect(box));
      else if(items && a < items.length) {
        var item = items[a];
        if(!item.locked.bounds) {
          item.setBounds(box, immediately);
          item.setRotation(0);
          if(options.z)
            item.setZ(options.z);
        }
      }
      




      
      box.left += box.width + padding;
      column++;
      if(column == columns) {
        box.left = bounds.left;
        box.top += (box.height * yScale) + padding;
        column = 0;
        row++;
      }
    }
    
    return rects;
  },
  
  
  
  
  
  
  
  
  
  
  unsquish: function(pairs, ignore) {
    var pairsProvided = (pairs ? true : false);
    if(!pairsProvided) {
      var items = Items.getTopLevelItems();
      pairs = [];
      iQ.each(items, function(index, item) {
        pairs.push({
          item: item,
          bounds: item.getBounds()
        });
      });
    }
  
    var pageBounds = Items.getSafeWindowBounds();
    iQ.each(pairs, function(index, pair) {
      var item = pair.item;
      if(item.locked.bounds || item == ignore)
        return;
        
      var bounds = pair.bounds;
      var newBounds = new Rect(bounds);

      var newSize;
      if(isPoint(item.userSize)) 
        newSize = new Point(item.userSize);
      else
        newSize = new Point(TabItems.tabWidth, TabItems.tabHeight);
        
      if(item.isAGroup) {
          newBounds.width = Math.max(newBounds.width, newSize.x);
          newBounds.height = Math.max(newBounds.height, newSize.y);
      } else {
        if(bounds.width < newSize.x) {
          newBounds.width = newSize.x;
          newBounds.height = newSize.y;
        }
      }

      newBounds.left -= (newBounds.width - bounds.width) / 2;
      newBounds.top -= (newBounds.height - bounds.height) / 2;
      
      var offset = new Point();
      if(newBounds.left < pageBounds.left)
        offset.x = pageBounds.left - newBounds.left;
      else if(newBounds.right > pageBounds.right)
        offset.x = pageBounds.right - newBounds.right;

      if(newBounds.top < pageBounds.top)
        offset.y = pageBounds.top - newBounds.top;
      else if(newBounds.bottom > pageBounds.bottom)
        offset.y = pageBounds.bottom - newBounds.bottom;
        
      newBounds.offset(offset);

      if(!bounds.equals(newBounds)) {        
        var blocked = false;
        iQ.each(pairs, function(index, pair2) {
          if(pair2 == pair || pair2.item == ignore)
            return;
            
          var bounds2 = pair2.bounds;
          if(bounds2.intersects(newBounds)) {
            blocked = true;
            return false;
          }
        });
        
        if(!blocked) {
          pair.bounds.copy(newBounds);
        }
      }
    });

    if(!pairsProvided) {
      iQ.each(pairs, function(index, pair) {
        pair.item.setBounds(pair.bounds);
      });
    }
  }
};

window.Items.init();

