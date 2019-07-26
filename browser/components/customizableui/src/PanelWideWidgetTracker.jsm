



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


const kWidePanelItemClass = "panel-wide-item";



const kColumnsInMenuPanel = 3;

let PanelWideWidgetTracker = {
  
  onWidgetAdded: function(aWidgetId, aArea, aPosition) {
    if (aArea == gPanel) {
      gPanelPlacements = CustomizableUI.getWidgetIdsInArea(gPanel);
      let moveForward = this.shouldMoveForward(aWidgetId, aPosition);
      this.adjustWidgets(aWidgetId, moveForward);
    }
  },
  onWidgetMoved: function(aWidgetId, aArea, aOldPosition, aNewPosition) {
    if (aArea == gPanel) {
      gPanelPlacements = CustomizableUI.getWidgetIdsInArea(gPanel);
      let moveForward = this.shouldMoveForward(aWidgetId, aNewPosition);
      this.adjustWidgets(aWidgetId, moveForward);
    }
  },
  onWidgetRemoved: function(aWidgetId, aPrevArea) {
    if (aPrevArea == gPanel) {
      gPanelPlacements = CustomizableUI.getWidgetIdsInArea(gPanel);
      let pos = gPanelPlacements.indexOf(aWidgetId);
      this.adjustWidgets(aWidgetId, false);
    }
  },
  onWidgetReset: function(aWidgetId) {
    gPanelPlacements = CustomizableUI.getWidgetIdsInArea(gPanel);
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
    gSeenWidgets.delete(aWidgetId);
    gWideWidgets.delete(aWidgetId);
  },
  shouldMoveForward: function(aWidgetId, aPosition) {
    let currentWidgetAtPosition = gPanelPlacements[aPosition + 1];
    return gWideWidgets.has(currentWidgetAtPosition) && !gWideWidgets.has(aWidgetId);
  },
  adjustWidgets: function(aWidgetId, aMoveForwards) {
    if (this.adjusting) {
      return;
    }
    this.adjusting = true;
    let widgetsAffected = [w for (w of gPanelPlacements) if (gWideWidgets.has(w))];
    
    
    
    
    let compareFn = aMoveForwards ? (function(a, b) a < b) : (function(a, b) a > b)
    widgetsAffected.sort(function(a, b) compareFn(gPanelPlacements.indexOf(a),
                                                  gPanelPlacements.indexOf(b)));
    for (let widget of widgetsAffected) {
      this.adjustPosition(widget, aMoveForwards);
    }
    this.adjusting = false;
  },
  
  
  
  adjustPosition: function(aWidgetId, aMoveForwards) {
    
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
      let desiredChange = -(prevSiblingCount % kColumnsInMenuPanel);
      if (aMoveForwards && fixedPos == null) {
        
        desiredChange = kColumnsInMenuPanel + desiredChange + 1;
      }
      desiredPos += desiredChange;
      CustomizableUI.moveWidgetWithinArea(aWidgetId, desiredPos);
    }
  },
  init: function() {
    
    gPanelPlacements = CustomizableUI.getWidgetIdsInArea(gPanel);
    CustomizableUI.addListener(this);
  },
};
