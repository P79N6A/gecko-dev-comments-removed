

dump("###################################### content loaded\n");

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
let HTMLSelectElement = Ci.nsIDOMHTMLSelectElement;
let HTMLOptionElement = Ci.nsIDOMHTMLOptionElement;


const kViewportMinScale  = 0;
const kViewportMaxScale  = 10;
const kViewportMinWidth  = 200;
const kViewportMaxWidth  = 10000;
const kViewportMinHeight = 223;
const kViewportMaxHeight = 10000;

const kReferenceDpi = 240; 

const kStateActive = 0x00000001; 


const ElementTouchHelper = {
  get radius() {
    let prefs = Services.prefs;
    delete this.radius;
    return this.radius = { "top": prefs.getIntPref("browser.ui.touch.top"),
                           "right": prefs.getIntPref("browser.ui.touch.right"),
                           "bottom": prefs.getIntPref("browser.ui.touch.bottom"),
                           "left": prefs.getIntPref("browser.ui.touch.left")
                         };
  },

  get weight() {
    delete this.weight;
    return this.weight = { "visited": Services.prefs.getIntPref("browser.ui.touch.weight.visited")
                         };
  },

  
  getClosest: function getClosest(aWindowUtils, aX, aY) {
    let dpiRatio = aWindowUtils.displayDPI / kReferenceDpi;

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






function elementFromPoint(x, y) {
  
  
  let cwu = Util.getWindowUtils(content);
  let scroll = ContentScroll.getScrollOffset(content);
  x = x - scroll.x;
  y = y - scroll.y;
  let elem = ElementTouchHelper.getClosest(cwu, x, y);

  
  while (elem && (elem instanceof HTMLIFrameElement || elem instanceof HTMLFrameElement)) {
    
    let rect = elem.getBoundingClientRect();
    x -= rect.left;
    y -= rect.top;
    let windowUtils = elem.contentDocument.defaultView.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    elem = ElementTouchHelper.getClosest(windowUtils, x, y);
  }

  return elem;
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

  
  for (let frame = aElement.ownerDocument.defaultView; frame != content; frame = frame.parent) {
    
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
  if ((blockDisplays.indexOf(computedStyle.getPropertyValue("display")) != -1 && computedStyle.getPropertyValue("overflow") == "hidden") || aElement instanceof HTMLSelectElement)
    return r;

  for (let i = 0; i < aElement.childElementCount; i++)
    r = r.union(getBoundingContentRect(aElement.children[i]));

  return r;
}

function getContentClientRects(aElement) {
  let offset = ContentScroll.getScrollOffset(content);
  offset = new Point(offset.x, offset.y);

  let nativeRects = aElement.getClientRects();
  
  for (let frame = aElement.ownerDocument.defaultView; frame != content; frame = frame.parent) {
    
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
};


let Content = {
  get _formAssistant() {
    delete this._formAssistant;
    return this._formAssistant = new FormAssistant();
  },

  init: function init() {
    this._isZoomedToElement = false;

    addMessageListener("Browser:Blur", this);
    addMessageListener("Browser:MouseOver", this);
    addMessageListener("Browser:MouseLong", this);
    addMessageListener("Browser:MouseDown", this);
    addMessageListener("Browser:MouseUp", this);
    addMessageListener("Browser:MouseCancel", this);
    addMessageListener("Browser:SaveAs", this);
    addMessageListener("Browser:ZoomToPoint", this);
    addMessageListener("Browser:MozApplicationCache:Fetch", this);
    addMessageListener("Browser:SetCharset", this);
    addMessageListener("Browser:ContextCommand", this);
    addMessageListener("Browser:CanUnload", this);

    if (Util.isParentProcess())
      addEventListener("DOMActivate", this, true);

    addEventListener("MozApplicationManifest", this, false);
    addEventListener("command", this, false);
    addEventListener("pagehide", this, false);
    addEventListener("keypress", this, false, false);

    docShell.QueryInterface(Ci.nsIDocShellHistory).useGlobalHistory = true;
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      
      
      case "keypress":
        let timer = new Util.Timeout(function() {
          if(aEvent.getPreventDefault())
            return;

          let eventData = {
            ctrlKey: aEvent.ctrlKey,
            altKey: aEvent.altKey,
            shiftKey: aEvent.shiftKey,
            metaKey: aEvent.metaKey,
            keyCode: aEvent.keyCode,
            charCode: aEvent.charCode
          };
          sendAsyncMessage("Browser:KeyPress", eventData);
        });
        timer.once(0);
        break;

      case "DOMActivate": {
        
        let target = aEvent.originalTarget;
        let href = Util.getHrefForElement(target);
        if (/^http(s?):/.test(href)) {
          aEvent.preventDefault();
          sendAsyncMessage("Browser:OpenURI", { uri: href,
                                                referrer: target.ownerDocument.documentURIObject.spec,
                                                bringFront: true });
        }
        break;
      }

      case "MozApplicationManifest": {
        let doc = aEvent.originalTarget;
        sendAsyncMessage("Browser:MozApplicationManifest", {
          location: doc.documentURIObject.spec,
          manifest: doc.documentElement.getAttribute("manifest"),
          charset: doc.characterSet
        });
        break;
      }

      case "command": {
        
        if (!aEvent.isTrusted)
          return;

        let ot = aEvent.originalTarget;
        let errorDoc = ot.ownerDocument;

        
        
        if (/^about:certerror\?e=nssBadCert/.test(errorDoc.documentURI)) {
          let perm = errorDoc.getElementById("permanentExceptionButton");
          let temp = errorDoc.getElementById("temporaryExceptionButton");
          if (ot == temp || ot == perm) {
            let action = (ot == perm ? "permanent" : "temporary");
            sendAsyncMessage("Browser:CertException", { url: errorDoc.location.href, action: action });
          }
          else if (ot == errorDoc.getElementById("getMeOutOfHereButton")) {
            sendAsyncMessage("Browser:CertException", { url: errorDoc.location.href, action: "leave" });
          }
        }
        else if (/^about:neterror\?e=netOffline/.test(errorDoc.documentURI)) {
          if (ot == errorDoc.getElementById("errorTryAgain")) {
            
            Util.forceOnline();
          }
        }
        break;
      }

      case "pagehide":
        if (aEvent.target == content.document) {
          this._isZoomedToElement = false;
          this._setTextZoom(1);
        }
        break;
    }
  },

  receiveMessage: function receiveMessage(aMessage) {
    let json = aMessage.json;
    let x = json.x;
    let y = json.y;
    let modifiers = json.modifiers;

    switch (aMessage.name) {
      case "Browser:ContextCommand": {
        let wrappedTarget = elementFromPoint(x, y);
        if (!wrappedTarget)
          break;
        let target = wrappedTarget.QueryInterface(Ci.nsIDOMNSEditableElement);
        if (!target)
          break;
        switch (json.command) {
          case "select-all":
            target.editor.selectAll();
            break;
          case "paste":
            target.editor.paste(Ci.nsIClipboard.kGlobalClipboard);
            break;
        }
        target.focus();
        break;
      }

      case "Browser:Blur":
        gFocusManager.clearFocus(content);
        break;

      case "Browser:CanUnload":
        let canUnload = docShell.contentViewer.permitUnload();
        sendSyncMessage("Browser:CanUnload:Return", { permit: canUnload });
        break;

      case "Browser:MouseOver": {
        let element = elementFromPoint(x, y);
        if (!element)
          return;

        
        this._sendMouseEvent("mousemove", element, x, y);
        break;
      }

      case "Browser:MouseDown": {
        let element = elementFromPoint(x, y);
        if (!element)
          return;

        
        let isDisabled = element instanceof HTMLOptionElement ? (element.disabled || element.parentNode.disabled) : element.disabled;
        if (isDisabled)
          return;

        
        this._doTapHighlight(element);
        break;
      }

      case "Browser:MouseCancel": {
        this._cancelTapHighlight();
        break;
      }

      case "Browser:MouseLong": {
        let element = elementFromPoint(x, y);
        if (!element)
          return;

#ifdef MOZ_PLATFORM_MAEMO
        if (element instanceof Ci.nsIDOMHTMLEmbedElement) {
          
          
          this._sendMouseEvent("mousedown", element, x, y, 2);
          this._sendMouseEvent("mouseup", element, x, y, 2);
          break;
        }
#endif

        ContextHandler.messageId = json.messageId;

        let event = content.document.createEvent("PopupEvents");
        event.initEvent("contextmenu", true, true);
        event.x = x;
        event.y = y;
        element.dispatchEvent(event);
        break;
      }

      case "Browser:MouseUp": {
        this._formAssistant.focusSync = true;
        let element = elementFromPoint(x, y);
        if (modifiers == Ci.nsIDOMNSEvent.CONTROL_MASK) {
          let uri = Util.getHrefForElement(element);
          if (uri)
            sendAsyncMessage("Browser:OpenURI", { uri: uri,
                                                  referrer: element.ownerDocument.documentURIObject.spec });
        } else if (!this._formAssistant.open(element) && this._highlightElement) {
          sendAsyncMessage("FindAssist:Hide", { });
          this._sendMouseEvent("mousemove", this._highlightElement, x, y);
          this._sendMouseEvent("mousedown", this._highlightElement, x, y);
          this._sendMouseEvent("mouseup", this._highlightElement, x, y);
        }
        this._cancelTapHighlight();
        ContextHandler.reset();
        this._formAssistant.focusSync = false;
        break;
      }

      case "Browser:SaveAs":
        if (json.type != Ci.nsIPrintSettings.kOutputFormatPDF)
          return;

        let printSettings = Cc["@mozilla.org/gfx/printsettings-service;1"]
                              .getService(Ci.nsIPrintSettingsService)
                              .newPrintSettings;
        printSettings.printSilent = true;
        printSettings.showPrintProgress = false;
        printSettings.printBGImages = true;
        printSettings.printBGColors = true;
        printSettings.printToFile = true;
        printSettings.toFileName = json.filePath;
        printSettings.printFrameType = Ci.nsIPrintSettings.kFramesAsIs;
        printSettings.outputFormat = Ci.nsIPrintSettings.kOutputFormatPDF;

        
        printSettings.footerStrCenter = "";
        printSettings.footerStrLeft   = "";
        printSettings.footerStrRight  = "";
        printSettings.headerStrCenter = "";
        printSettings.headerStrLeft   = "";
        printSettings.headerStrRight  = "";

        let listener = {
          onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
            if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
              sendAsyncMessage("Browser:SaveAs:Return", { type: json.type, id: json.id, referrer: json.referrer });
            }
          },
          onProgressChange : function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress) {},

          
          onLocationChange : function() { throw "Unexpected onLocationChange"; },
          onStatusChange   : function() { throw "Unexpected onStatusChange";   },
          onSecurityChange : function() { throw "Unexpected onSecurityChange"; }
        };

        let webBrowserPrint = content.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebBrowserPrint);
        webBrowserPrint.print(printSettings, listener);
        break;

      case "Browser:ZoomToPoint": {
        let rect = null;
        if (this._isZoomedToElement) {
          this._isZoomedToElement = false;
          this._setTextZoom(1);
        } else {
          this._isZoomedToElement = true;
          let element = elementFromPoint(x, y);
          let win = element.ownerDocument.defaultView;
          while (element && win.getComputedStyle(element,null).display == "inline")
            element = element.parentNode;
          if (element) {
            rect = getBoundingContentRect(element);
            if (Services.prefs.getBoolPref("browser.ui.zoom.reflow")) {
              sendAsyncMessage("Browser:ZoomToPoint:Return", { x: x, y: y, zoomTo: rect });
              this._setTextZoom(Math.max(1, rect.width / json.width));

              let oldRect = rect;
              rect = getBoundingContentRect(element);
              y += rect.top - oldRect.top;
            }
          }
        }
        content.setTimeout(function() {
          sendAsyncMessage("Browser:ZoomToPoint:Return", { x: x, y: y, zoomTo: rect });
        }, 0);
        break;
      }

      case "Browser:MozApplicationCache:Fetch": {
        let currentURI = Services.io.newURI(json.location, json.charset, null);
        let manifestURI = Services.io.newURI(json.manifest, json.charset, currentURI);
        let updateService = Cc["@mozilla.org/offlinecacheupdate-service;1"]
                            .getService(Ci.nsIOfflineCacheUpdateService);
        updateService.scheduleUpdate(manifestURI, currentURI, content);
        break;
      }

      case "Browser:SetCharset": {
        let docCharset = docShell.QueryInterface(Ci.nsIDocCharset);
        docCharset.charset = json.charset;

        let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
        webNav.reload(Ci.nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
        break;
      }
    }
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
      let rects = getContentClientRects(aElement);
      for (let i = 0; i < rects.length; i++) {
        let rect = rects[i];
        if ((aX > rect.left && aX < (rect.left + rect.width)) &&
            (aY > rect.top && aY < (rect.top + rect.height))) {
          isTouchClick = false;
          break;
        }
      }

      if (isTouchClick) {
        let rect = rects[0];
        let point = (new Rect(rect.left, rect.top, rect.width, rect.height)).center();
        aX = point.x;
        aY = point.y;
      }
    }

    let scrollOffset = ContentScroll.getScrollOffset(content);
    let windowUtils = Util.getWindowUtils(content);
    aButton = aButton || 0;
    windowUtils.sendMouseEventToWindow(aName, aX - scrollOffset.x, aY - scrollOffset.y, aButton, 1, 0, true);
  },

  _setTextZoom: function _setTextZoom(aZoom) {
    let viewer = docShell.contentViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);
    viewer.textZoom = aZoom;
  }
};

