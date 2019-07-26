





dump("### Content.js loaded\n");

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;
let Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "Services", function() {
  Cu.import("resource://gre/modules/Services.jsm");
  return Services;
});

XPCOMUtils.defineLazyGetter(this, "Rect", function() {
  Cu.import("resource://gre/modules/Geometry.jsm");
  return Rect;
});

XPCOMUtils.defineLazyGetter(this, "Point", function() {
  Cu.import("resource://gre/modules/Geometry.jsm");
  return Point;
});

XPCOMUtils.defineLazyServiceGetter(this, "gFocusManager",
  "@mozilla.org/focus-manager;1", "nsIFocusManager");

XPCOMUtils.defineLazyServiceGetter(this, "gDOMUtils",
  "@mozilla.org/inspector/dom-utils;1", "inIDOMUtils");

let XULDocument = Ci.nsIDOMXULDocument;
let HTMLHtmlElement = Ci.nsIDOMHTMLHtmlElement;
let HTMLIFrameElement = Ci.nsIDOMHTMLIFrameElement;
let HTMLFrameElement = Ci.nsIDOMHTMLFrameElement;
let HTMLFrameSetElement = Ci.nsIDOMHTMLFrameSetElement;
let HTMLSelectElement = Ci.nsIDOMHTMLSelectElement;
let HTMLOptionElement = Ci.nsIDOMHTMLOptionElement;

const kReferenceDpi = 240; 

const kStateActive = 0x00000001; 







const ElementTouchHelper = {
  get radius() {
    let prefs = Services.prefs;
    delete this.radius;
    return this.radius = { "top": prefs.getIntPref("ui.touch.radius.topmm"),
                           "right": prefs.getIntPref("ui.touch.radius.rightmm"),
                           "bottom": prefs.getIntPref("ui.touch.radius.bottommm"),
                           "left": prefs.getIntPref("ui.touch.radius.leftmm")
                         };
  },

  get weight() {
    delete this.weight;
    return this.weight = { "visited": Services.prefs.getIntPref("ui.touch.radius.visitedWeight")
                         };
  },

  
  getClosest: function getClosest(aWindowUtils, aX, aY) {
    if (!this.dpiRatio)
      this.dpiRatio = aWindowUtils.displayDPI / kReferenceDpi;

    let dpiRatio = this.dpiRatio;

    let target = aWindowUtils.elementFromPoint(aX, aY,
                                               true,   
                                               false); 

    
    if (this._isElementClickable(target))
      return target;

    let nodes = aWindowUtils.nodesFromRect(aX, aY, this.radius.top * dpiRatio,
                                                   this.radius.right * dpiRatio,
                                                   this.radius.bottom * dpiRatio,
                                                   this.radius.left * dpiRatio, true, false);

    let threshold = Number.POSITIVE_INFINITY;
    for (let i = 0; i < nodes.length; i++) {
      let current = nodes[i];
      if (!current.mozMatchesSelector || !this._isElementClickable(current))
        continue;

      let rect = current.getBoundingClientRect();
      let distance = this._computeDistanceFromRect(aX, aY, rect);

      
      if (current && current.mozMatchesSelector("*:visited"))
        distance *= (this.weight.visited / 100);

      if (distance < threshold) {
        target = current;
        threshold = distance;
      }
    }

    return target;
  },

  _isElementClickable: function _isElementClickable(aElement) {
    const selector = "a,:link,:visited,[role=button],button,input,select,textarea,label";
    for (let elem = aElement; elem; elem = elem.parentNode) {
      if (this._hasMouseListener(elem))
        return true;
      if (elem.mozMatchesSelector && elem.mozMatchesSelector(selector))
        return true;
    }
    return false;
  },

  _computeDistanceFromRect: function _computeDistanceFromRect(aX, aY, aRect) {
    let x = 0, y = 0;
    let xmost = aRect.left + aRect.width;
    let ymost = aRect.top + aRect.height;

    
    
    if (aRect.left < aX && aX < xmost)
      x = Math.min(xmost - aX, aX - aRect.left);
    else if (aX < aRect.left)
      x = aRect.left - aX;
    else if (aX > xmost)
      x = aX - xmost;

    
    
    if (aRect.top < aY && aY < ymost)
      y = Math.min(ymost - aY, aY - aRect.top);
    else if (aY < aRect.top)
      y = aRect.top - aY;
    if (aY > ymost)
      y = aY - ymost;

    return Math.sqrt(Math.pow(x, 2) + Math.pow(y, 2));
  },

  _els: Cc["@mozilla.org/eventlistenerservice;1"].getService(Ci.nsIEventListenerService),
  _clickableEvents: ["mousedown", "mouseup", "click"],
  _hasMouseListener: function _hasMouseListener(aElement) {
    let els = this._els;
    let listeners = els.getListenerInfoFor(aElement, {});
    for (let i = 0; i < listeners.length; i++) {
      if (this._clickableEvents.indexOf(listeners[i].type) != -1)
        return true;
    }
    return false;
  }
};



















