






















































var Trench = function(element, xory, type, edge) {
  
  
  
  this.id = Trenches.nextId++;

  
  
  
  
  
  
  this.el = element;
  this.xory = xory; 
  this.type = type; 
  this.edge = edge; 

  this.$el = iQ(this.el);

  
  
  
  
  
  this.active = false;
  this.gutter = Items.defaultGutter;

  
  
  
  this.position = 0;

  
  
  
  this.radius = Trenches.defaultRadius;

  
  
  
  
  
  this.range = new Range(0,10000);
  this.minRange = new Range(0,0);
  this.activeRange = new Range(0,10000);
};
Trench.prototype = {
  
  
  
  
  
  
  
  
  setPosition: function Trench_setPos(position, range, minRange) {
    this.position = position;
    
    
    if (isRange(range)) {
      this.range = range;
    }
    
    
    if (isRange(minRange))
      this.minRange = minRange;
    
    
    if ( this.xory == "x" ) 
      this.rect = new Rect ( this.position - this.radius, this.range.min, 2 * this.radius, this.range.extent );
    else
      this.rect = new Rect ( this.range.min, this.position - this.radius, this.range.extent, 2 * this.radius );
      
    this.show(); 

  },
  
  
  
  
  
  
  
  setActiveRange: function Trench_setActiveRect(activeRange) {
    if (!isRange(activeRange))
      return false;
    this.activeRange = activeRange;
    if ( this.xory == "x" ) 
      this.activeRect = new Rect ( this.position - this.radius, this.activeRange.min, 2 * this.radius, this.activeRange.extent );
    else
      this.activeRect = new Rect ( this.activeRange.min, this.position - this.radius, this.activeRange.extent, 2 * this.radius );    
  },
  
  
  
  
  
  
  
  
  setWithRect: function Trench_setWithRect(rect) {
    
    if (!isRect(rect))
      Utils.error('argument must be Rect');
    
    
    
    
    if (this.xory == "x")
      var range = new Range(rect.top - this.gutter, rect.bottom + this.gutter);
    else
      var range = new Range(rect.left - this.gutter, rect.right + this.gutter);

    if (this.type == "border") {
      
      if (this.edge == "left")
        this.setPosition(rect.left - this.gutter, range);
      else if (this.edge == "right")
        this.setPosition(rect.right + this.gutter, range);
      else if (this.edge == "top")
        this.setPosition(rect.top - this.gutter, range);
      else if (this.edge == "bottom")
        this.setPosition(rect.bottom + this.gutter, range);
    } else if (this.type == "guide") {
      
      if (this.edge == "left")    
        this.setPosition(rect.left, false, range);
      else if (this.edge == "right")
        this.setPosition(rect.right, false, range);
      else if (this.edge == "top")
        this.setPosition(rect.top, false, range);
      else if (this.edge == "bottom")
        this.setPosition(rect.bottom, false, range);
    }
  },
  
  
  
  
  
  
  
  show: function Trench_show() { 
    if (!Trenches.showDebug) {
      this.hide();
      return;
    }

    if (!this.visibleTrench)
      this.visibleTrench = iQ("<div/>").css({position: 'absolute', zIndex:-102, opacity: 0.05, id: 'visibleTrench'+this.id});
    var visibleTrench = this.visibleTrench;

    if (!this.activeVisibleTrench) {
      this.activeVisibleTrench = iQ("<div/>").css({position: 'absolute', zIndex:-101, id: 'activeVisibleTrench'+this.id});
    }
    var activeVisibleTrench = this.activeVisibleTrench;

    if (this.active)
      activeVisibleTrench.css({opacity: 0.45});
    else
      activeVisibleTrench.css({opacity: 0});
      
    if (this.type == "border") {
      visibleTrench.css({backgroundColor:'red'});
      activeVisibleTrench.css({backgroundColor:'red'});      
    } else {
      visibleTrench.css({backgroundColor:'blue'});
      activeVisibleTrench.css({backgroundColor:'blue'});
    }

    visibleTrench.css(this.rect);
    activeVisibleTrench.css(this.activeRect || this.rect);
    iQ("body").append(visibleTrench);
    iQ("body").append(activeVisibleTrench);
  },
  
  
  
  
  hide: function Trench_hide() {
    if (this.visibleTrench)
      this.visibleTrench.remove();
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  rectOverlaps: function Trench_rectOverlaps(rect,assumeConstantSize,keepProportional) {
    var edgeToCheck;
    if (this.type == "border") {
      if (this.edge == "left")
        edgeToCheck = "right";
      else if (this.edge == "right")
        edgeToCheck = "left";
      else if (this.edge == "top")
        edgeToCheck = "bottom";
      else if (this.edge == "bottom")
        edgeToCheck = "top";
    } else { 
      edgeToCheck = this.edge;
    }

    rect.adjustedEdge = edgeToCheck;
    
    switch (edgeToCheck) {
      case "left":
        if (this.ruleOverlaps(rect.left, rect.yRange)) {
          rect.left = this.position;
          return rect;
        }
        break;
      case "right":
        if (this.ruleOverlaps(rect.right, rect.yRange)) {
          if (assumeConstantSize) {
            rect.left = this.position - rect.width;
          } else {
            var newWidth = this.position - rect.left;
            if (keepProportional)
              rect.height = rect.height * newWidth / rect.width;
            rect.width = newWidth;
          }
          return rect;
        }
        break;
      case "top":
        if (this.ruleOverlaps(rect.top, rect.xRange)) {
          rect.top = this.position;
          return rect;
        }
        break;
      case "bottom":
        if (this.ruleOverlaps(rect.bottom, rect.xRange)) {
          if (assumeConstantSize) {
            rect.top = this.position - rect.height;
          } else {
            var newHeight = this.position - rect.top;
            if (keepProportional)
              rect.width = rect.width * newHeight / rect.height;
            rect.height = newHeight;
          }
          return rect;
        }
    }
      
    return false;
  },
  
  
  
  
  
  
  
  
  
  
  ruleOverlaps: function Trench_ruleOverlaps(position, range) {
    return (this.position - this.radius <= position && position <= this.position + this.radius
            && this.activeRange.contains(range));
  },
  
  
  
  
  
  
  
  
  
  
  adjustRangeIfIntercept: function Trench_adjustRangeIfIntercept(position, range) {
    if (this.position - this.radius > range.min && this.position + this.radius < range.max) {
      var activeRange = new Range(this.activeRange);
      
      
      
      
      
      
      if (position < this.minRange.min) {
        activeRange.min = Math.min(this.minRange.min,position);        
      } else if (position > this.minRange.max) {
        activeRange.max = Math.max(this.minRange.max,position);
      } else {
        
        
      }
      return activeRange;
    }
    return false;
  },
  
  
  
  
  
  calculateActiveRange: function Trench_calculateActiveRange() {
    
    if (this.type != 'guide')
      return;

    
    this.setActiveRange(this.range);

    var groups = Groups.groups;
    var trench = this;
    groups.forEach(function( group ) {
      if (group.isDragging) 
        return;
      if (group.isNewTabsGroup())
        return;
      if (trench.el == group.container) 
        return;
      var bounds = group.getBounds();
      var activeRange = new Range();
      if (trench.xory == 'y') { 
        activeRange = trench.adjustRangeIfIntercept(bounds.left, bounds.yRange);
        if (activeRange)
          trench.setActiveRange(activeRange);
        activeRange = trench.adjustRangeIfIntercept(bounds.right, bounds.yRange);
        if (activeRange)
          trench.setActiveRange(activeRange);
      } else { 
        activeRange = trench.adjustRangeIfIntercept(bounds.top, bounds.xRange);
        if (activeRange)
          trench.setActiveRange(activeRange);
        activeRange = trench.adjustRangeIfIntercept(bounds.bottom, bounds.xRange);
        if (activeRange)
          trench.setActiveRange(activeRange);
      }
    });
  }
};




var Trenches = {
  
  
  
  
  
  nextId: 0,
  showDebug: false,
  defaultRadius: 10,

  
  
  
  
  preferTop: true,
  preferLeft: true,
  
  activeTrenches: {},
  trenches: [],

  
  
  
  
  
  
  getById: function Trenches_getById(id) {
    return this.trenches[id];
  },

  
  
  
  
  
  
  
  
  
  register: function Trenches_register(element, xory, type, edge) {
    var trench = new Trench(element, xory, type, edge);
    this.trenches[trench.id] = trench;
    return trench.id;
  },

  
  
  
  
  
  
  unregister: function Trenches_unregister(ids) {
    if (!iQ.isArray(ids))
      ids = [ids];
    var self = this;
    ids.forEach(function(id){
      self.trenches[id].hide();
      delete self.trenches[id];
    });
  },

  
  
  
  
  
  
  activateOthersTrenches: function Trenches_activateOthersTrenches(element) {
    this.trenches.forEach(function(t) {
      if (t.el === element)
        return;
      t.active = true;
      t.calculateActiveRange();
      t.show(); 
    });
  },

  
  
  
  disactivate: function Trenches_disactivate() {
    this.trenches.forEach(function(t) {
      t.active = false;
      t.show();
    });
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  snap: function Trenches_snap(rect,assumeConstantSize,keepProportional) {
    var aT = this.activeTrenches;
    
    var updated = false;
    var updatedX = false;
    var updatedY = false;
    
    for (let i in this.trenches) {
      var t = this.trenches[i];
      if (!t.active)
        continue;
      
      var newRect = t.rectOverlaps(rect,assumeConstantSize,keepProportional);

      if (newRect) {
        if (assumeConstantSize && updatedX && updatedY)
          break;
        if (assumeConstantSize && updatedX && (newRect.adjustedEdge == "left"||newRect.adjustedEdge == "right"))
          continue;
        if (assumeConstantSize && updatedY && (newRect.adjustedEdge == "top"||newRect.adjustedEdge == "bottom"))
          continue;
         rect = newRect;
         updated = true;
  
        
        if (newRect.adjustedEdge == "left" && this.preferLeft)
          updatedX = true;
        if (newRect.adjustedEdge == "right" && !this.preferLeft)
          updatedX = true;
  
        
        if (newRect.adjustedEdge == "top" && this.preferTop)
          updatedY = true;
        if (newRect.adjustedEdge == "bottom" && !this.preferTop)
          updatedY = true;

      }
    }    

    if (updated)
      return rect;
    else
      return false;
  },

  
  
  
  show: function Trenches_show() {
    this.trenches.forEach(function(t){
      t.show();
    });
  }, 

  
  
  
  toggleShown: function Trenches_toggleShown() {
    this.showDebug = !this.showDebug;
    this.show();
  }
};
