









































var drag = {
  info: null,
  zIndex: 100
};









var Drag = function(element, event) {
  this.el = element;
  this.$el = iQ(this.el);
  this.item = Items.item(this.el);
  this.parent = this.item.parent;
  this.startPosition = new Point(event.clientX, event.clientY);
  this.startTime = Utils.getMilliseconds();
  
  this.item.isDragging = true;
  this.$el.data('isDragging', true);
  this.item.setZ(999999);
  
  this.safeWindowBounds = this.getSafeWindowBounds();
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
};

Drag.prototype = {
  
  snap: function(event, ui){
    var bounds = this.item.getBounds();
    var update = false; 
    var updateX = false;
    var updateY = false;
    var newRect;

    
    if (!Keys.meta) { 
      newRect = Trenches.snap(bounds,true);
      if (newRect) 
        update = true;
    }

    
    newRect = newRect || bounds;

    var swb = this.safeWindowBounds;

    var snapRadius = ( Keys.meta ? 0 : Trenches.defaultRadius );
    if (newRect.left < swb.left + snapRadius ) {
      newRect.left = swb.left;
      update = true;
      updateX = true;
    }
    if (newRect.left + newRect.width > swb.left + swb.width - snapRadius) {
      if (updateX)
        newRect.width = swb.left + swb.width - newRect.left;
      else
        newRect.left = swb.left + swb.width - newRect.width;
      update = true;
    }
    if (newRect.top < swb.top + snapRadius) {
      newRect.top = swb.top;
      update = true;
      updateY = true;
    }
    if (newRect.top + newRect.height > swb.top + swb.height - snapRadius) {
      if (updateY)
        newRect.height = swb.top + swb.height - newRect.top;
      else
        newRect.top = swb.top + swb.height - newRect.height;
      update = true;
    }

    if (update)
      this.item.setBounds(newRect,true);

    return ui;
  },
  
  
  
  
  drag: function(event, ui) {

      var bb = this.item.getBounds();
      bb.left = ui.position.left;
      bb.top = ui.position.top;
      this.item.setBounds(bb, true);
      ui = this.snap(event,ui);


      
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
    this.$el.data('isDragging', false);    

    
    
    
    





    if(this.parent && !this.parent.locked.close && this.parent != this.item.parent 
        && this.parent._children.length == 0 && !this.parent.getTitle()) {
      this.parent.close();
    }
     
    if(this.parent && this.parent.expanded)
      this.parent.arrange();
      
    if(this.item && !this.item.parent) {
      this.item.setZ(drag.zIndex);
      drag.zIndex++;
      
      this.item.reloadBounds();
      this.item.pushAway();
    }
    
    Trenches.disactivate();
    
  },
  getSafeWindowBounds: function() {
    
    var gutter = Items.defaultGutter;
    var pageBounds = Items.getPageBounds();
    var newTabGroupBounds = Groups.getBoundsForNewTabGroup();
    
    
    
    var topGutter = 5;
    return new Rect( gutter, topGutter, pageBounds.width - 2 * gutter, newTabGroupBounds.top -  gutter - topGutter );
  }
};
