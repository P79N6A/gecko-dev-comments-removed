





const Cu = Components.utils;
const Ci = Components.interfaces;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = ["LayoutHelpers"];

let LayoutHelpers = function(aTopLevelWindow) {
  this._topDocShell = aTopLevelWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                     .getInterface(Ci.nsIWebNavigation)
                                     .QueryInterface(Ci.nsIDocShell);
};

this.LayoutHelpers = LayoutHelpers;

LayoutHelpers.prototype = {

  









  getAdjustedQuads: function(node, region) {
    if (!node || !node.getBoxQuads) {
      return [];
    }

    let quads = node.getBoxQuads({
      box: region
    });

    if (!quads.length) {
      return [];
    }

    let [xOffset, yOffset] = this.getFrameOffsets(node);
    let scale = LayoutHelpers.getCurrentZoom(node);

    let adjustedQuads = [];
    for (let quad of quads) {
      adjustedQuads.push({
        p1: {
          w: quad.p1.w * scale,
          x: quad.p1.x * scale + xOffset,
          y: quad.p1.y * scale + yOffset,
          z: quad.p1.z * scale
        },
        p2: {
          w: quad.p2.w * scale,
          x: quad.p2.x * scale + xOffset,
          y: quad.p2.y * scale + yOffset,
          z: quad.p2.z * scale
        },
        p3: {
          w: quad.p3.w * scale,
          x: quad.p3.x * scale + xOffset,
          y: quad.p3.y * scale + yOffset,
          z: quad.p3.z * scale
        },
        p4: {
          w: quad.p4.w * scale,
          x: quad.p4.x * scale + xOffset,
          y: quad.p4.y * scale + yOffset,
          z: quad.p4.z * scale
        },
        bounds: {
          bottom: quad.bounds.bottom * scale + yOffset,
          height: quad.bounds.height * scale,
          left: quad.bounds.left * scale + xOffset,
          right: quad.bounds.right * scale + xOffset,
          top: quad.bounds.top * scale + yOffset,
          width: quad.bounds.width * scale,
          x: quad.bounds.x * scale + xOffset,
          y: quad.bounds.y * scale + yOffset
        }
      });
    }

    return adjustedQuads;
  },

  










  getRect: function(aNode, aContentWindow) {
    let frameWin = aNode.ownerDocument.defaultView;
    let clientRect = aNode.getBoundingClientRect();

    
    
    let rect = {top: clientRect.top + aContentWindow.pageYOffset,
            left: clientRect.left + aContentWindow.pageXOffset,
            width: clientRect.width,
            height: clientRect.height};

    
    while (true) {
      
      if (this.isTopLevelWindow(frameWin)) {
        break;
      }

      let frameElement = this.getFrameElement(frameWin);
      if (!frameElement) {
        break;
      }

      
      
      
      let frameRect = frameElement.getBoundingClientRect();

      let [offsetTop, offsetLeft] =
        this.getIframeContentOffset(frameElement);

      rect.top += frameRect.top + offsetTop;
      rect.left += frameRect.left + offsetLeft;

      frameWin = this.getParentWindow(frameWin);
    }

    return rect;
  },

  













  getIframeContentOffset: function(aIframe) {
    let style = aIframe.contentWindow.getComputedStyle(aIframe, null);

    
    if (!style) {
      return [0, 0];
    }

    let paddingTop = parseInt(style.getPropertyValue("padding-top"));
    let paddingLeft = parseInt(style.getPropertyValue("padding-left"));

    let borderTop = parseInt(style.getPropertyValue("border-top-width"));
    let borderLeft = parseInt(style.getPropertyValue("border-left-width"));

    return [borderTop + paddingTop, borderLeft + paddingLeft];
  },

  










  getElementFromPoint: function(aDocument, aX, aY) {
    let node = aDocument.elementFromPoint(aX, aY);
    if (node && node.contentDocument) {
      if (node instanceof Ci.nsIDOMHTMLIFrameElement) {
        let rect = node.getBoundingClientRect();

        
        let [offsetTop, offsetLeft] = this.getIframeContentOffset(node);

        aX -= rect.left + offsetLeft;
        aY -= rect.top + offsetTop;

        if (aX < 0 || aY < 0) {
          
          return node;
        }
      }
      if (node instanceof Ci.nsIDOMHTMLIFrameElement ||
          node instanceof Ci.nsIDOMHTMLFrameElement) {
        let subnode = this.getElementFromPoint(node.contentDocument, aX, aY);
        if (subnode) {
          node = subnode;
        }
      }
    }
    return node;
  },

  









  scrollIntoViewIfNeeded: function(elem, centered) {
    
    
    centered = centered === undefined? true: !!centered;

    let win = elem.ownerDocument.defaultView;
    let clientRect = elem.getBoundingClientRect();

    
    
    
    

    let topToBottom = clientRect.bottom;
    let bottomToTop = clientRect.top - win.innerHeight;
    let leftToRight = clientRect.right;
    let rightToLeft = clientRect.left - win.innerWidth;
    let xAllowed = true;  
    let yAllowed = true;  

    
    

    if ((topToBottom > 0 || !centered) && topToBottom <= elem.offsetHeight) {
      win.scrollBy(0, topToBottom - elem.offsetHeight);
      yAllowed = false;
    } else
    if ((bottomToTop < 0 || !centered) && bottomToTop >= -elem.offsetHeight) {
      win.scrollBy(0, bottomToTop + elem.offsetHeight);
      yAllowed = false;
    }

    if ((leftToRight > 0 || !centered) && leftToRight <= elem.offsetWidth) {
      if (xAllowed) {
        win.scrollBy(leftToRight - elem.offsetWidth, 0);
        xAllowed = false;
      }
    } else
    if ((rightToLeft < 0 || !centered) && rightToLeft >= -elem.offsetWidth) {
      if (xAllowed) {
        win.scrollBy(rightToLeft + elem.offsetWidth, 0);
        xAllowed = false;
      }
    }

    
    

    if (centered) {

      if (yAllowed && (topToBottom <= 0 || bottomToTop >= 0)) {
        win.scroll(win.scrollX,
                   win.scrollY + clientRect.top
                   - (win.innerHeight - elem.offsetHeight) / 2);
      }

      if (xAllowed && (leftToRight <= 0 || rightToLeft <= 0)) {
        win.scroll(win.scrollX + clientRect.left
                   - (win.innerWidth - elem.offsetWidth) / 2,
                   win.scrollY);
      }
    }

    if (!this.isTopLevelWindow(win)) {
      
      let frameElement = this.getFrameElement(win);
      this.scrollIntoViewIfNeeded(frameElement, centered);
    }
  },

  






  isNodeConnected: function(aNode) {
    try {
      let connected = (aNode.ownerDocument && aNode.ownerDocument.defaultView &&
                      !(aNode.compareDocumentPosition(aNode.ownerDocument.documentElement) &
                      aNode.DOCUMENT_POSITION_DISCONNECTED));
      return connected;
    } catch (e) {
      
      return false;
    }
  },

  





  isTopLevelWindow: function(win) {
    let docShell = win.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIWebNavigation)
                   .QueryInterface(Ci.nsIDocShell);

    return docShell === this._topDocShell;
  },

  





  isIncludedInTopLevelWindow: function LH_isIncludedInTopLevelWindow(win) {
    if (this.isTopLevelWindow(win)) {
      return true;
    }

    let parent = this.getParentWindow(win);
    if (!parent || parent === win) {
      return false;
    }

    return this.isIncludedInTopLevelWindow(parent);
  },

  





  getParentWindow: function(win) {
    if (this.isTopLevelWindow(win)) {
      return null;
    }

    let docShell = win.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIWebNavigation)
                   .QueryInterface(Ci.nsIDocShell);

    if (docShell.isBrowserOrApp) {
      let parentDocShell = docShell.getSameTypeParentIgnoreBrowserAndAppBoundaries();
      return parentDocShell ? parentDocShell.contentViewer.DOMDocument.defaultView : null;
    } else {
      return win.parent;
    }
  },

  







  getFrameElement: function(win) {
    if (this.isTopLevelWindow(win)) {
      return null;
    }

    let winUtils = win.
      QueryInterface(Components.interfaces.nsIInterfaceRequestor).
      getInterface(Components.interfaces.nsIDOMWindowUtils);

    return winUtils.containerElement;
  },

  







  getFrameOffsets: function(node) {
    let xOffset = 0;
    let yOffset = 0;
    let frameWin = node.ownerDocument.defaultView;
    let scale = LayoutHelpers.getCurrentZoom(node);

    while (true) {
      
      if (this.isTopLevelWindow(frameWin)) {
        break;
      }

      let frameElement = this.getFrameElement(frameWin);
      if (!frameElement) {
        break;
      }

      
      
      
      let frameRect = frameElement.getBoundingClientRect();

      let [offsetTop, offsetLeft] =
        this.getIframeContentOffset(frameElement);

      xOffset += frameRect.left + offsetLeft;
      yOffset += frameRect.top + offsetTop;

      frameWin = this.getParentWindow(frameWin);
    }

    return [xOffset * scale, yOffset * scale];
  },

  







  getNodeBounds: function(node) {
    if (!node) {
      return;
    }

    let scale = LayoutHelpers.getCurrentZoom(node);

    
    let offsetLeft = 0;
    let offsetTop = 0;
    let el = node;
    while (el && el.parentNode) {
      offsetLeft += el.offsetLeft;
      offsetTop += el.offsetTop;
      el = el.offsetParent;
    }

    
    el = node;
    while (el && el.parentNode) {
      if (el.scrollTop) {
        offsetTop -= el.scrollTop;
      }
      if (el.scrollLeft) {
        offsetLeft -= el.scrollLeft;
      }
      el = el.parentNode;
    }

    
    let [xOffset, yOffset] = this.getFrameOffsets(node);
    xOffset += offsetLeft;
    yOffset += offsetTop;

    xOffset *= scale;
    yOffset *= scale;

    
    let width = node.offsetWidth * scale;
    let height = node.offsetHeight * scale;

    return {
      p1: {x: xOffset, y: yOffset},
      p2: {x: xOffset + width, y: yOffset},
      p3: {x: xOffset + width, y: yOffset + height},
      p4: {x: xOffset, y: yOffset + height}
    };
  }
};












