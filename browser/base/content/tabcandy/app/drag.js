


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
  
  this.$el.data('isDragging', true);
  this.item.setZ(999999);
  
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
    var me = this.item;
		var bounds = me.getBounds();

		
		var newRect = Trenches.snap(bounds,true);
		if (newRect) 
			me.setBounds(newRect,true);
    
    return ui;
    
  },
  
  
  
  
  drag: function(event, ui) {
    if(this.item.isAGroup) {
      var bb = this.item.getBounds();
      bb.left = ui.position.left;
      bb.top = ui.position.top;
      this.item.setBounds(bb, true);
      ui = this.snap(event,ui);
    } else
      this.item.reloadBounds();
      
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
    
  }
};
