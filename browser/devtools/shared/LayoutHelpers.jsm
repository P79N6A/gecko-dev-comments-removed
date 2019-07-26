





const Cu = Components.utils;
const Ci = Components.interfaces;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "PlatformKeys", function() {
  return Services.strings.createBundle(
    "chrome://global-platform/locale/platformKeys.properties");
});

var EXPORTED_SYMBOLS = ["LayoutHelpers"];

LayoutHelpers = {

  






  getDirtyRect: function LH_getDirectyRect(aNode) {
    let frameWin = aNode.ownerDocument.defaultView;
    let clientRect = aNode.getBoundingClientRect();

    
    
    rect = {top: clientRect.top,
            left: clientRect.left,
            width: clientRect.width,
            height: clientRect.height};

    
    while (true) {

      
      let diffx = frameWin.innerWidth - (rect.left + rect.width);
      if (diffx < 0) {
        rect.width += diffx;
      }

      
      let diffy = frameWin.innerHeight - (rect.top + rect.height);
      if (diffy < 0) {
        rect.height += diffy;
      }

      
      if (rect.left < 0) {
        rect.width += rect.left;
        rect.left = 0;
      }

      
      if (rect.top < 0) {
        rect.height += rect.top;
        rect.top = 0;
      }

      

      
      if (frameWin.parent === frameWin || !frameWin.frameElement) {
        break;
      }

      
      
      
      let frameRect = frameWin.frameElement.getBoundingClientRect();

      let [offsetTop, offsetLeft] =
        this.getIframeContentOffset(frameWin.frameElement);

      rect.top += frameRect.top + offsetTop;
      rect.left += frameRect.left + offsetLeft;

      frameWin = frameWin.parent;
    }

    return rect;
  },

  








  getRect: function LH_getRect(aNode, aContentWindow) {
    let frameWin = aNode.ownerDocument.defaultView;
    let clientRect = aNode.getBoundingClientRect();

    
    
    rect = {top: clientRect.top + aContentWindow.pageYOffset,
            left: clientRect.left + aContentWindow.pageXOffset,
            width: clientRect.width,
            height: clientRect.height};

    
    while (true) {

      
      if (frameWin.parent === frameWin || !frameWin.frameElement) {
        break;
      }

      
      
      
      let frameRect = frameWin.frameElement.getBoundingClientRect();

      let [offsetTop, offsetLeft] =
        this.getIframeContentOffset(frameWin.frameElement);

      rect.top += frameRect.top + offsetTop;
      rect.left += frameRect.left + offsetLeft;

      frameWin = frameWin.parent;
    }

    return rect;
  },

  













  getIframeContentOffset: function LH_getIframeContentOffset(aIframe) {
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

  


  getZoomedRect: function LH_getZoomedRect(aWin, aRect) {
    
    let zoom =
      aWin.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
        .getInterface(Components.interfaces.nsIDOMWindowUtils)
        .screenPixelsPerCSSPixel;

    
    let aRectScaled = {};
    for (let prop in aRect) {
      aRectScaled[prop] = aRect[prop] * zoom;
    }

    return aRectScaled;
  },


  








  getElementFromPoint: function LH_elementFromPoint(aDocument, aX, aY) {
    let node = aDocument.elementFromPoint(aX, aY);
    if (node && node.contentDocument) {
      if (node instanceof Ci.nsIDOMHTMLIFrameElement) {
        let rect = node.getBoundingClientRect();

        
        let [offsetTop, offsetLeft] = LayoutHelpers.getIframeContentOffset(node);

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

  







  scrollIntoViewIfNeeded:
  function LH_scrollIntoViewIfNeeded(elem, centered) {
    
    
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

    if (win.parent !== win) {
      
      LH_scrollIntoViewIfNeeded(win.frameElement, centered);
    }
  },

  





  isNodeConnected: function LH_isNodeConnected(aNode)
  {
    try {
      let connected = (aNode.ownerDocument && aNode.ownerDocument.defaultView &&
                      !(aNode.compareDocumentPosition(aNode.ownerDocument.documentElement) &
                      aNode.DOCUMENT_POSITION_DISCONNECTED));
      return connected;
    } catch (e) {
      
      return false;
    }
  },

  







  prettyKey: function LH_prettyKey(aElemKey)
  {
    let elemString = "";
    let elemMod = aElemKey.getAttribute("modifiers");

    if (elemMod.match("accel")) {
      if (Services.appinfo.OS == "Darwin") {
        
        
        
        
        elemString += "Cmd-";
      } else {
        elemString += PlatformKeys.GetStringFromName("VK_CONTROL") +
                      PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
      }
    }
    if (elemMod.match("access")) {
      if (Services.appinfo.OS == "Darwin") {
        elemString += PlatformKeys.GetStringFromName("VK_CONTROL") +
                      PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
      } else {
        elemString += PlatformKeys.GetStringFromName("VK_ALT") +
                      PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
      }
    }
    if (elemMod.match("shift")) {
      elemString += PlatformKeys.GetStringFromName("VK_SHIFT") +
                    PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
    }
    if (elemMod.match("alt")) {
      elemString += PlatformKeys.GetStringFromName("VK_ALT") +
                    PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
    }
    if (elemMod.match("ctrl")) {
      elemString += PlatformKeys.GetStringFromName("VK_CONTROL") +
                    PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
    }
    if (elemMod.match("meta")) {
      elemString += PlatformKeys.GetStringFromName("VK_META") +
                    PlatformKeys.GetStringFromName("MODIFIER_SEPARATOR");
    }

    return elemString +
      (aElemKey.getAttribute("keycode").replace(/^.*VK_/, "") ||
       aElemKey.getAttribute("key")).toUpperCase();
  }
};