Content.init();

let ViewportHandler = {
  init: function init() {
    addEventListener("DOMWindowCreated", this, false);
    addEventListener("DOMMetaAdded", this, false);
    addEventListener("DOMContentLoaded", this, false);
    addEventListener("pageshow", this, false);
  },

  handleEvent: function handleEvent(aEvent) {
    let target = aEvent.originalTarget;
    let isRootDocument = (target == content.document || target.ownerDocument == content.document);
    if (!isRootDocument)
      return;

    switch (aEvent.type) {
      case "DOMWindowCreated":
        this.resetMetadata();
        break;

      case "DOMMetaAdded":
        if (target.name == "viewport")
          this.updateMetadata();
        break;

      case "DOMContentLoaded":
      case "pageshow":
        this.updateMetadata();
        break;
    }
  },

  resetMetadata: function resetMetadata() {
    sendAsyncMessage("Browser:ViewportMetadata", null);
  },

  updateMetadata: function updateMetadata() {
    sendAsyncMessage("Browser:ViewportMetadata", this.getViewportMetadata());
  },

  










  getViewportMetadata: function getViewportMetadata() {
    let doctype = content.document.doctype;
    if (doctype && /(WAP|WML|Mobile)/.test(doctype.publicId))
      return { defaultZoom: 1, autoSize: true, allowZoom: true, autoScale: true };

    let windowUtils = Util.getWindowUtils(content);
    let handheldFriendly = windowUtils.getDocumentMetadata("HandheldFriendly");
    if (handheldFriendly == "true")
      return { defaultZoom: 1, autoSize: true, allowZoom: true, autoScale: true };

    if (content.document instanceof XULDocument)
      return { defaultZoom: 1, autoSize: true, allowZoom: false, autoScale: false };

    
    
    if (Util.isParentProcess())
      return { defaultZoom: 1, autoSize: true, allowZoom: false, autoScale: false };

    
    
    

    
    
    let scale = parseFloat(windowUtils.getDocumentMetadata("viewport-initial-scale"));
    let minScale = parseFloat(windowUtils.getDocumentMetadata("viewport-minimum-scale"));
    let maxScale = parseFloat(windowUtils.getDocumentMetadata("viewport-maximum-scale"));

    let widthStr = windowUtils.getDocumentMetadata("viewport-width");
    let heightStr = windowUtils.getDocumentMetadata("viewport-height");
    let width = Util.clamp(parseInt(widthStr), kViewportMinWidth, kViewportMaxWidth);
    let height = Util.clamp(parseInt(heightStr), kViewportMinHeight, kViewportMaxHeight);

    let allowZoomStr = windowUtils.getDocumentMetadata("viewport-user-scalable");
    let allowZoom = !/^(0|no|false)$/.test(allowZoomStr); 

    scale = Util.clamp(scale, kViewportMinScale, kViewportMaxScale);
    minScale = Util.clamp(minScale, kViewportMinScale, kViewportMaxScale);
    maxScale = Util.clamp(maxScale, kViewportMinScale, kViewportMaxScale);

    
    let autoSize = (widthStr == "device-width" ||
                    (!widthStr && (heightStr == "device-height" || scale == 1.0)));

    return {
      defaultZoom: scale,
      minZoom: minScale,
      maxZoom: maxScale,
      width: width,
      height: height,
      autoSize: autoSize,
      allowZoom: allowZoom,
      autoScale: true
    };
  }
};

