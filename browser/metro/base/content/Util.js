



Components.utils.import("resource:///modules/ContentUtil.jsm");

let Util = {
  



  getWindowUtils: function getWindowUtils(aWindow) {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
  },

  
  
  forceOnline: function forceOnline() {
    Services.io.offline = false;
  },

  



  
  executeSoon: function executeSoon(aFunc) {
    Services.tm.mainThread.dispatch({
      run: function() {
        aFunc();
      }
    }, Ci.nsIThread.DISPATCH_NORMAL);
  },

  



  
  dumpLn: function dumpLn() {
    for (let i = 0; i < arguments.length; i++)
      dump(arguments[i] + " ");
    dump("\n");
  },

  



  transitionElementVisibility: function(aNodes, aVisible) {
    
    aNodes = aNodes.length ? aNodes : [aNodes];
    let defd = Promise.defer();
    let pending = 0;
    Array.forEach(aNodes, function(aNode) {
      if (aVisible) {
        aNode.hidden = false;
        aNode.removeAttribute("fade"); 
      } else {
        aNode.setAttribute("fade", true); 
      }
      aNode.addEventListener("transitionend", function onTransitionEnd(aEvent){
        aNode.removeEventListener("transitionend", onTransitionEnd);
        if (!aVisible) {
          aNode.hidden = true;
        }
        pending--;
        if (!pending){
          defd.resolve(true);
        }
      }, false);
      pending++;
    });
    return defd.promise;
  },

  isTextInput: function isTextInput(aElement) {
    return ((aElement instanceof Ci.nsIDOMHTMLInputElement &&
             aElement.mozIsTextField(false)) ||
            aElement instanceof Ci.nsIDOMHTMLTextAreaElement);
  },

  isEditableContent: function isEditableContent(aElement) {
    if (!aElement)
      return false;
    if (aElement.isContentEditable || aElement.designMode == "on")
      return true;
    return false;
  },

  isEditable: function isEditable(aElement) {
    if (!aElement)
      return false;
    if (this.isTextInput(aElement) || this.isEditableContent(aElement))
      return true;

    
    
    if ((aElement instanceof Ci.nsIDOMHTMLIFrameElement ||
         aElement instanceof Ci.nsIDOMHTMLDivElement) &&
        aElement.contentDocument &&
        this.isEditableContent(aElement.contentDocument.body)) {
      return true;
    }

    return aElement.ownerDocument && aElement.ownerDocument.designMode == "on";
  },

  isMultilineInput: function isMultilineInput(aElement) {
    return (aElement instanceof Ci.nsIDOMHTMLTextAreaElement);
  },

  isLink: function isLink(aElement) {
    return ((aElement instanceof Ci.nsIDOMHTMLAnchorElement && aElement.href) ||
            (aElement instanceof Ci.nsIDOMHTMLAreaElement && aElement.href) ||
            aElement instanceof Ci.nsIDOMHTMLLinkElement ||
            aElement.getAttributeNS(kXLinkNamespace, "type") == "simple");
  },

  isText: function isText(aElement) {
    return (aElement instanceof Ci.nsIDOMHTMLParagraphElement ||
            aElement instanceof Ci.nsIDOMHTMLDivElement ||
            aElement instanceof Ci.nsIDOMHTMLLIElement ||
            aElement instanceof Ci.nsIDOMHTMLPreElement ||
            aElement instanceof Ci.nsIDOMHTMLHeadingElement ||
            aElement instanceof Ci.nsIDOMHTMLTableCellElement ||
            aElement instanceof Ci.nsIDOMHTMLBodyElement);
  },

  



  getCleanRect: function getCleanRect() {
    return {
      left: 0, top: 0, right: 0, bottom: 0
    };
  },

  pointWithinRect: function pointWithinRect(aX, aY, aRect) {
    return (aRect.left < aX && aRect.top < aY &&
            aRect.right > aX && aRect.bottom > aY);
  },

  pointWithinDOMRect: function pointWithinDOMRect(aX, aY, aRect) {
    if (!aRect.width || !aRect.height)
      return false;
    return this.pointWithinRect(aX, aY, aRect);
  },

  isEmptyDOMRect: function isEmptyDOMRect(aRect) {
    if ((aRect.bottom - aRect.top) <= 0 &&
        (aRect.right - aRect.left) <= 0)
      return true;
    return false;
  },

  
  dumpDOMRect: function dumpDOMRect(aMsg, aRect) {
    try {
      Util.dumpLn(aMsg,
                  "left:" + Math.round(aRect.left) + ",",
                  "top:" + Math.round(aRect.top) + ",",
                  "right:" + Math.round(aRect.right) + ",",
                  "bottom:" + Math.round(aRect.bottom) + ",",
                  "width:" + Math.round(aRect.right - aRect.left) + ",",
                  "height:" + Math.round(aRect.bottom - aRect.top) );
    } catch (ex) {
      Util.dumpLn("dumpDOMRect:", ex.message);
    }
  },

  



  getDownloadSize: function dv__getDownloadSize (aSize) {
    let [size, units] = DownloadUtils.convertByteUnits(aSize);
    if (aSize > 0)
      return size + units;
    else
      return Strings.browser.GetStringFromName("downloadsUnknownSize");
  },

  



  makeURI: function makeURI(aURL, aOriginCharset, aBaseURI) {
    return Services.io.newURI(aURL, aOriginCharset, aBaseURI);
  },

  makeURLAbsolute: function makeURLAbsolute(base, url) {
    
    return this.makeURI(url, null, this.makeURI(base)).spec;
  },

  isLocalScheme: function isLocalScheme(aURL) {
    return ((aURL.indexOf("about:") == 0 &&
             aURL != "about:blank" &&
             aURL != "about:empty" &&
             aURL != "about:start") ||
            aURL.indexOf("chrome:") == 0);
  },

  
  isURLEmpty: function isURLEmpty(aURL) {
    return (!aURL ||
            aURL == "about:blank" ||
            aURL == "about:empty" ||
            aURL == "about:home" ||
            aURL == "about:start");
  },

  
  getEmptyURLTabTitle: function getEmptyURLTabTitle() {
    let browserStrings = Services.strings.createBundle("chrome://browser/locale/browser.properties");

    return browserStrings.GetStringFromName("tabs.emptyTabTitle");
  },

  
  isURLMemorable: function isURLMemorable(aURL) {
    return !(aURL == "about:blank" ||
             aURL == "about:empty" ||
             aURL == "about:start");
  },

  



  clamp: function(num, min, max) {
    return Math.max(min, Math.min(max, num));
  },

  



   



  translateToTopLevelWindow: function translateToTopLevelWindow(aElement) {
    let offsetX = 0;
    let offsetY = 0;
    let element = aElement;
    while (element &&
           element.ownerDocument &&
           element.ownerDocument.defaultView != content) {
      element = element.ownerDocument.defaultView.frameElement;
      let rect = element.getBoundingClientRect();
      offsetX += rect.left;
      offsetY += rect.top;
    }
    let win = null;
    if (element == aElement)
      win = content;
    else
      win = element.contentDocument.defaultView;
    return { targetWindow: win, offsetX: offsetX, offsetY: offsetY };
  },

  get displayDPI() {
    delete this.displayDPI;
    return this.displayDPI = this.getWindowUtils(window).displayDPI;
  },

  






  centerElementInView: function centerElementInView(aViewHeight, aRect) {
    
    
    if (aRect.bottom <= aViewHeight) {
      return 0;
    }

    
    let targetHeight = aRect.bottom - aRect.top;
    
    let viewBottom = content.innerHeight;

    
    
    if (targetHeight <= aViewHeight) {
      
      
      
      
      
      
      let splitMargin = Math.round((aViewHeight - targetHeight) * .5);
      let distanceToPageBounds = viewBottom - aRect.bottom;
      let distanceFromChromeTop = aRect.bottom - aViewHeight;
      let distanceToCenter =
        distanceFromChromeTop + Math.min(distanceToPageBounds, splitMargin);
      return distanceToCenter;
    }
  },

  



  copyImageToClipboard: function Util_copyImageToClipboard(aImageLoadingContent) {
    let image = aImageLoadingContent.QueryInterface(Ci.nsIImageLoadingContent);
    if (!image) {
      Util.dumpLn("copyImageToClipboard error: image is not an nsIImageLoadingContent");
      return;
    }
    try {
      let xferable = Cc["@mozilla.org/widget/transferable;1"].createInstance(Ci.nsITransferable);
      xferable.init(null);
      let imgRequest = aImageLoadingContent.getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
      let mimeType = imgRequest.mimeType;
      let imgContainer = imgRequest.image;
      let imgPtr = Cc["@mozilla.org/supports-interface-pointer;1"].createInstance(Ci.nsISupportsInterfacePointer);
      imgPtr.data = imgContainer;
      xferable.setTransferData(mimeType, imgPtr, null);
      let clip = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
      clip.setData(xferable, null, Ci.nsIClipboard.kGlobalClipboard);
    } catch (e) {
      Util.dumpLn(e.message);
    }
  },
};








