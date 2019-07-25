









































var drag = {
  info: null,
  zIndex: 100
};














var Drag = function(item, event) {
  try {
    Utils.assert('item', item && item.isAnItem);
    
    this.item = item;
    this.el = item.container;
    this.$el = iQ(this.el);
    this.parent = this.item.parent;
    this.startPosition = new Point(event.clientX, event.clientY);
    this.startTime = Utils.getMilliseconds();
    
    this.item.isDragging = true;
    this.item.setZ(999999);
    
    this.safeWindowBounds = Items.getSafeWindowBounds();
    Trenches.activateOthersTrenches(this.el);
    
    
    if(this.item.isAGroup) {
      var tab = Page.getActiveTab();
      if(!tab || tab.parent != this.item) {
        if(this.item._children.length)
          Page.setActiveTab(this.item._children[0]);
      }
    } else {
      Page.setActiveTab(this.item);
    }
  } catch(e) {
    Utils.log(e);
  }
};

Drag.prototype = {
  
  snap: function(event, ui, assumeConstantSize, keepProportional){
    var bounds = this.item.getBounds();
    var update = false; 
    var updateX = false;
    var updateY = false;
    var newRect;

    
    if ( !Keys.meta                           
        && !this.item.overlapsWithOtherItems() 
        ) { 
      newRect = Trenches.snap(bounds,assumeConstantSize,keepProportional);
      if (newRect) { 
        update = true;
        bounds = newRect;
      }
    }

    
    newRect = this.snapToEdge(bounds,assumeConstantSize,keepProportional);
    if (newRect) {
      update = true;
      bounds = newRect;
    }

    if (update)
      this.item.setBounds(bounds,true);
  },
  
  
  
  
  
  
  
  
  
  
  
  
  snapToEdge: function Drag_snapToEdge(rect, assumeConstantSize, keepProportional) {
    var swb = this.safeWindowBounds;
    var update = false;
    var updateX = false;
    var updateY = false;

    var snapRadius = ( Keys.meta ? 0 : Trenches.defaultRadius );
    if (rect.left < swb.left + snapRadius ) {
      rect.left = swb.left;
      update = true;
      updateX = true;
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
    }
    if (rect.top < swb.top + snapRadius) {
      rect.top = swb.top;
      update = true;
      updateY = true;
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
    }
    
    if (update)
      return rect;
    else
      return false;
  },
  
  
  
  
  drag: function(event, ui) {
    this.snap(event,ui,true);
      
    if(this.parent && this.parent.expanded) {
      var now = Utils.getMilliseconds();
      var distance = this.startPosition.distance(new Point(event.clientX, event.clientY));
      if(distance > 100) {
        this.parent.remove(this.item);
        this.parent.collapse();
      }
    }
  },

  
  
  
  stop: function() {
    this.item.isDragging = false;

    if(this.parent && !this.parent.locked.close && this.parent != this.item.parent 
        && this.parent._children.length == 0 && !this.parent.getTitle()) {
      this.parent.close();
    }
     
    if(this.parent && this.parent.expanded)
      this.parent.arrange();
      
    if(this.item && !this.item.parent) {
      this.item.setZ(drag.zIndex);
      drag.zIndex++;
      
      this.item.pushAway();
    }
    
    Trenches.disactivate();
  }
};
