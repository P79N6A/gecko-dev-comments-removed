



"use strict";
const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = ["PanelWideWidgetTracker"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
  "resource:///modules/CustomizableUI.jsm");

let gModuleName = "[PanelWideWidgetTracker]";
#include logging.js

let gPanel = CustomizableUI.AREA_PANEL;

let gPanelPlacements = [];


let gWideWidgets = new Set();

let gSeenWidgets = new Set();


const kWidePanelItemClass = "panel-combined-item";

let PanelWideWidgetTracker = {
  
  onWidgetAdded: function(aWidgetId, aArea, aPosition) {
    if (aArea == gPanel) {
      let moveForward = this.shouldMoveForward(aWidgetId, aPosition);
      this.adjustWidgets(aWidgetId, aPosition, moveForward);
    }
  },
  onWidgetMoved: function(aWidgetId, aArea, aOldPosition, aNewPosition) {
    if (aArea == gPanel) {
      let moveForward = this.shouldMoveForward(aWidgetId, aNewPosition);
      this.adjustWidgets(aWidgetId, Math.min(aOldPosition, aNewPosition), moveForward);
    }
  },
  onWidgetRemoved: function(aWidgetId, aPrevArea) {
    if (aPrevArea == gPanel) {
      let pos = gPanelPlacements.indexOf(aWidgetId);
      this.adjustWidgets(aWidgetId, pos);
    }
  },
  
  
  
  onWidgetAfterDOMChange: function(aNode, aNextNode, aContainer) {
    if (!gSeenWidgets.has(aNode.id)) {
      if (aNode.classList.contains(kWidePanelItemClass)) {
        gWideWidgets.add(aNode.id);
      }
      gSeenWidgets.add(aNode.id);
    }
  },
  
  onWidgetDestroyed: function(aWidgetId) {
    gSeenWidgets.remove(aWidgetId);
    gWideWidgets.remove(aWidgetId);
  },
  shouldMoveForward: function(aWidgetId, aPosition) {
    let currentWidgetAtPosition = gPanelPlacements[aPosition];
    return gWideWidgets.has(currentWidgetAtPosition) && !gWideWidgets.has(aWidgetId);
  },
  adjustWidgets: function(aWidgetId, aPosition, aMoveForwards) {
    if (this.adjustmentStack == 0) {
      this.movingForward = aMoveForwards;
    }
    gPanelPlacements = CustomizableUI.getWidgetIdsInArea(gPanel);
    
    let widgetsAffected = [];
    for (let widget of gWideWidgets) {
      let wideWidgetPos = gPanelPlacements.indexOf(widget);
      
      
      
      
      
      if (wideWidgetPos > aPosition || (!this.adjustmentStack && wideWidgetPos == aPosition)) {
        widgetsAffected.push(widget);
      }
    }
    if (!widgetsAffected.length) {
      return;
    }
    widgetsAffected.sort(function(a, b) gPanelPlacements.indexOf(a) < gPanelPlacements.indexOf(b));
    this.adjustmentStack++;
    this.adjustPosition(widgetsAffected[0]);
    this.adjustmentStack--;
    if (this.adjustmentStack == 0) {
      delete this.movingForward;
    }
  },
  
  
  
  adjustPosition: function(aWidgetId) {
    
    
    const kColumnsInMenuPanel = 3;

    
    let placementIndex = gPanelPlacements.indexOf(aWidgetId);
    let prevSiblingCount = 0;
    let fixedPos = null;
    while (placementIndex--) {
      let thisWidgetId = gPanelPlacements[placementIndex];
      if (gWideWidgets.has(thisWidgetId)) {
        continue;
      }
      let widgetWrapper = CustomizableUI.getWidget(gPanelPlacements[placementIndex]);
      
      if (!widgetWrapper) {
        continue;
      }
      
      if (widgetWrapper.provider == CustomizableUI.PROVIDER_XUL &&
          widgetWrapper.instances.length == 0) {
        continue;
      }

      
      if (widgetWrapper.provider == CustomizableUI.PROVIDER_API &&
          widgetWrapper.showInPrivateBrowsing === false) {
        if (!fixedPos) {
          fixedPos = placementIndex;
        } else {
          fixedPos = Math.min(fixedPos, placementIndex);
        }
        
        prevSiblingCount = 0;
      } else {
        prevSiblingCount++;
      }
    }

    if (fixedPos !== null || prevSiblingCount % kColumnsInMenuPanel) {
      let desiredPos = (fixedPos !== null) ? fixedPos : gPanelPlacements.indexOf(aWidgetId);
      if (this.movingForward) {
        
        desiredPos += (kColumnsInMenuPanel - (prevSiblingCount % kColumnsInMenuPanel)) + 1;
      } else {
        desiredPos -= prevSiblingCount % kColumnsInMenuPanel;
      }
      
      
      
      
      CustomizableUI.moveWidgetWithinArea(aWidgetId, desiredPos);
    }
  },
  adjustmentStack: 0,
  init: function() {
    
    gPanelPlacements = CustomizableUI.getWidgetIdsInArea(gPanel);
    CustomizableUI.addListener(this);
  },
};
