




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;
let Cr = Components.results;







var APZCObserver = {
  _debugEvents: false,
  _enabled: false,

  get enabled() {
    return this._enabled;
  },

  init: function() {
    this._enabled = Services.prefs.getBoolPref(kAsyncPanZoomEnabled);
    if (!this._enabled) {
      return;
    }

    let os = Services.obs;
    os.addObserver(this, "apzc-request-content-repaint", false);
    os.addObserver(this, "apzc-handle-pan-begin", false);
    os.addObserver(this, "apzc-handle-pan-end", false);

    Elements.tabList.addEventListener("TabSelect", this, true);
    Elements.tabList.addEventListener("TabOpen", this, true);
    Elements.tabList.addEventListener("TabClose", this, true);
  },

  handleEvent: function APZC_handleEvent(aEvent) {
    switch (aEvent.type) {
      case 'pageshow':
      case 'TabSelect':
        const ROOT_ID = 1;
        let windowUtils = Browser.selectedBrowser.contentWindow.
                          QueryInterface(Ci.nsIInterfaceRequestor).
                          getInterface(Ci.nsIDOMWindowUtils);
        
        let element;
        try {
          element = windowUtils.findElementWithViewId(ROOT_ID);
        } catch (e) {
          
          break;
        }
        windowUtils.setDisplayPortForElement(0, 0, ContentAreaObserver.width,
                                             ContentAreaObserver.height,
                                             element);
        break;
      case 'TabOpen': {
        let browser = aEvent.originalTarget.linkedBrowser;
        browser.addEventListener("pageshow", this, true);
        
        browser.messageManager.addMessageListener("Browser:ContentScroll", this);
        break;
      }
      case 'TabClose': {
        let browser = aEvent.originalTarget.linkedBrowser;
        browser.removeEventListener("pageshow", this, true);
        browser.messageManager.removeMessageListener("Browser:ContentScroll", this);
        break;
      }
    }
  },
  shutdown: function shutdown() {
    if (!this._enabled) {
      return;
    }
    Elements.tabList.removeEventListener("TabSelect", this, true);
    Elements.tabList.removeEventListener("TabOpen", this, true);
    Elements.tabList.removeEventListener("TabClose", this, true);

    let os = Services.obs;
    os.removeObserver(this, "apzc-request-content-repaint");
    os.removeObserver(this, "apzc-handle-pan-begin");
    os.removeObserver(this, "apzc-handle-pan-end");
  },
  observe: function ao_observe(aSubject, aTopic, aData) {
    if (aTopic == "apzc-request-content-repaint") {
      let frameMetrics = JSON.parse(aData);
      let scrollId = frameMetrics.scrollId;
      let scrollTo = frameMetrics.scrollTo;
      let displayPort = frameMetrics.displayPort;
      let resolution = frameMetrics.resolution;
      let compositedRect = frameMetrics.compositedRect;

      let windowUtils = Browser.selectedBrowser.contentWindow.
                                QueryInterface(Ci.nsIInterfaceRequestor).
                                getInterface(Ci.nsIDOMWindowUtils);
      windowUtils.setScrollPositionClampingScrollPortSize(compositedRect.width,
                                                          compositedRect.height);
      Browser.selectedBrowser.messageManager.sendAsyncMessage("Content:SetDisplayPort", {
        scrollX: scrollTo.x,
        scrollY: scrollTo.y,
        x: displayPort.x + scrollTo.x,
        y: displayPort.y + scrollTo.y,
        w: displayPort.width,
        h: displayPort.height,
        scale: resolution,
        id: scrollId
      });

      if (this._debugEvents) {
        Util.dumpLn("APZC scrollId: " + scrollId);
        Util.dumpLn("APZC scrollTo.x: " + scrollTo.x + ", scrollTo.y: " + scrollTo.y);
        Util.dumpLn("APZC setResolution: " + resolution);
        Util.dumpLn("APZC setDisplayPortForElement: displayPort.x: " +
                    displayPort.x + ", displayPort.y: " + displayPort.y +
                    ", displayPort.width: " + displayPort.width +
                    ", displayort.height: " + displayPort.height);
      }
    } else if (aTopic == "apzc-handle-pan-begin") {
      
      
      
      
      if (InputSourceHelper.isPrecise) {
        InputSourceHelper._imprecise();
      }
    }
  },

  receiveMessage: function(aMessage) {
    let json = aMessage.json;
    switch (aMessage.name) {
       
       
       
       
       
      case "Browser:ContentScroll": {
        let data = json.viewId + " " + json.presShellId + " (" + json.scrollOffset.x + ", " + json.scrollOffset.y + ")";
        Services.obs.notifyObservers(null, "scroll-offset-changed", data);
        break;
      }
    }
  }
};
