


"use strict";

var ZoomHelper = {
  zoomInAndSnapToRange: function(aRange) {
    
    let viewport = BrowserApp.selectedTab.getViewport();
    let fudge = 15; 
    let boundingElement = aRange.offsetNode;
    while (!boundingElement.getBoundingClientRect && boundingElement.parentNode) {
      boundingElement = boundingElement.parentNode;
    }

    let rect = ElementTouchHelper.getBoundingContentRect(boundingElement);
    let drRect = aRange.getClientRect();
    let scrollTop =
      BrowserApp.selectedBrowser.contentDocument.documentElement.scrollTop ||
      BrowserApp.selectedBrowser.contentDocument.body.scrollTop;

    
    
    let topPos = scrollTop + drRect.top - (viewport.cssHeight / 2.0);

    
    let boundingStyle = window.getComputedStyle(boundingElement);
    let leftAdjustment = parseInt(boundingStyle.paddingLeft) +
                         parseInt(boundingStyle.borderLeftWidth);

    BrowserApp.selectedTab._mReflozPositioned = true;

    rect.type = "Browser:ZoomToRect";
    rect.x = Math.max(viewport.cssPageLeft, rect.x  - fudge + leftAdjustment);
    rect.y = Math.max(topPos, viewport.cssPageTop);
    rect.w = viewport.cssWidth;
    rect.h = viewport.cssHeight;
    rect.animate = false;

    Messaging.sendRequest(rect);
    BrowserApp.selectedTab._mReflozPoint = null;
  },

  zoomOut: function() {
    BrowserEventHandler.resetMaxLineBoxWidth();
    Messaging.sendRequest({ type: "Browser:ZoomToPageWidth" });
  },

  isRectZoomedIn: function(aRect, aViewport) {
    
    
    
    
    
    
    const minDifference = -20;
    const maxDifference = 20;
    const maxZoomAllowed = 4; 

    let vRect = new Rect(aViewport.cssX, aViewport.cssY, aViewport.cssWidth, aViewport.cssHeight);
    let overlap = vRect.intersect(aRect);
    let overlapArea = overlap.width * overlap.height;
    let availHeight = Math.min(aRect.width * vRect.height / vRect.width, aRect.height);
    let showing = overlapArea / (aRect.width * availHeight);
    let dw = (aRect.width - vRect.width);
    let dx = (aRect.x - vRect.x);

    if (fuzzyEquals(aViewport.zoom, maxZoomAllowed) && overlap.width / aRect.width > 0.9) {
      
      
      return true;
    }

    return (showing > 0.9 &&
            dx > minDifference && dx < maxDifference &&
            dw > minDifference && dw < maxDifference);
  },

  


  zoomToElement: function(aElement, aClickY = -1, aCanZoomOut = true, aCanScrollHorizontally = true) {
    let rect = ElementTouchHelper.getBoundingContentRect(aElement);
    ZoomHelper.zoomToRect(rect, aClickY, aCanZoomOut, aCanScrollHorizontally, aElement);
  },

  zoomToRect: function(aRect, aClickY = -1, aCanZoomOut = true, aCanScrollHorizontally = true, aElement) {
    const margin = 15;

    if(!aRect.h || !aRect.w) {
      aRect.h = aRect.height;
      aRect.w = aRect.width;
    }

    let viewport = BrowserApp.selectedTab.getViewport();
    let bRect = new Rect(aCanScrollHorizontally ? Math.max(viewport.cssPageLeft, aRect.x - margin) : viewport.cssX,
                         aRect.y,
                         aCanScrollHorizontally ? aRect.w + 2 * margin : viewport.cssWidth,
                         aRect.h);
    
    bRect.width = Math.min(bRect.width, viewport.cssPageRight - bRect.x);

    
    
    if (aElement) {
      if (BrowserEventHandler.mReflozPref) {
        let zoomFactor = BrowserApp.selectedTab.getZoomToMinFontSize(aElement);

        bRect.width = zoomFactor <= 1.0 ? bRect.width : gScreenWidth / zoomFactor;
        bRect.height = zoomFactor <= 1.0 ? bRect.height : bRect.height / zoomFactor;
        if (zoomFactor == 1.0 || ZoomHelper.isRectZoomedIn(bRect, viewport)) {
          if (aCanZoomOut) {
            ZoomHelper.zoomOut();
          }
          return;
        }
      } else if (ZoomHelper.isRectZoomedIn(bRect, viewport)) {
        if (aCanZoomOut) {
          ZoomHelper.zoomOut();
        }
        return;
      }
    }

    let rect = {};

    rect.type = "Browser:ZoomToRect";
    rect.x = bRect.x;
    rect.y = bRect.y;
    rect.w = bRect.width;
    rect.h = Math.min(bRect.width * viewport.cssHeight / viewport.cssWidth, bRect.height);

    if (aClickY >= 0) {
      
      
      
      
      let cssTapY = viewport.cssY + aClickY;
      if ((bRect.height > rect.h) && (cssTapY > rect.y + (rect.h * 1.2))) {
        rect.y = cssTapY - (rect.h / 2);
      }
    }

    if (rect.w > viewport.cssWidth || rect.h > viewport.cssHeight) {
      BrowserEventHandler.resetMaxLineBoxWidth();
    }

    Messaging.sendRequest(rect);
  },
};