LayoutHelpers.getRootBindingParent = function(node) {
  let parent;
  let doc = node.ownerDocument;
  if (!doc) {
    return node;
  }
  while ((parent = doc.getBindingParent(node))) {
    node = parent;
  }
  return node;
};

LayoutHelpers.getBindingParent = function(node) {
  let doc = node.ownerDocument;
  if (!doc) {
    return false;
  }

  
  let parent = doc.getBindingParent(node);
  if (!parent) {
    return false;
  }

  return parent;
}








LayoutHelpers.isAnonymous = function(node) {
  return LayoutHelpers.getRootBindingParent(node) !== node;
};











LayoutHelpers.isNativeAnonymous = function(node) {
  if (!LayoutHelpers.getBindingParent(node)) {
    return false;
  }
  return !LayoutHelpers.isXBLAnonymous(node) &&
         !LayoutHelpers.isShadowAnonymous(node);
};










LayoutHelpers.isXBLAnonymous = function(node) {
  let parent = LayoutHelpers.getBindingParent(node);
  if (!parent) {
    return false;
  }

  
  if (parent.shadowRoot && parent.shadowRoot.contains(node)) {
    return false;
  }

  let anonNodes = [...node.ownerDocument.getAnonymousNodes(parent) || []];
  return anonNodes.indexOf(node) > -1;
};








LayoutHelpers.isShadowAnonymous = function(node) {
  let parent = LayoutHelpers.getBindingParent(node);
  if (!parent) {
    return false;
  }

  
  
  return parent.shadowRoot && parent.shadowRoot.contains(node);
};









let windowUtils = new WeakMap;
LayoutHelpers.getCurrentZoom = function(node, map = z=>z) {
  let win = node.ownerDocument.defaultView;
  let utils = windowUtils.get(win);
  if (utils) {
    return utils.fullZoom;
  }

  utils = win.QueryInterface(Ci.nsIInterfaceRequestor)
             .getInterface(Ci.nsIDOMWindowUtils);
  windowUtils.set(win, utils);
  return utils.fullZoom;
};
