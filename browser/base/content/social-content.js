







const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");


docShell.isAppTab = true;



SocialErrorListener = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports]),

  defaultTemplate: "about:socialerror?mode=tryAgainOnly&url=%{url}&origin=%{origin}",
  urlTemplate: null,

  init() {
    addMessageListener("Social:SetErrorURL", this);
    let webProgress = docShell.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                              .getInterface(Components.interfaces.nsIWebProgress);
    webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_STATE_REQUEST |
                                          Ci.nsIWebProgress.NOTIFY_LOCATION);
  },
  receiveMessage(message) {
    switch(message.name) {
      case "Social:SetErrorURL": {
        
        this.urlTemplate = message.objects.template;
      }
    }
  },

  setErrorPage() {
    
    let frame = docShell.chromeEventHandler;
    let origin = frame.getAttribute("origin");
    let src = frame.getAttribute("src");
    if (src == "about:providerdirectory") {
      frame = content.document.getElementById("activation-frame");
      src = frame.getAttribute("src");
    }

    let url = this.urlTemplate || this.defaultTemplate;
    url = url.replace("%{url}", encodeURIComponent(src));
    url = url.replace("%{origin}", encodeURIComponent(origin));
    if (frame != docShell.chromeEventHandler) {
      
      
      frame.setAttribute("src", url);
    } else {
      let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
      webNav.loadURI(url, null, null, null, null);
    }
    sendAsyncMessage("Social:ErrorPageNotify", {
        origin: origin,
        url: src
    });
  },

  onStateChange(aWebProgress, aRequest, aState, aStatus) {
    let failure = false;
    if ((aState & Ci.nsIWebProgressListener.STATE_STOP)) {
      if (aRequest instanceof Ci.nsIHttpChannel) {
        try {
          
          
          failure = aRequest.responseStatus >= 400 &&
                    aRequest.responseStatus < 600;
        } catch (e) {
          failure = aStatus != Components.results.NS_OK;
        }
      }
    }

    
    
    if (failure && aStatus != Components.results.NS_BINDING_ABORTED) {
      aRequest.cancel(Components.results.NS_BINDING_ABORTED);
      this.setErrorPage();
    }
  },

  onLocationChange(aWebProgress, aRequest, aLocation, aFlags) {
    if (aRequest && aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_ERROR_PAGE) {
      aRequest.cancel(Components.results.NS_BINDING_ABORTED);
      this.setErrorPage();
    }
  },

  onProgressChange() {},
  onStatusChange() {},
  onSecurityChange() {},
};

SocialErrorListener.init();