function elementFromPoint(aX, aY) {
  
  
  let cwu = Util.getWindowUtils(content);
  let elem = ElementTouchHelper.getClosest(cwu, aX, aY);

  
  while (elem && (elem instanceof HTMLIFrameElement ||
                  elem instanceof HTMLFrameElement)) {
    
    let rect = elem.getBoundingClientRect();
    aX -= rect.left;
    aY -= rect.top;
    let windowUtils = elem.contentDocument
                          .defaultView
                          .QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);
    elem = ElementTouchHelper.getClosest(windowUtils, aX, aY);
  }
  return { element: elem, frameX: aX, frameY: aY };
}







function getBoundingContentRect(aElement) {
  if (!aElement)
    return new Rect(0, 0, 0, 0);

  let document = aElement.ownerDocument;
  while(document.defaultView.frameElement)
    document = document.defaultView.frameElement.ownerDocument;

  let offset = ContentScroll.getScrollOffset(content);
  offset = new Point(offset.x, offset.y);

  let r = aElement.getBoundingClientRect();

  
  let view = aElement.ownerDocument.defaultView;
  for (let frame = view; frame != content; frame = frame.parent) {
    
    let rect = frame.frameElement.getBoundingClientRect();
    let left = frame.getComputedStyle(frame.frameElement, "").borderLeftWidth;
    let top = frame.getComputedStyle(frame.frameElement, "").borderTopWidth;
    offset.add(rect.left + parseInt(left), rect.top + parseInt(top));
  }

  return new Rect(r.left + offset.x, r.top + offset.y, r.width, r.height);
}








function getOverflowContentBoundingRect(aElement) {
  let r = getBoundingContentRect(aElement);

  
  let computedStyle = aElement.ownerDocument.defaultView.getComputedStyle(aElement);
  let blockDisplays = ["block", "inline-block", "list-item"];
  if ((blockDisplays.indexOf(computedStyle.getPropertyValue("display")) != -1 &&
       computedStyle.getPropertyValue("overflow") == "hidden") ||
      aElement instanceof HTMLSelectElement) {
    return r;
  }

  for (let i = 0; i < aElement.childElementCount; i++) {
    r = r.union(getBoundingContentRect(aElement.children[i]));
  }

  return r;
}






