
























































function Trench(element, xory, type, edge) {
  
  
  
  this.id = Trenches.nextId++;

  
  
  
  
  
  
  
  this.el = element;
  this.parentItem = null;
  this.xory = xory; 
  this.type = type; 
  this.edge = edge; 

  this.$el = iQ(this.el);

  
  
  
  this.dom = [];

  
  
  
  this.showGuide = false;

  
  
  
  
  
  this.active = false;
  this.gutter = Items.defaultGutter;

  
  
  
  this.position = 0;

  
  
  
  
  
  this.range = new Range(0,10000);
  this.minRange = new Range(0,0);
  this.activeRange = new Range(0,10000);
};

Trench.prototype = {
  
  
  
  toString: function Trench_toString() {
    return "[Trench " + this.edge + " " + this.type +
           (this.parentItem ? " (" + this.parentItem + ")" : "") +
           "]";
  },

  
  
  
  get radius() this.customRadius || Trenches.defaultRadius,

  setParentItem: function Trench_setParentItem(item) {
    if (!item.isAnItem) {
      Utils.assert(false, "parentItem must be an Item");
      return false;
    }
    this.parentItem = item;
    return true;
  },

  
  
  
  
  
  
  
  
  setPosition: function Trench_setPos(position, range, minRange) {
    this.position = position;

    var page = Items.getPageBounds(true);

    
    if (Utils.isRange(range)) {
      this.range = range;
    } else {
      this.range = new Range(0, (this.xory == 'x' ? page.height : page.width));
    }

    
    if (Utils.isRange(minRange))
      this.minRange = minRange;

    
    if (this.xory == "x") 
      this.rect = new Rect(this.position - this.radius, this.range.min, 2 * this.radius, this.range.extent);
    else 
      this.rect = new Rect(this.range.min, this.position - this.radius, this.range.extent, 2 * this.radius);

    this.show(); 
  },

  
  
  
  
  
  
  setActiveRange: function Trench_setActiveRect(activeRange) {
    if (!Utils.isRange(activeRange))
      return false;
    this.activeRange = activeRange;
    if (this.xory == "x") { 
      this.activeRect = new Rect(this.position - this.radius, this.activeRange.min, 2 * this.radius, this.activeRange.extent);
      this.guideRect = new Rect(this.position, this.activeRange.min, 0, this.activeRange.extent);
    } else { 
      this.activeRect = new Rect(this.activeRange.min, this.position - this.radius, this.activeRange.extent, 2 * this.radius);
      this.guideRect = new Rect(this.activeRange.min, this.position, this.activeRange.extent, 0);
    }
    return true;
  },

  
  
  
  
  
  
  
  setWithRect: function Trench_setWithRect(rect) {

    if (!Utils.isRect(rect))
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
    if (this.active && this.showGuide) {
      if (!this.dom.guideTrench)
        this.dom.guideTrench = iQ("<div/>").addClass('guideTrench').css({id: 'guideTrench'+this.id});
      var guideTrench = this.dom.guideTrench;
      guideTrench.css(this.guideRect);
      iQ("body").append(guideTrench);
    } else {
      if (this.dom.guideTrench) {
        this.dom.guideTrench.remove();
        delete this.dom.guideTrench;
      }
    }

    if (!Trenches.showDebug) {
      this.hide(true); 
      return;
    }

    if (!this.dom.visibleTrench)
      this.dom.visibleTrench = iQ("<div/>")
        .addClass('visibleTrench')
        .addClass(this.type) 
        .css({id: 'visibleTrench'+this.id});
    var visibleTrench = this.dom.visibleTrench;

    if (!this.dom.activeVisibleTrench)
      this.dom.activeVisibleTrench = iQ("<div/>")
        .addClass('activeVisibleTrench')
        .addClass(this.type) 
        .css({id: 'activeVisibleTrench'+this.id});
    var activeVisibleTrench = this.dom.activeVisibleTrench;

    if (this.active)
      activeVisibleTrench.addClass('activeTrench');
    else
      activeVisibleTrench.removeClass('activeTrench');

    visibleTrench.css(this.rect);
    activeVisibleTrench.css(this.activeRect || this.rect);
    iQ("body").append(visibleTrench);
    iQ("body").append(activeVisibleTrench);
  },

  
  
  
  hide: function Trench_hide(dontHideGuides) {
    if (this.dom.visibleTrench)
      this.dom.visibleTrench.remove();
    if (this.dom.activeVisibleTrench)
      this.dom.activeVisibleTrench.remove();
    if (!dontHideGuides && this.dom.guideTrench)
      this.dom.guideTrench.remove();
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  rectOverlaps: function Trench_rectOverlaps(rect,stationaryCorner,assumeConstantSize,keepProportional) {
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
          if (stationaryCorner.indexOf('right') > -1)
            rect.width = rect.right - this.position;
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
          if (stationaryCorner.indexOf('bottom') > -1)
            rect.height = rect.bottom - this.position;
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
    return (this.position - this.radius < position &&
           position < this.position + this.radius &&
           this.activeRange.overlaps(range));
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

    
    this.setActiveRange(this.range);

    
    if (this.type != 'guide')
      return;

    var groupItems = GroupItems.groupItems;
    var trench = this;
    groupItems.forEach(function(groupItem) {
      if (groupItem.isDragging) 
        return;
      if (trench.el == groupItem.container) 
        return;
      var bounds = groupItem.getBounds();
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
  disabled: false,

  
  
  
  
  preferTop: true,
  get preferLeft() { return !UI.rtl; },

  trenches: [],

  
  
  
  toString: function Trenches_toString() {
    return "[Trenches count=" + this.trenches.length + "]";
  },

  
  
  
  
  
  
  getById: function Trenches_getById(id) {
    return this.trenches[id];
  },

  
  
  
  
  
  
  
  
  
  register: function Trenches_register(element, xory, type, edge) {
    var trench = new Trench(element, xory, type, edge);
    this.trenches[trench.id] = trench;
    return trench.id;
  },

  
  
  
  
  
  
  
  
  
  
  registerWithItem: function Trenches_registerWithItem(item, type) {
    var container = item.container;
    var ids = {};
    ids.left = Trenches.register(container,"x",type,"left");
    ids.right = Trenches.register(container,"x",type,"right");
    ids.top = Trenches.register(container,"y",type,"top");
    ids.bottom = Trenches.register(container,"y",type,"bottom");

    this.getById(ids.left).setParentItem(item);
    this.getById(ids.right).setParentItem(item);
    this.getById(ids.top).setParentItem(item);
    this.getById(ids.bottom).setParentItem(item);

    return ids;
  },

  
  
  
  
  
  
  unregister: function Trenches_unregister(ids) {
    if (!Array.isArray(ids))
      ids = [ids];
    var self = this;
    ids.forEach(function(id) {
      self.trenches[id].hide();
      delete self.trenches[id];
    });
  },

  
  
  
  
  
  
  activateOthersTrenches: function Trenches_activateOthersTrenches(element) {
    this.trenches.forEach(function(t) {
      if (t.el === element)
        return;
      if (t.parentItem && (t.parentItem.isAFauxItem || t.parentItem.isDragging))
        return;
      t.active = true;
      t.calculateActiveRange();
      t.show(); 
    });
  },

  
  
  
  disactivate: function Trenches_disactivate() {
    this.trenches.forEach(function(t) {
      t.active = false;
      t.showGuide = false;
      t.show();
    });
  },

  
  
  
  hideGuides: function Trenches_hideGuides() {
    this.trenches.forEach(function(t) {
      t.showGuide = false;
      t.show();
    });
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  snap: function Trenches_snap(rect,stationaryCorner,assumeConstantSize,keepProportional) {
    
    Trenches.hideGuides();

    var updated = false;
    var updatedX = false;
    var updatedY = false;

    var snappedTrenches = {};

    for (var i in this.trenches) {
      var t = this.trenches[i];
      if (!t.active)
        continue;
      
      var newRect = t.rectOverlaps(rect,stationaryCorner,assumeConstantSize,keepProportional);

      if (newRect) { 

        if (assumeConstantSize && updatedX && updatedY)
          break;
        if (assumeConstantSize && updatedX && (newRect.adjustedEdge == "left"||newRect.adjustedEdge == "right"))
          continue;
        if (assumeConstantSize && updatedY && (newRect.adjustedEdge == "top"||newRect.adjustedEdge == "bottom"))
          continue;

        rect = newRect;
        updated = true;

        
        snappedTrenches[newRect.adjustedEdge] = t;

        
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

    if (updated) {
      rect.snappedTrenches = snappedTrenches;
      return rect;
    }
    return false;
  },

  
  
  
  show: function Trenches_show() {
    this.trenches.forEach(function(t) {
      t.show();
    });
  },

  
  
  
  toggleShown: function Trenches_toggleShown() {
    this.showDebug = !this.showDebug;
    this.show();
  }
};
