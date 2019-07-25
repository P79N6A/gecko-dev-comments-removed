










































var drag = {
  info: null,
  zIndex: 100,
  lastMoveTime: 0
};




var resize = {
  info: null,
  lastMoveTime: 0
};














function Drag(item, event, isFauxDrag) {
  Utils.assert(item && (item.isAnItem || item.isAFauxItem), 
      'must be an item, or at least a faux item');

  this.item = item;
  this.el = item.container;
  this.$el = iQ(this.el);
  this.parent = this.item.parent;
  this.startPosition = new Point(event.clientX, event.clientY);
  this.startTime = Date.now();

  this.item.isDragging = true;
  this.item.setZ(999999);

  this.safeWindowBounds = Items.getSafeWindowBounds();

  Trenches.activateOthersTrenches(this.el);

  if (!isFauxDrag) {
    
    if (this.item.isAGroupItem) {
      var tab = UI.getActiveTab();
      if (!tab || tab.parent != this.item) {
        if (this.item._children.length)
          UI.setActive(this.item._children[0]);
      }
    } else if (this.item.isATabItem) {
      UI.setActive(this.item);
    }
  }
};

Drag.prototype = {
  
  
  
  toString: function Drag_toString() {
    return "[Drag (" + this.item + ")]";
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  snapBounds: function Drag_snapBounds(bounds, stationaryCorner, assumeConstantSize, keepProportional, checkItemStatus) {
    if (!stationaryCorner)
      stationaryCorner = UI.rtl ? 'topright' : 'topleft';
    var update = false; 
    var updateX = false;
    var updateY = false;
    var newRect;
    var snappedTrenches = {};

    

    
    if (!Keys.meta && !Trenches.disabled) {
      
      
      let snappable = !(this.item.isATabItem &&
                       this.item.overlapsWithOtherItems()) &&
                       !iQ(".acceptsDrop").length;
      if (!checkItemStatus || snappable) {
        newRect = Trenches.snap(bounds, stationaryCorner, assumeConstantSize,
                                keepProportional);
        if (newRect) { 
          update = true;
          snappedTrenches = newRect.snappedTrenches || {};
          bounds = newRect;
        }
      }
    }

    
    newRect = this.snapToEdge(bounds, stationaryCorner, assumeConstantSize,
                              keepProportional);
    if (newRect) {
      update = true;
      bounds = newRect;
      Utils.extend(snappedTrenches, newRect.snappedTrenches);
    }

    Trenches.hideGuides();
    for (var edge in snappedTrenches) {
      var trench = snappedTrenches[edge];
      if (typeof trench == 'object') {
        trench.showGuide = true;
        trench.show();
      }
    }

    return update ? bounds : false;
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  snap: function Drag_snap(stationaryCorner, assumeConstantSize, keepProportional) {
    var bounds = this.item.getBounds();
    bounds = this.snapBounds(bounds, stationaryCorner, assumeConstantSize, keepProportional, true);
    if (bounds) {
      this.item.setBounds(bounds, true);
      return true;
    }
    return false;
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  snapToEdge: function Drag_snapToEdge(rect, stationaryCorner, assumeConstantSize, keepProportional) {

    var swb = this.safeWindowBounds;
    var update = false;
    var updateX = false;
    var updateY = false;
    var snappedTrenches = {};

    var snapRadius = (Keys.meta ? 0 : Trenches.defaultRadius);
    if (rect.left < swb.left + snapRadius ) {
      if (stationaryCorner.indexOf('right') > -1 && !assumeConstantSize)
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
      if (stationaryCorner.indexOf('bottom') > -1 && !assumeConstantSize)
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

  
  
  
  drag: function Drag_drag(event) {
    this.snap(UI.rtl ? 'topright' : 'topleft', true);

    if (this.parent && this.parent.expanded) {
      var distance = this.startPosition.distance(new Point(event.clientX, event.clientY));
      if (distance > 100) {
        this.parent.remove(this.item);
        this.parent.collapse();
      }
    }
  },

  
  
  
  
  
  
  stop: function Drag_stop(immediately) {
    Trenches.hideGuides();
    this.item.isDragging = false;

    if (this.parent && this.parent != this.item.parent &&
       this.parent.isEmpty()) {
      this.parent.close();
    }

    if (this.parent && this.parent.expanded)
      this.parent.arrange();

    if (this.item.parent)
      this.item.parent.arrange();

    if (!this.item.parent) {
      this.item.setZ(drag.zIndex);
      drag.zIndex++;

      this.item.pushAway(immediately);
    }

    Trenches.disactivate();
  }
};
