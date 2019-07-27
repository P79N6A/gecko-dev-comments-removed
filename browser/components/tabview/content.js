



"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;

Cu.import("resource:///modules/tabview/utils.jsm");



this.__defineGetter__("webProgress", function () {
  let ifaceReq = docShell.QueryInterface(Ci.nsIInterfaceRequestor);
  return ifaceReq.getInterface(Ci.nsIWebProgress);
});





let WindowEventHandler = {
  
  
  
  
  onDOMWillOpenModalDialog: function WEH_onDOMWillOpenModalDialog(event) {
    
    
    if (!event.isTrusted)
      return;

    
    
    
    sendSyncMessage("Panorama:DOMWillOpenModalDialog");
  },

  
  
  
  
  onMozAfterPaint: function WEH_onMozAfterPaint(event) {
    if (event.clientRects.length > 0) {
      sendAsyncMessage("Panorama:MozAfterPaint");
    }
  }
};


addEventListener("DOMWillOpenModalDialog", WindowEventHandler.onDOMWillOpenModalDialog, false);
addEventListener("MozAfterPaint", WindowEventHandler.onMozAfterPaint, false);





let WindowMessageHandler = {
  
  
  
  isDocumentLoaded: function WMH_isDocumentLoaded(cx) {
    let isLoaded = (content.document.readyState != "uninitialized" &&
                    !webProgress.isLoadingDocument);

    sendAsyncMessage(cx.name, {isLoaded: isLoaded});
  },

  
  
  
  isImageDocument: function WMH_isImageDocument(cx) {
    let isImageDocument = (content.document instanceof Ci.nsIImageDocument);

    sendAsyncMessage(cx.name, {isImageDocument: isImageDocument});
  },

  waitForDocumentLoad: function WMH_waitForDocumentLoad() {
    addEventListener("load", function listener() {
      removeEventListener("load", listener, true);
      sendAsyncMessage("Panorama:documentLoaded");
    }, true);
  },
};


addMessageListener("Panorama:isDocumentLoaded", WindowMessageHandler.isDocumentLoaded);
addMessageListener("Panorama:isImageDocument", WindowMessageHandler.isImageDocument);
addMessageListener("Panorama:waitForDocumentLoad", WindowMessageHandler.waitForDocumentLoad);