Util.Timeout = function(aCallback) {
  this._callback = aCallback;
  this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this._type = null;
};

Util.Timeout.prototype = {
  
  notify: function notify() {
    if (this._type == this._timer.TYPE_ONE_SHOT)
      this._type = null;

    if (this._callback.notify)
      this._callback.notify();
    else
      this._callback.apply(null);
  },

  
  _start: function _start(aDelay, aType, aCallback) {
    if (aCallback)
      this._callback = aCallback;
    this.clear();
    this._timer.initWithCallback(this, aDelay, aType);
    this._type = aType;
    return this;
  },

  
  once: function once(aDelay, aCallback) {
    return this._start(aDelay, this._timer.TYPE_ONE_SHOT, aCallback);
  },

  
  interval: function interval(aDelay, aCallback) {
    return this._start(aDelay, this._timer.TYPE_REPEATING_SLACK, aCallback);
  },

  
  clear: function clear() {
    if (this.isPending()) {
      this._timer.cancel();
      this._type = null;
    }
    return this;
  },

  
  flush: function flush() {
    if (this.isPending()) {
      this.notify();
      this.clear();
    }
    return this;
  },

  
  isPending: function isPending() {
    return this._type !== null;
  }
};


{
  for (let name in ContentUtil) {
    let copy = ContentUtil[name];
    if (copy !== undefined)
      Util[name] = copy;
  }
}
