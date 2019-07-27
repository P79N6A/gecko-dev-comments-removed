



"use strict";

Components.utils.import("resource:///modules/CustomizableUI.jsm");

let gManagers = new WeakMap();

const kPaletteId = "customization-palette";
const kPlaceholderClass = "panel-customization-placeholder";

this.EXPORTED_SYMBOLS = ["DragPositionManager"];

function AreaPositionManager(aContainer) {
  
  let window = aContainer.ownerDocument.defaultView;
  this._dir = window.getComputedStyle(aContainer).direction;
  let containerRect = aContainer.getBoundingClientRect();
  this._containerInfo = {
    left: containerRect.left,
    right: containerRect.right,
    top: containerRect.top,
    width: containerRect.width
  };
  this._inPanel = aContainer.id == CustomizableUI.AREA_PANEL;
  this._horizontalDistance = null;
  this.update(aContainer);
}

AreaPositionManager.prototype = {
  _nodePositionStore: null,
  _wideCache: null,

  update: function(aContainer) {
    let window = aContainer.ownerDocument.defaultView;
    this._nodePositionStore = new WeakMap();
    this._wideCache = new Set();
    let last = null;
    let singleItemHeight;
    for (let child of aContainer.children) {
      if (child.hidden) {
        continue;
      }
      let isNodeWide = this._checkIfWide(child);
      if (isNodeWide) {
        this._wideCache.add(child.id);
      }
      let coordinates = this._lazyStoreGet(child);
      
      
      if (!this._horizontalDistance && last && !isNodeWide) {
        this._horizontalDistance = coordinates.left - last.left;
      }
      
      if (!isNodeWide && !singleItemHeight) {
        singleItemHeight = coordinates.height;
      }
      last = !isNodeWide ? coordinates : null;
    }
    if (this._inPanel) {
      this._heightToWidthFactor = CustomizableUI.PANEL_COLUMN_COUNT;
    } else {
      this._heightToWidthFactor = this._containerInfo.width / singleItemHeight;
    }
  },

  







  find: function(aContainer, aX, aY, aDraggedItemId) {
    let closest = null;
    let minCartesian = Number.MAX_VALUE;
    let containerX = this._containerInfo.left;
    let containerY = this._containerInfo.top;
    for (let node of aContainer.children) {
      let coordinates = this._lazyStoreGet(node);
      let offsetX = coordinates.x - containerX;
      let offsetY = coordinates.y - containerY;
      let hDiff = offsetX - aX;
      let vDiff = offsetY - aY;
      
      
      if (this.isWide(node)) {
        hDiff /= CustomizableUI.PANEL_COLUMN_COUNT;
      }
      
      
      hDiff /= this._heightToWidthFactor;

      let cartesianDiff = hDiff * hDiff + vDiff * vDiff;
      if (cartesianDiff < minCartesian) {
        minCartesian = cartesianDiff;
        closest = node;
      }
    }

    
    if (closest) {
      let doc = aContainer.ownerDocument;
      let draggedItem = doc.getElementById(aDraggedItemId);
      
      if (this._inPanel && draggedItem &&
          draggedItem.classList.contains(CustomizableUI.WIDE_PANEL_CLASS)) {
        return this._firstInRow(closest);
      }
      let targetBounds = this._lazyStoreGet(closest);
      let farSide = this._dir == "ltr" ? "right" : "left";
      let outsideX = targetBounds[farSide];
      
      
      if (aY > targetBounds.top && aY < targetBounds.bottom) {
        if ((this._dir == "ltr" && aX > outsideX) ||
            (this._dir == "rtl" && aX < outsideX)) {
          return closest.nextSibling || aContainer;
        }
      }
    }
    return closest;
  },

  





  insertPlaceholder: function(aContainer, aBefore, aWide, aSize, aIsFromThisArea) {
    let isShifted = false;
    let shiftDown = aWide;
    for (let child of aContainer.children) {
      
      if (child.getAttribute("hidden") == "true") {
        continue;
      }
      
      
      
      if (child == aBefore && !child.classList.contains(kPlaceholderClass)) {
        isShifted = true;
        
        
        if (!shiftDown && this.isWide(child)) {
          shiftDown = true;
        }
      }
      
      
      
      if (this.__undoShift) {
        isShifted = false;
      }
      if (isShifted) {
        
        
        if (this.__moveDown) {
          shiftDown = true;
        }
        if (aIsFromThisArea && !this._lastPlaceholderInsertion) {
          child.setAttribute("notransition", "true");
        }
        
        child.style.transform = this._getNextPos(child, shiftDown, aSize);
      } else {
        
        child.style.transform = "";
      }
    }
    if (aContainer.lastChild && aIsFromThisArea &&
        !this._lastPlaceholderInsertion) {
      
      aContainer.lastChild.getBoundingClientRect();
      
      for (let child of aContainer.children) {
        child.removeAttribute("notransition");
      }
    }
    delete this.__moveDown;
    delete this.__undoShift;
    this._lastPlaceholderInsertion = aBefore;
  },

  isWide: function(aNode) {
    return this._wideCache.has(aNode.id);
  },

  _checkIfWide: function(aNode) {
    return this._inPanel && aNode && aNode.firstChild &&
           aNode.firstChild.classList.contains(CustomizableUI.WIDE_PANEL_CLASS);
  },

  






  clearPlaceholders: function(aContainer, aNoTransition) {
    for (let child of aContainer.children) {
      if (aNoTransition) {
        child.setAttribute("notransition", true);
      }
      child.style.transform = "";
      if (aNoTransition) {
        
        child.getBoundingClientRect();
        child.removeAttribute("notransition");
      }
    }
    
    
    if (aNoTransition) {
      this._lastPlaceholderInsertion = null;
    }
  },

  _getNextPos: function(aNode, aShiftDown, aSize) {
    
    if (this._inPanel && aShiftDown) {
      return "translate(0, " + aSize.height + "px)";
    }
    return this._diffWithNext(aNode, aSize);
  },

  _diffWithNext: function(aNode, aSize) {
    let xDiff;
    let yDiff = null;
    let nodeBounds = this._lazyStoreGet(aNode);
    let side = this._dir == "ltr" ? "left" : "right";
    let next = this._getVisibleSiblingForDirection(aNode, "next");
    
    
    if (next) {
      let otherBounds = this._lazyStoreGet(next);
      xDiff = otherBounds[side] - nodeBounds[side];
      
      
      
      
      
      if (this.isWide(next)) {
        let otherXDiff = this._moveNextBasedOnPrevious(aNode, nodeBounds,
                                                       this._firstInRow(aNode));
        
        
        
        
        
        
        
        
        if ((otherXDiff < 0) == (xDiff < 0)) {
          this.__moveDown = true;
        } else {
          
          
          xDiff = otherXDiff;
          this.__undoShift = true;
        }
      } else {
        
        
        
        yDiff = otherBounds.top - nodeBounds.top;
      }
    } else {
      
      
      let firstNode = this._firstInRow(aNode);
      if (aNode == firstNode) {
        
        
        xDiff = this._horizontalDistance || aSize.width;
      } else {
        
        
        
        xDiff = this._moveNextBasedOnPrevious(aNode, nodeBounds, firstNode);
      }
    }

    
    if (yDiff === null) {
      
      
      if ((xDiff > 0 && this._dir == "rtl") || (xDiff < 0 && this._dir == "ltr")) {
        yDiff = aSize.height;
      } else {
        
        yDiff = 0;
      }
    }
    return "translate(" + xDiff + "px, " + yDiff + "px)";
  },

  






  _moveNextBasedOnPrevious: function(aNode, aNodeBounds, aFirstNodeInRow) {
    let next = this._getVisibleSiblingForDirection(aNode, "previous");
    let otherBounds = this._lazyStoreGet(next);
    let side = this._dir == "ltr" ? "left" : "right";
    let xDiff = aNodeBounds[side] - otherBounds[side];
    
    
    
    let bound = this._containerInfo[this._dir == "ltr" ? "right" : "left"];
    if ((this._dir == "ltr" && xDiff + aNodeBounds.right > bound) ||
        (this._dir == "rtl" && xDiff + aNodeBounds.left < bound)) {
      xDiff = this._lazyStoreGet(aFirstNodeInRow)[side] - aNodeBounds[side];
    }
    return xDiff;
  },

  





  _lazyStoreGet: function(aNode) {
    let rect = this._nodePositionStore.get(aNode);
    if (!rect) {
      
      
      
      
      let clientRect = aNode.getBoundingClientRect();
      rect = {
        left: clientRect.left,
        right: clientRect.right,
        width: clientRect.width,
        height: clientRect.height,
        top: clientRect.top,
        bottom: clientRect.bottom,
      };
      rect.x = rect.left + rect.width / 2;
      rect.y = rect.top + rect.height / 2;
      Object.freeze(rect);
      this._nodePositionStore.set(aNode, rect);
    }
    return rect;
  },

  _firstInRow: function(aNode) {
    
    
    
    let bound = Math.floor(this._lazyStoreGet(aNode).top);
    let rv = aNode;
    let prev;
    while (rv && (prev = this._getVisibleSiblingForDirection(rv, "previous"))) {
      if (Math.floor(this._lazyStoreGet(prev).bottom) <= bound) {
        return rv;
      }
      rv = prev;
    }
    return rv;
  },

  _getVisibleSiblingForDirection: function(aNode, aDirection) {
    let rv = aNode;
    do {
      rv = rv[aDirection + "Sibling"];
    } while (rv && rv.getAttribute("hidden") == "true")
    return rv;
  }
}

let DragPositionManager = {
  start: function(aWindow) {
    let areas = CustomizableUI.areas.filter((area) => CustomizableUI.getAreaType(area) != "toolbar");
    areas = areas.map((area) => CustomizableUI.getCustomizeTargetForArea(area, aWindow));
    areas.push(aWindow.document.getElementById(kPaletteId));
    for (let areaNode of areas) {
      let positionManager = gManagers.get(areaNode);
      if (positionManager) {
        positionManager.update(areaNode);
      } else {
        gManagers.set(areaNode, new AreaPositionManager(areaNode));
      }
    }
  },

  add: function(aWindow, aArea, aContainer) {
    if (CustomizableUI.getAreaType(aArea) != "toolbar") {
      return;
    }

    gManagers.set(aContainer, new AreaPositionManager(aContainer));
  },

  remove: function(aWindow, aArea, aContainer) {
    if (CustomizableUI.getAreaType(aArea) != "toolbar") {
      return;
    }

    gManagers.delete(aContainer);
  },

  stop: function() {
    gManagers.clear();
  },

  getManagerForArea: function(aArea) {
    return gManagers.get(aArea);
  }
};

Object.freeze(DragPositionManager);

