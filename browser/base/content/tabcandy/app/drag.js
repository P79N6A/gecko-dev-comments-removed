









































var drag = {
  info: null,
  zIndex: 100
};















var Drag = function(item, event, isResizing) {
  try {
    Utils.assert('must be an item, or at least a faux item',
                 item && (item.isAnItem || item.isAFauxItem));
    
    this.isResizing = isResizing || false;
    this.item = item;
    this.el = item.container;
    this.$el = iQ(this.el);
    this.parent = this.item.parent;
    this.startPosition = new Point(event.clientX, event.clientY);
    this.startTime = Utils.getMilliseconds();
    
    this.item.isDragging = true;
    this.item.setZ(999999);
    
    if (this.item.isATabItem && !isResizing)
      this.safeWindowBounds = Items.getSafeWindowBounds( true );
    else
      this.safeWindowBounds = Items.getSafeWindowBounds( );

    Trenches.activateOthersTrenches(this.el);
    
    
    if (this.item.isAGroup) {
      var tab = Page.getActiveTab();
      if (!tab || tab.parent != this.item) {
        if (this.item._children.length)
          Page.setActiveTab(this.item._children[0]);
      }
    } else if (this.item.isATabItem) {
      Page.setActiveTab(this.item);
    }
  } catch(e) {
    Utils.log(e);
  }
};

Drag.prototype = {
  
  
  
  
  
  
  
  
  
  
  
  
  snap: function(stationaryCorner, assumeConstantSize, keepProportional) {
    var stationaryCorner = stationaryCorner || 'topleft';
    var bounds = this.item.getBounds();
    var update = false; 
    var updateX = false;
    var updateY = false;
    var newRect;
    var snappedTrenches = {};

    
    if ( 
         !Keys.meta
         
         && !(this.item.isATabItem && this.item.overlapsWithOtherItems()) ) { 
      newRect = Trenches.snap(bounds,stationaryCorner,assumeConstantSize,keepProportional);
      if (newRect) { 
        update = true;
        snappedTrenches = newRect.snappedTrenches || {};
        bounds = newRect;
      }
    }

    
    newRect = this.snapToEdge(bounds,stationaryCorner,assumeConstantSize,keepProportional);
    if (newRect) {
      update = true;
      bounds = newRect;
      iQ.extend(snappedTrenches,newRect.snappedTrenches);
    }

    Trenches.hideGuides();
    for (let edge in snappedTrenches) {
      let trench = snappedTrenches[edge];
      if (typeof trench == 'object') {
        trench.showGuide = true;
        trench.show();
      } else if (trench === 'edge') {
        
      }
    }

    if (update)
      this.item.setBounds(bounds,true);
  },
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  snapToEdge: function Drag_snapToEdge(rect, stationaryCorner, assumeConstantSize, keepProportional) {
  
    var swb = this.safeWindowBounds;
    var update = false;
    var updateX = false;
    var updateY = false;
    var snappedTrenches = {};

    var snapRadius = ( Keys.meta ? 0 : Trenches.defaultRadius );
    if (rect.left < swb.left + snapRadius ) {
      if (stationaryCorner.indexOf('right') > -1)
        rect.width = rect.right - swb.left;
      rect.left = swb.left;
      update = true;
      updateX = true;
      snappedTrenches.left = 'edge';
    }
    
    if (rect.right > swb.right - snapRadius) {
      if (updateX || !assumeConstantSize) {
        var newWidth = swb.right - rect.left;
        if (keepProportional)
          rect.height = rect.height * newWidth / rect.width;
        rect.width = newWidth;
        update = true;
      } else if (!updateX || !Trenches.preferLeft) {
        rect.left = swb.right - rect.width;
        update = true;
      }
      snappedTrenches.right = 'edge';
      delete snappedTrenches.left;
    }
    if (rect.top < swb.top + snapRadius) {
      if (stationaryCorner.indexOf('bottom') > -1)
        rect.height = rect.bottom - swb.top;
      rect.top = swb.top;
      update = true;
      updateY = true;
      snappedTrenches.top = 'edge';
    }
    if (rect.bottom > swb.bottom - snapRadius) {
      if (updateY || !assumeConstantSize) {
        var newHeight = swb.bottom - rect.top;
        if (keepProportional)
          rect.width = rect.width * newHeight / rect.height;
        rect.height = newHeight;
        update = true;
      } else if (!updateY || !Trenches.preferTop) {
        rect.top = swb.bottom - rect.height;
        update = true;
      }
      snappedTrenches.top = 'edge';
      delete snappedTrenches.bottom;
    }
    
    if (update) {
      rect.snappedTrenches = snappedTrenches;
      return rect;
    }
    return false;
  },
  
  
  
  
  drag: function(event, ui) {
    this.snap('topleft',true);
      
    if (this.parent && this.parent.expanded) {
      var now = Utils.getMilliseconds();
      var distance = this.startPosition.distance(new Point(event.clientX, event.clientY));
      if (distance > 100) {
        this.parent.remove(this.item);
        this.parent.collapse();
      }
    }
  },

  
  
  
  stop: function() {
    Trenches.hideGuides();
    this.item.isDragging = false;

    if (this.parent && !this.parent.locked.close && this.parent != this.item.parent 
        && this.parent.isEmpty()) {
      this.parent.close();
    }
     
    if (this.parent && this.parent.expanded)
      this.parent.arrange();
      
    if (this.item && !this.item.parent) {
      this.item.setZ(drag.zIndex);
      drag.zIndex++;
      
      this.item.pushAway();
    }
    
    Trenches.disactivate();
  }
};
