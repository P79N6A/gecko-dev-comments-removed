



































"use strict";

let Cu = Components.utils;

Cu.import("resource:///modules/tabview/utils.jsm");



__defineGetter__("webProgress", function () {
  let ifaceReq = docShell.QueryInterface(Ci.nsIInterfaceRequestor);
  return ifaceReq.getInterface(Ci.nsIWebProgress);
});





let WindowEventHandler = {
  
  
  
  
  onDOMContentLoaded: function WEH_onDOMContentLoaded(event) {
    sendAsyncMessage("Panorama:DOMContentLoaded");
  },

  
  
  
  
  onDOMWillOpenModalDialog: function WEH_onDOMWillOpenModalDialog(event) {
    
    
    if (!event.isTrusted)
      return;

    
    
    
    sendSyncMessage("Panorama:DOMWillOpenModalDialog");
  }
};


addEventListener("DOMContentLoaded", WindowEventHandler.onDOMContentLoaded, false);
addEventListener("DOMWillOpenModalDialog", WindowEventHandler.onDOMWillOpenModalDialog, false);





let WindowMessageHandler = {
  
  
  
  isDocumentLoaded: function WMH_isDocumentLoaded(cx) {
    let isLoaded = (content.document.readyState == "complete" &&
                    !webProgress.isLoadingDocument);

    sendAsyncMessage(cx.name, {isLoaded: isLoaded});
  }
};


addMessageListener("Panorama:isDocumentLoaded", WindowMessageHandler.isDocumentLoaded);







let WebProgressListener = {
  
  
  
  onStateChange: function WPL_onStateChange(webProgress, request, flag, status) {
    
    
    
    if (flag & Ci.nsIWebProgressListener.STATE_START) {
      
      if (this._isTopWindow(webProgress))
        sendAsyncMessage("Panorama:StoragePolicy:granted");
    }

    
    
    if (flag & Ci.nsIWebProgressListener.STATE_STOP) {
      
      if (this._isTopWindow(webProgress) &&
          request && request instanceof Ci.nsIHttpChannel) {
        request.QueryInterface(Ci.nsIHttpChannel);

        let exclude = false;
        let reason = "";

        
        
        if (this._isNoStoreResponse(request)) {
          exclude = true;
          reason = "no-store";
        }
        
        
        else if (request.URI.schemeIs("https")) {
          let cacheControlHeader = this._getCacheControlHeader(request);
          if (cacheControlHeader && !(/public/i).test(cacheControlHeader)) {
            exclude = true;
            reason = "https";
          }
        }

        if (exclude)
          sendAsyncMessage("Panorama:StoragePolicy:denied", {reason: reason});
      }
    }
  },

  
  
  
  
  _isTopWindow: function WPL__isTopWindow(webProgress) {
    
    return !!Utils.attempt(function () webProgress.DOMWindow == content);
  },

  
  
  
  _isNoStoreResponse: function WPL__isNoStoreResponse(req) {
    
    return !!Utils.attempt(function () req.isNoStoreResponse());
  },

  
  
  
  _getCacheControlHeader: function WPL__getCacheControlHeader(req) {
    
    return Utils.attempt(function () req.getResponseHeader("Cache-Control"));
  },

  
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports])
};


webProgress.addProgressListener(WebProgressListener, Ci.nsIWebProgress.NOTIFY_STATE_WINDOW);