ViewportHandler.init();


const kXLinkNamespace = "http://www.w3.org/1999/xlink";

var ContextHandler = {
  _types: [],

  _getLinkURL: function ch_getLinkURL(aLink) {
    let href = aLink.href;
    if (href)
      return href;

    href = aLink.getAttributeNS(kXLinkNamespace, "href");
    if (!href || !href.match(/\S/)) {
      
      
      throw "Empty href";
    }

    return Util.makeURLAbsolute(aLink.baseURI, href);
  },

  _getURI: function ch_getURI(aURL) {
    try {
      return Util.makeURI(aURL);
    } catch (ex) { }

    return null;
  },

  _getProtocol: function ch_getProtocol(aURI) {
    if (aURI)
      return aURI.scheme;
    return null;
  },

  init: function ch_init() {
    addEventListener("contextmenu", this, false);
    addEventListener("pagehide", this, false);
    addMessageListener("Browser:ContextCommand", this, false);
    this.popupNode = null;
  },

  reset: function ch_reset() {
    this.popupNode = null;
  },

  handleEvent: function ch_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "contextmenu":
        this.onContextMenu(aEvent);
        break;
      case "pagehide":
        this.reset();
        break;
    }
  },

  onContextMenu: function ch_onContextMenu(aEvent) {
    if (aEvent.getPreventDefault())
      return;

    let state = {
      types: [],
      label: "",
      linkURL: "",
      linkTitle: "",
      linkProtocol: null,
      mediaURL: "",
      x: aEvent.x,
      y: aEvent.y
    };

    let popupNode = this.popupNode = aEvent.originalTarget;

    
    if (popupNode.nodeType == Ci.nsIDOMNode.ELEMENT_NODE) {
      
      if (popupNode instanceof Ci.nsIImageLoadingContent && popupNode.currentURI) {
        state.types.push("image");
        state.label = state.mediaURL = popupNode.currentURI.spec;
      } else if (popupNode instanceof Ci.nsIDOMHTMLMediaElement) {
        state.label = state.mediaURL = (popupNode.currentSrc || popupNode.src);
        state.types.push((popupNode.paused || popupNode.ended) ? "media-paused" : "media-playing");
        if (popupNode instanceof Ci.nsIDOMHTMLVideoElement)
          state.types.push("video");
      }
    }

    let elem = popupNode;
    while (elem) {
      if (elem.nodeType == Ci.nsIDOMNode.ELEMENT_NODE) {
        
        if ((elem instanceof Ci.nsIDOMHTMLAnchorElement && elem.href) ||
            (elem instanceof Ci.nsIDOMHTMLAreaElement && elem.href) ||
            elem instanceof Ci.nsIDOMHTMLLinkElement ||
            elem.getAttributeNS(kXLinkNamespace, "type") == "simple") {

          
          state.types.push("link");
          state.label = state.linkURL = this._getLinkURL(elem);
          state.linkTitle = popupNode.textContent || popupNode.title;
          state.linkProtocol = this._getProtocol(this._getURI(state.linkURL));
          break;
        } else if (elem instanceof Ci.nsIDOMHTMLInputElement && elem.type === "text") {
          let selectionStart = elem.selectionStart;
          let selectionEnd = elem.selectionEnd;

          state.types.push("input-text");
          if (selectionStart != selectionEnd) {
            state.types.push("copy");
            state.string = elem.value.slice(selectionStart, selectionEnd);
          } else if (elem.value) {
            state.types.push("copy-all");
            state.string = elem.value;
          }

          if (selectionStart > 0 || selectionEnd < elem.textLength)
            state.types.push("select-all");

          let clipboard = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
          let flavors = ["text/unicode"];
          let hasData = clipboard.hasDataMatchingFlavors(flavors, flavors.length, Ci.nsIClipboard.kGlobalClipboard);

          if (hasData && !elem.readOnly)
            state.types.push("paste");
          break;
        }
      }

      elem = elem.parentNode;
    }

    for (let i = 0; i < this._types.length; i++)
      if (this._types[i].handler(state, popupNode))
        state.types.push(this._types[i].name);

    state.messageId = this.messageId;

    sendAsyncMessage("Browser:ContextMenu", state);
  },

  receiveMessage: function ch_receiveMessage(aMessage) {
    let node = this.popupNode;
    let command = aMessage.json.command;

    switch (command) {
      case "play":
      case "pause":
        if (node instanceof Ci.nsIDOMHTMLMediaElement)
          node[command]();
        break;

      case "fullscreen":
        if (node instanceof Ci.nsIDOMHTMLVideoElement) {
          node.pause();
          Cu.import("resource:///modules/video.jsm");
          Video.fullScreenSourceElement = node;
          sendAsyncMessage("Browser:FullScreenVideo:Start");
        }
        break;
    }
  },

  







  registerType: function registerType(aName, aHandler) {
    this._types.push({name: aName, handler: aHandler});
  },

  
  unregisterType: function unregisterType(aName) {
    this._types = this._types.filter(function(type) type.name != aName);
  }
};