let Content = {
  _debugEvents: false,

  get formAssistant() {
    delete this.formAssistant;
    return this.formAssistant = new FormAssistant();
  },

  init: function init() {
    this._isZoomedToElement = false;

    
    addMessageListener("Browser:Blur", this);
    addMessageListener("Browser:SaveAs", this);
    addMessageListener("Browser:MozApplicationCache:Fetch", this);
    addMessageListener("Browser:SetCharset", this);
    addMessageListener("Browser:CanUnload", this);
    addMessageListener("Browser:PanBegin", this);

    addEventListener("touchstart", this, false);
    addEventListener("click", this, true);
    addEventListener("keydown", this);
    addEventListener("keyup", this);

    
    addEventListener("MozApplicationManifest", this, false);
    addEventListener("DOMContentLoaded", this, false);
    addEventListener("pagehide", this, false);
    
    
    
    
    addEventListener("click", this, false);

    docShell.QueryInterface(Ci.nsIDocShellHistory).useGlobalHistory = true;
  },

  



  handleEvent: function handleEvent(aEvent) {
    if (this._debugEvents) Util.dumpLn("Content:", aEvent.type);
    switch (aEvent.type) {
      case "MozApplicationManifest": {
        let doc = aEvent.originalTarget;
        sendAsyncMessage("Browser:MozApplicationManifest", {
          location: doc.documentURIObject.spec,
          manifest: doc.documentElement.getAttribute("manifest"),
          charset: doc.characterSet
        });
        break;
      }

      case "keydown":
        if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE)
          this.formAssistant.close();
        break;

      case "keyup":
        
        
        if (!aEvent.target.value)
          this.formAssistant.close();
        else
          this.formAssistant.open(aEvent.target);
        break;

      case "click":
        if (aEvent.eventPhase == aEvent.BUBBLING_PHASE)
          this._onClick(aEvent);
        else
          this._genericMouseClick(aEvent);
        break;
      
      case "DOMContentLoaded":
        this._maybeNotifyErroPage();
        break;

      case "pagehide":
        if (aEvent.target == content.document)
          this._resetFontSize();          
        break;

      case "touchstart":
        let touch = aEvent.changedTouches[0];
        this._genericMouseDown(touch.clientX, touch.clientY);
        break;
    }
  },

  receiveMessage: function receiveMessage(aMessage) {
    if (this._debugEvents) Util.dumpLn("Content:", aMessage.name);
    let json = aMessage.json;
    let x = json.x;
    let y = json.y;
    let modifiers = json.modifiers;

    switch (aMessage.name) {
      case "Browser:Blur":
        gFocusManager.clearFocus(content);
        break;

      case "Browser:CanUnload":
        let canUnload = docShell.contentViewer.permitUnload();
        sendSyncMessage("Browser:CanUnload:Return", { permit: canUnload });
        break;

      case "Browser:SaveAs":
        break;

      case "Browser:MozApplicationCache:Fetch": {
        let currentURI = Services.io.newURI(json.location, json.charset, null);
        let manifestURI = Services.io.newURI(json.manifest, json.charset, currentURI);
        let updateService = Cc["@mozilla.org/offlinecacheupdate-service;1"]
                            .getService(Ci.nsIOfflineCacheUpdateService);
        updateService.scheduleUpdate(manifestURI, currentURI, content);
        break;
      }

      case "Browser:SetCharset": {
        docShell.charset = json.charset;

        let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
        webNav.reload(Ci.nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
        break;
      }

      case "Browser:PanBegin":
        this._cancelTapHighlight();
        break;
    }
  },

  






  _genericMouseDown: function _genericMouseDown(x, y) {
    let { element } = elementFromPoint(x, y);
    if (!element)
      return;

    
    let isDisabled = element instanceof HTMLOptionElement ?
      (element.disabled || element.parentNode.disabled) : element.disabled;
    if (isDisabled)
      return;

    
    this._doTapHighlight(element);
  },

  _genericMouseClick: function _genericMouseClick(aEvent) {
    ContextMenuHandler.reset();

    let { element: element } = elementFromPoint(aEvent.clientX, aEvent.clientY);
    if (!element)
      return;

    
    if (!this.lastClickElement || this.lastClickElement != element) {
      this.lastClickElement = element;
      if (aEvent.mozInputSource == Ci.nsIDOMMouseEvent.MOZ_SOURCE_MOUSE &&
          !(element instanceof HTMLSelectElement)) {
        return;
      }
    }

    this.formAssistant.focusSync = true;

    
    
    
    
    
    if (!this.formAssistant.open(element, aEvent)) {
      if (gFocusManager.focusedElement &&
          gFocusManager.focusedElement != element) {
        gFocusManager.focusedElement.blur();
      }
      
      gFocusManager.setFocus(element, Ci.nsIFocusManager.FLAG_NOSCROLL);
      sendAsyncMessage("FindAssist:Hide", { });
    }

    this._cancelTapHighlight();
    this.formAssistant.focusSync = false;
  },

  



  
  _onClick: function _onClick(aEvent) {
    
    if (!aEvent.isTrusted)
      return;

    let ot = aEvent.originalTarget;
    let errorDoc = ot.ownerDocument;
    if (!errorDoc)
      return;

    
    
    if (/^about:certerror\?e=nssBadCert/.test(errorDoc.documentURI)) {
      let perm = errorDoc.getElementById("permanentExceptionButton");
      let temp = errorDoc.getElementById("temporaryExceptionButton");
      if (ot == temp || ot == perm) {
        let action = (ot == perm ? "permanent" : "temporary");
        sendAsyncMessage("Browser:CertException",
                         { url: errorDoc.location.href, action: action });
      } else if (ot == errorDoc.getElementById("getMeOutOfHereButton")) {
        sendAsyncMessage("Browser:CertException",
                         { url: errorDoc.location.href, action: "leave" });
      }
    } else if (/^about:blocked/.test(errorDoc.documentURI)) {
      
      
      
      let isMalware = /e=malwareBlocked/.test(errorDoc.documentURI);
    
      if (ot == errorDoc.getElementById("getMeOutButton")) {
        sendAsyncMessage("Browser:BlockedSite",
                         { url: errorDoc.location.href, action: "leave" });
      } else if (ot == errorDoc.getElementById("reportButton")) {
        
        
        
        let action = isMalware ? "report-malware" : "report-phishing";
        sendAsyncMessage("Browser:BlockedSite",
                         { url: errorDoc.location.href, action: action });
      } else if (ot == errorDoc.getElementById("ignoreWarningButton")) {
        
        
        
        let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
        webNav.loadURI(content.location,
                       Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CLASSIFIER,
                       null, null, null);
      }
    }
  },


  



  _getContentClientRects: function getContentClientRects(aElement) {
    let offset = ContentScroll.getScrollOffset(content);
    offset = new Point(offset.x, offset.y);

    let nativeRects = aElement.getClientRects();
    
    for (let frame = aElement.ownerDocument.defaultView; frame != content;
         frame = frame.parent) {
      
      let rect = frame.frameElement.getBoundingClientRect();
      let left = frame.getComputedStyle(frame.frameElement, "").borderLeftWidth;
      let top = frame.getComputedStyle(frame.frameElement, "").borderTopWidth;
      offset.add(rect.left + parseInt(left), rect.top + parseInt(top));
    }

    let result = [];
    for (let i = nativeRects.length - 1; i >= 0; i--) {
      let r = nativeRects[i];
      result.push({ left: r.left + offset.x,
                    top: r.top + offset.y,
                    width: r.width,
                    height: r.height
                  });
    }
    return result;
  },

  _maybeNotifyErroPage: function _maybeNotifyErroPage() {
    
    
    
    if (content.location.href !== content.document.documentURI)
      sendAsyncMessage("Browser:ErrorPage", null);
  },

  _resetFontSize: function _resetFontSize() {
    this._isZoomedToElement = false;
    this._setMinFontSize(0);
  },

  _highlightElement: null,

  _doTapHighlight: function _doTapHighlight(aElement) {
    gDOMUtils.setContentState(aElement, kStateActive);
    this._highlightElement = aElement;
  },

  _cancelTapHighlight: function _cancelTapHighlight(aElement) {
    gDOMUtils.setContentState(content.document.documentElement, kStateActive);
    this._highlightElement = null;
  },

  





  _sendMouseEvent: function _sendMouseEvent(aName, aElement, aX, aY, aButton) {
    
    
    if (!(aElement instanceof HTMLHtmlElement)) {
      let isTouchClick = true;
      let rects = this._getContentClientRects(aElement);
      for (let i = 0; i < rects.length; i++) {
        let rect = rects[i];
        
        
        
        let inBounds = 
          (aX > rect.left + 1 && aX < (rect.left + rect.width - 1)) &&
          (aY > rect.top + 1 && aY < (rect.top + rect.height - 1));
        if (inBounds) {
          isTouchClick = false;
          break;
        }
      }

      if (isTouchClick) {
        let rect = new Rect(rects[0].left, rects[0].top,
                            rects[0].width, rects[0].height);
        if (rect.isEmpty())
          return;

        let point = rect.center();
        aX = point.x;
        aY = point.y;
      }
    }

    let button = aButton || 0;
    let scrollOffset = ContentScroll.getScrollOffset(content);
    let x = aX - scrollOffset.x;
    let y = aY - scrollOffset.y;

    
    
    let windowUtils = Util.getWindowUtils(content);
    windowUtils.sendMouseEventToWindow(aName, x, y, button, 1, 0, true,
                                       1.0, Ci.nsIDOMMouseEvent.MOZ_SOURCE_MOUSE);
  },

  _setMinFontSize: function _setMinFontSize(aSize) {
    let viewer = docShell.contentViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);
    if (viewer)
      viewer.minFontSize = aSize;
  }
};

