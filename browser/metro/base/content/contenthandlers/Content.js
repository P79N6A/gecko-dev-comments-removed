





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

XPCOMUtils.defineLazyModuleGetter(this, "LoginManagerContent",
  "resource://gre/modules/LoginManagerContent.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gFocusManager",
  "@mozilla.org/focus-manager;1", "nsIFocusManager");

XPCOMUtils.defineLazyServiceGetter(this, "gDOMUtils",
  "@mozilla.org/inspector/dom-utils;1", "inIDOMUtils");

this.XULDocument = Ci.nsIDOMXULDocument;
this.HTMLHtmlElement = Ci.nsIDOMHTMLHtmlElement;
this.HTMLIFrameElement = Ci.nsIDOMHTMLIFrameElement;
this.HTMLFrameElement = Ci.nsIDOMHTMLFrameElement;
this.HTMLFrameSetElement = Ci.nsIDOMHTMLFrameSetElement;
this.HTMLSelectElement = Ci.nsIDOMHTMLSelectElement;
this.HTMLOptionElement = Ci.nsIDOMHTMLOptionElement;

const kReferenceDpi = 240; 

const kStateActive = 0x00000001; 

const kZoomToElementMargin = 16; 







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
this.getBoundingContentRect = getBoundingContentRect;








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
this.getOverflowContentBoundingRect = getOverflowContentBoundingRect;