ContextHandler.init();

ContextHandler.registerType("mailto", function(aState, aElement) {
  return aState.linkProtocol == "mailto";
});

ContextHandler.registerType("callto", function(aState, aElement) {
  let protocol = aState.linkProtocol;
  return protocol == "tel" || protocol == "callto" || protocol == "sip" || protocol == "voipto";
});

ContextHandler.registerType("link-saveable", function(aState, aElement) {
  let protocol = aState.linkProtocol;
  return (protocol && protocol != "mailto" && protocol != "javascript" && protocol != "news" && protocol != "snews");
});

ContextHandler.registerType("link-openable", function(aState, aElement) {
  return Util.isOpenableScheme(aState.linkProtocol);
});

ContextHandler.registerType("link-shareable", function(aState, aElement) {
  return Util.isShareableScheme(aState.linkProtocol);
});

ContextHandler.registerType("input-text", function(aState, aElement) {
    return aElement instanceof Ci.nsIDOMHTMLInputElement;
});

["image", "video"].forEach(function(aType) {
  ContextHandler.registerType(aType+"-shareable", function(aState, aElement) {
    if (aState.types.indexOf(aType) == -1)
      return false;

    let protocol = ContextHandler._getProtocol(ContextHandler._getURI(aState.mediaURL));
    return Util.isShareableScheme(protocol);
  });
});