Content.init();

var FormSubmitObserver = {
  init: function init(){
    addMessageListener("Browser:TabOpen", this);
    addMessageListener("Browser:TabClose", this);

    addEventListener("pageshow", this, false);

    Services.obs.addObserver(this, "invalidformsubmit", false);
  },

  handleEvent: function handleEvent(aEvent) {
    let target = aEvent.originalTarget;
    let isRootDocument = (target == content.document || target.ownerDocument == content.document);
    if (!isRootDocument)
      return;

    
    if (aEvent.type == "pageshow")
      Content.formAssistant.invalidSubmit = false;
  },

  receiveMessage: function receiveMessage(aMessage) {
    let json = aMessage.json;
    switch (aMessage.name) {
      case "Browser:TabOpen":
        Services.obs.addObserver(this, "formsubmit", false);
        break;
      case "Browser:TabClose":
        Services.obs.removeObserver(this, "formsubmit", false);
        break;
    }
  },

  notify: function notify(aFormElement, aWindow, aActionURI, aCancelSubmit) {
    
    if (aWindow == content)
      
      sendAsyncMessage("Browser:FormSubmit", {});
  },

  notifyInvalidSubmit: function notifyInvalidSubmit(aFormElement, aInvalidElements) {
    if (!aInvalidElements.length)
      return;

    let element = aInvalidElements.queryElementAt(0, Ci.nsISupports);
    if (!(element instanceof HTMLInputElement ||
          element instanceof HTMLTextAreaElement ||
          element instanceof HTMLSelectElement ||
          element instanceof HTMLButtonElement)) {
      return;
    }

    Content.formAssistant.invalidSubmit = true;
    Content.formAssistant.open(element);
  },

  QueryInterface : function(aIID) {
    if (!aIID.equals(Ci.nsIFormSubmitObserver) &&
        !aIID.equals(Ci.nsISupportsWeakReference) &&
        !aIID.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

FormSubmitObserver.init();