let Content = {
  _debugEvents: false,

  get formAssistant() {
    delete this.formAssistant;
    return this.formAssistant = new FormAssistant();
  },

  init: function init() {
    
    addMessageListener("Browser:Blur", this);
    addMessageListener("Browser:SaveAs", this);
    addMessageListener("Browser:MozApplicationCache:Fetch", this);
    addMessageListener("Browser:SetCharset", this);
    addMessageListener("Browser:CanUnload", this);
    addMessageListener("Browser:PanBegin", this);
    addMessageListener("Gesture:SingleTap", this);
    addMessageListener("Gesture:DoubleTap", this);

    addEventListener("touchstart", this, false);
    addEventListener("click", this, true);
    addEventListener("keydown", this);
    addEventListener("keyup", this);

    
    addEventListener("MozApplicationManifest", this, false);
    addEventListener("DOMContentLoaded", this, false);
    addEventListener("DOMAutoComplete", this, false);
    addEventListener("DOMFormHasPassword", this, false);
    addEventListener("blur", this, false);
    
    
    
    
    addEventListener("click", this, false);

    docShell.useGlobalHistory = true;
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

      case "keyup":
        
        
        
        if ((!aEvent.target.value && aEvent.keyCode != aEvent.DOM_VK_DOWN)
          || aEvent.keyCode == aEvent.DOM_VK_ESCAPE)
          this.formAssistant.close();
        else
          this.formAssistant.open(aEvent.target, aEvent);
        break;

      case "click":
        
        
        
        
        SelectionHandler.onClickCoords(aEvent.clientX, aEvent.clientY);

        if (aEvent.eventPhase == aEvent.BUBBLING_PHASE)
          this._onClickBubble(aEvent);
        else
          this._onClickCapture(aEvent);
        break;

      case "DOMFormHasPassword":
        LoginManagerContent.onFormPassword(aEvent);
        break;

      case "DOMContentLoaded":
        this._maybeNotifyErrorPage();
        break;

      case "DOMAutoComplete":
      case "blur":
        LoginManagerContent.onUsernameInput(aEvent);
        break;

      case "touchstart":
        this._onTouchStart(aEvent);
        break;
    }
  },

  receiveMessage: function receiveMessage(aMessage) {
    if (this._debugEvents) Util.dumpLn("Content:", aMessage.name);
    let json = aMessage.json;
    let x = json.x;
    let y = json.y;

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
        docShell.gatherCharsetMenuTelemetry();
        docShell.charset = json.charset;

        let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
        webNav.reload(Ci.nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
        break;
      }

      case "Browser:PanBegin":
        this._cancelTapHighlight();
        break;

      case "Gesture:SingleTap":
        this._onSingleTap(json.x, json.y, json.modifiers);
        break;

      case "Gesture:DoubleTap":
        this._onDoubleTap(json.x, json.y);
        break;
    }
  },

  



  _onTouchStart: function _onTouchStart(aEvent) {
    let element = aEvent.target;

    
    let isDisabled = element instanceof HTMLOptionElement ?
      (element.disabled || element.parentNode.disabled) : element.disabled;
    if (isDisabled)
      return;

    
    this._doTapHighlight(element);
  },

  _onClickCapture: function _onClickCapture(aEvent) {
    let element = aEvent.target;

    ContextMenuHandler.reset();

    
    if (!this.lastClickElement || this.lastClickElement != element) {
      this.lastClickElement = element;
      if (aEvent.mozInputSource == Ci.nsIDOMMouseEvent.MOZ_SOURCE_MOUSE &&
          !(element instanceof HTMLSelectElement)) {
        return;
      }
    }

    this.formAssistant.focusSync = true;
    this.formAssistant.open(element, aEvent);
    this._cancelTapHighlight();
    this.formAssistant.focusSync = false;

    
    if (Util.isEditable(element) &&
        aEvent.mozInputSource == Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH) {
      let { offsetX, offsetY } = Util.translateToTopLevelWindow(element);
      sendAsyncMessage("Content:SelectionCaret", {
        xPos: aEvent.clientX + offsetX,
        yPos: aEvent.clientY + offsetY
      });
    } else {
      SelectionHandler.closeSelection();
    }
  },

  
  _onClickBubble: function _onClickBubble(aEvent) {
    
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
      
    
      if (ot == errorDoc.getElementById("getMeOutButton")) {
        sendAsyncMessage("Browser:BlockedSite",
                         { url: errorDoc.location.href, action: "leave" });
      } else if (ot == errorDoc.getElementById("reportButton")) {
        
        
        sendAsyncMessage("Browser:BlockedSite",
                         { url: errorDoc.location.href, action: "report-phishing" });
      } else if (ot == errorDoc.getElementById("ignoreWarningButton")) {
        
        
        
        let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
        webNav.loadURI(content.location,
                       Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CLASSIFIER,
                       null, null, null);
      }
    }
  },

  _onSingleTap: function (aX, aY, aModifiers) {
    let utils = Util.getWindowUtils(content);
    for (let type of ["mousemove", "mousedown", "mouseup"]) {
      utils.sendMouseEventToWindow(type, aX, aY, 0, 1, aModifiers, true, 1.0,
          Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH);
    }
  },

  _onDoubleTap: function (aX, aY) {
    let { element } = Content.getCurrentWindowAndOffset(aX, aY);
    while (element && !this._shouldZoomToElement(element)) {
      element = element.parentNode;
    }

    if (!element) {
      this._zoomOut();
    } else {
      this._zoomToElement(element);
    }
  },

  


  _zoomOut: function() {
    let rect = new Rect(0,0,0,0);
    this._zoomToRect(rect);
  },

  _zoomToElement: function(aElement) {
    let rect = getBoundingContentRect(aElement);
    this._inflateRect(rect, kZoomToElementMargin);
    this._zoomToRect(rect);
  },

  _inflateRect: function(aRect, aMargin) {
    aRect.left -= aMargin;
    aRect.top -= aMargin;
    aRect.bottom += aMargin;
    aRect.right += aMargin;
  },

  _zoomToRect: function (aRect) {
    let utils = Util.getWindowUtils(content);
    let viewId = utils.getViewId(content.document.documentElement);
    let presShellId = {};
    utils.getPresShellId(presShellId);
    sendAsyncMessage("Content:ZoomToRect", {
      rect: aRect,
      presShellId: presShellId.value,
      viewId: viewId,
    });
  },

  _shouldZoomToElement: function(aElement) {
    let win = aElement.ownerDocument.defaultView;
    if (win.getComputedStyle(aElement, null).display == "inline") {
      return false;
    }
    else if (aElement instanceof Ci.nsIDOMHTMLLIElement) {
      return false;
    }
    else if (aElement instanceof Ci.nsIDOMHTMLQuoteElement) {
      return false;
    }
    else {
      return true;
    }
  },


  



  





  getCurrentWindowAndOffset: function(x, y) {
    
    
    
    let utils = Util.getWindowUtils(content);
    let element = utils.elementFromPoint(x, y, true, false);
    let offset = { x:0, y:0 };

    while (element && (element instanceof HTMLIFrameElement ||
                       element instanceof HTMLFrameElement)) {
      
      let rect = element.getBoundingClientRect();

      
      

      
      scrollOffset = ContentScroll.getScrollOffset(element.contentDocument.defaultView);
      
      x -= rect.left + scrollOffset.x;
      y -= rect.top + scrollOffset.y;

      

      
      offset.x += rect.left;
      offset.y += rect.top;

      
      utils = element.contentDocument
                     .defaultView
                     .QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils);

      
      element = utils.elementFromPoint(x, y, true, false);
    }

    if (!element)
      return {};

    return {
      element: element,
      contentWindow: element.ownerDocument.defaultView,
      offset: offset,
      utils: utils
    };
  },


  _maybeNotifyErrorPage: function _maybeNotifyErrorPage() {
    
    
    
    if (content.location.href !== content.document.documentURI)
      sendAsyncMessage("Browser:ErrorPage", null);
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
        Services.obs.removeObserver(this, "formsubmit");
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
this.Content = Content;

FormSubmitObserver.init();