ContextHandler.registerType("image-loaded", function(aState, aElement) {
  if (aState.types.indexOf("image") != -1) {
    let request = aElement.getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
    if (request && (request.imageStatus & request.STATUS_SIZE_AVAILABLE))
      return true;
  }
  return false;
});

var FormSubmitObserver = {
  init: function init(){
    addMessageListener("Browser:TabOpen", this);
    addMessageListener("Browser:TabClose", this);
  },

  receiveMessage: function findHandlerReceiveMessage(aMessage) {
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

  QueryInterface : function(aIID) {
    if (!aIID.equals(Ci.nsIFormSubmitObserver) &&
        !aIID.equals(Ci.nsISupportsWeakReference) &&
        !aIID.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

FormSubmitObserver.init();

var FindHandler = {
  get _fastFind() {
    delete this._fastFind;
    this._fastFind = Cc["@mozilla.org/typeaheadfind;1"].createInstance(Ci.nsITypeAheadFind);
    this._fastFind.init(docShell);
    return this._fastFind;
  },

  init: function findHandlerInit() {
    addMessageListener("FindAssist:Find", this);
    addMessageListener("FindAssist:Next", this);
    addMessageListener("FindAssist:Previous", this);
  },

  receiveMessage: function findHandlerReceiveMessage(aMessage) {
    let findResult = Ci.nsITypeAheadFind.FIND_NOTFOUND;
    let json = aMessage.json;
    switch (aMessage.name) {
      case "FindAssist:Find":
        findResult = this._fastFind.find(json.searchString, false);
        break;

      case "FindAssist:Previous":
        findResult = this._fastFind.findAgain(true, false);
        break;

      case "FindAssist:Next":
        findResult = this._fastFind.findAgain(false, false);
        break;
    }

    if (findResult == Ci.nsITypeAheadFind.FIND_NOTFOUND) {
      sendAsyncMessage("FindAssist:Show", { rect: null , result: findResult });
      return;
    }

    let selection = this._fastFind.currentWindow.getSelection();
    if (!selection.rangeCount || selection.isCollapsed) {
      
      let nodes = content.document.querySelectorAll("input[type='text'], textarea");
      for (let i = 0; i < nodes.length; i++) {
        let node = nodes[i];
        if (node instanceof Ci.nsIDOMNSEditableElement && node.editor) {
          selection = node.editor.selectionController.getSelection(Ci.nsISelectionController.SELECTION_NORMAL);
          if (selection.rangeCount && !selection.isCollapsed)
            break;
        }
      }
    }

    let scroll = ContentScroll.getScrollOffset(content);
    for (let frame = this._fastFind.currentWindow; frame != content; frame = frame.parent) {
      let rect = frame.frameElement.getBoundingClientRect();
      let left = frame.getComputedStyle(frame.frameElement, "").borderLeftWidth;
      let top = frame.getComputedStyle(frame.frameElement, "").borderTopWidth;
      scroll.add(rect.left + parseInt(left), rect.top + parseInt(top));
    }

    let rangeRect = selection.getRangeAt(0).getBoundingClientRect();
    let rect = new Rect(scroll.x + rangeRect.left, scroll.y + rangeRect.top, rangeRect.width, rangeRect.height);

    
    let timer = new Util.Timeout(function() {
      sendAsyncMessage("FindAssist:Show", { rect: rect.isEmpty() ? null: rect , result: findResult });
    });
    timer.once(0);
  }
};

FindHandler.init();

var ConsoleAPIObserver = {
  init: function init() {
    addMessageListener("Browser:TabOpen", this);
    addMessageListener("Browser:TabClose", this);
  },

  receiveMessage: function receiveMessage(aMessage) {
    let json = aMessage.json;
    switch (aMessage.name) {
      case "Browser:TabOpen":
        Services.obs.addObserver(this, "console-api-log-event", false);
        break;
      case "Browser:TabClose":
        Services.obs.removeObserver(this, "console-api-log-event", false);
        break;
    }
  },

  observe: function observe(aMessage, aTopic, aData) {
    let contentWindowId = content.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
    aMessage = aMessage.wrappedJSObject;
    if (aMessage.ID != contentWindowId)
      return;

    let mappedArguments = Array.map(aMessage.arguments, this.formatResult, this);
    let joinedArguments = Array.join(mappedArguments, " ");

    if (aMessage.level == "error" || aMessage.level == "warn") {
      let flag = (aMessage.level == "error" ? Ci.nsIScriptError.errorFlag : Ci.nsIScriptError.warningFlag);
      let consoleMsg = Cc["@mozilla.org/scripterror;1"].createInstance(Ci.nsIScriptError);
      consoleMsg.init(joinedArguments, null, null, 0, 0, flag, "content javascript");
      Services.console.logMessage(consoleMsg);
    } else {
      Services.console.logStringMessage(joinedArguments);
    }
  },

  getResultType: function getResultType(aResult) {
    let type = aResult === null ? "null" : typeof aResult;
    if (type == "object" && aResult.constructor && aResult.constructor.name)
      type = aResult.constructor.name;
    return type.toLowerCase();
  },

  formatResult: function formatResult(aResult) {
    let output = "";
    let type = this.getResultType(aResult);
    switch (type) {
      case "string":
      case "boolean":
      case "date":
      case "error":
      case "number":
      case "regexp":
        output = aResult.toString();
        break;
      case "null":
      case "undefined":
        output = type;
        break;
      default:
        if (aResult.toSource) {
          try {
            output = aResult.toSource();
          } catch (ex) { }
        }
        if (!output || output == "({})") {
          output = aResult.toString();
        }
        break;
    }

    return output;
  }
};

ConsoleAPIObserver.init();
