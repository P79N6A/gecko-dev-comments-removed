



















window.Item = function() {
  
  
  this.isAnItem = true;
  
  
  
  this.bounds = null;
  
  
  
  
  this.debug = false;
  
  
  
  this.$debug = null;
  
  
  
  this.container = null;
  
  
  
  
  
  
  
  
  this.locked = null;
  
  
  
  this.parent = null;
  
  
  
  
  this.userSize = null;
  
  this.dragOptions = {
    cancelClass: 'close',
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
				
			iQ(this).removeClass("acceptsDrop");
		},
		drop: function(event){
			iQ(this).removeClass("acceptsDrop");
		},
		
		
		
		accept: function dropAcceptFunction(el) {
			var $el = iQ(el);
			if($el.hasClass('tab')) {
				var item = Items.item($el);
				if(item && (!item.parent || !item.parent.expanded)) {
					return true;
				}
			}
			return false;
		}
	};
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
  },
  
  
  
  
  getBounds: function() {
    Utils.assert('this.bounds', isRect(this.bounds));
    return new Rect(this.bounds);    
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
    Trenches.unregister(this.container);
    this.save();
  },

  
  
  
  pushAway: function() {
    var buffer = 2;
    
    var items = Items.getTopLevelItems();
    iQ.each(items, function(index, item) {
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

    
    var pageBounds = Items.getPageBounds();
    if(Items.squishMode == 'squish') {
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
    } else if(Items.squishMode == 'all') {
      var newPageBounds = null;
      iQ.each(items, function(index, item) {
        if(item.locked.bounds)
          return;
          
        var data = item.pushAwayData;
        var bounds = data.bounds;
        newPageBounds = (newPageBounds ? newPageBounds.union(bounds) : new Rect(bounds));
      });
      
      var wScale = pageBounds.width / newPageBounds.width;
      var hScale = pageBounds.height / newPageBounds.height;
      var scale = Math.min(hScale, wScale);
      iQ.each(items, function(index, item) {
        if(item.locked.bounds)
          return;
          
        var data = item.pushAwayData;
        var bounds = data.bounds;

        bounds.left -= newPageBounds.left;
        bounds.left *= scale;
        bounds.width *= scale;

        bounds.top -= newPageBounds.top;            
        bounds.top *= scale;
        bounds.height *= scale;
      });
    }

    
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

		var container = this.container;

		if (!this.borderTrenches) {
			var bT = this.borderTrenches = {};
			bT.left = Trenches.register(container,"x","border","left");
			bT.right = Trenches.register(container,"x","border","right");
			bT.top = Trenches.register(container,"y","border","top");
			bT.bottom = Trenches.register(container,"y","border","bottom");
		}
		var bT = this.borderTrenches;
		bT.left.setWithRect(rect);
		bT.right.setWithRect(rect);
		bT.top.setWithRect(rect);
		bT.bottom.setWithRect(rect);
				
		if (!this.guideTrenches) {
			var gT = this.guideTrenches = {};
			gT.left = Trenches.register(container,"x","guide","left");
			gT.right = Trenches.register(container,"x","guide","right");
			gT.top = Trenches.register(container,"y","guide","top");
			gT.bottom = Trenches.register(container,"y","guide","bottom");
		}
		var gT = this.guideTrenches;
		gT.left.setWithRect(rect);
		gT.right.setWithRect(rect);
		gT.top.setWithRect(rect);
		gT.bottom.setWithRect(rect);

  },
};  




window.Items = {
  
  
  
  
  squishMode: 'squish', 
  
  
  
  
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
      tabWidth = Math.min(Math.min(TabItems.tabWidth, bounds.width / count), bounds.height / tabAspect);
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
  
    var pageBounds = Items.getPageBounds();
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

