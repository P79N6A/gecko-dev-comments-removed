



























window.Item = function() {
  
  
  this.isAnItem = true;
  
  
  
  this.bounds = null;
  
  
  
  
  this.debug = false;
  
  
  
  this.$debug = null;
  
  
  
  this.container = null;
  
  
  
  this.locked = false;
};

window.Item.prototype = { 
  
  
  
  
  
  
  _init: function(container) {
    this.container = container;
    
    if(this.debug) {
      this.$debug = $('<div />')
        .css({
          border: '2px solid green',
          zIndex: -10,
          position: 'absolute'
        })
        .appendTo($('body'));
    }
    
    this.reloadBounds();        
    $(this.container).data('item', this);
  },
  
  
  
  
  getBounds: function() {
    return new Rect(this.bounds);    
  },
  
  
  
  
  
  
  
  
  
  
  setPosition: function(left, top, immediately) {
    this.setBounds(new Rect(left, top, this.bounds.width, this.bounds.height), immediately);
  },

  
  
  
  
  
  
  
  
  
  setSize: function(width, height, immediately) {
    this.setBounds(new Rect(this.bounds.left, this.bounds.top, width, height), immediately);
  },

  
  
  
  getZ: function() {
    return parseInt($(this.container).css('zIndex'));
  },
    
  
  
  
  pushAway: function() {
    var buffer = 10;
    
    var items = Items.getTopLevelItems();
    $.each(items, function(index, item) {
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
    
      $.each(items, function(index, item) {
        if(item == baseItem || item.locked)
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

    
    if(false) {
      var pageBounds = Items.getPageBounds();
      $.each(items, function(index, item) {
        var data = item.pushAwayData;
        if(data.generation == 0 || item.locked)
          return;
  
        function apply(item, postStep, posStep2, sizeStep) {
          var data = item.pushAwayData;
          if(data.generation == 0)
            return;
            
          var bounds = data.bounds;
          bounds.width -= sizeStep.x; 
          bounds.height -= sizeStep.y;
          bounds.left += posStep.x;
          bounds.top += posStep.y;
          
          if(sizeStep.y > sizeStep.x) {
            var newWidth = bounds.height * (TabItems.tabWidth / TabItems.tabHeight);
            bounds.left += (bounds.width - newWidth) / 2;
            bounds.width = newWidth;
          } else {
            var newHeight = bounds.width * (TabItems.tabHeight / TabItems.tabWidth);
            bounds.top += (bounds.height - newHeight) / 2;
            bounds.height = newHeight;
          }
          
          var pusher = data.pusher;
          if(pusher)  
            apply(pusher, posStep.plus(posStep2), posStep2, sizeStep);
        }
  
        var bounds = data.bounds;
        var posStep = new Point();
        var posStep2 = new Point();
        var sizeStep = new Point();
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
    }

    
    $.each(items, function(index, item) {
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
  }  
};  




window.Items = {
  
  
  
  item: function(el) {
    return $(el).data('item');
  },
  
  
  
  
  getTopLevelItems: function() {
    var items = [];
    
    $('.tab, .group').each(function() {
      $this = $(this);
      if(!$this.data('group'))
        items.push($this.data('item'));
    });
    
    return items;
  }, 

  
  
  
  getPageBounds: function() {
    var top = 20;
    var bottom = TabItems.tabHeight + 10; 
    return new Rect(0, top, window.innerWidth, window.innerHeight - (top + bottom));
  },
  
  
  
  
  
  
  
  
  
  
  arrange: function(items, bounds, options) {
    var animate;
    if(!options || typeof(options.animate) == 'undefined') 
      animate = true;
    else 
      animate = options.animate;

    if(typeof(options) == 'undefined')
      options = {};
    
    var tabAspect = TabItems.tabHeight / TabItems.tabWidth;
    var count = items.length;
    if(!count)
      return;
      
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
      tabWidth = Math.min(bounds.width / count, bounds.height / tabAspect);
      tabHeight = tabWidth * tabAspect;
    }
    
    var box = new Rect(bounds.left, bounds.top, tabWidth, tabHeight);
    var row = 0;
    var column = 0;
    var immediately;
    
    $.each(items, function(index, item) {





        immediately = !animate;
        
      item.setBounds(box, immediately);




      
      box.left += box.width + padding;
      column++;
      if(column == columns) {
        box.left = bounds.left;
        box.top += (box.height * yScale) + padding;
        column = 0;
        row++;
      }
    });
  }
};

