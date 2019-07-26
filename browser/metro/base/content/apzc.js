




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;
let Cr = Components.results;







var APZCObserver = {
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
        windowUtils.setDisplayPortForElement(0, 0, ContentAreaObserver.width,
                                             ContentAreaObserver.height,
                                             windowUtils.findElementWithViewId(ROOT_ID));
        break;
      case 'TabOpen': {
        let browser = aEvent.originalTarget.linkedBrowser;
        browser.addEventListener("pageshow", this, true);
        break;
      }
      case 'TabClose': {
        let browser = aEvent.originalTarget.linkedBrowser;
        browser.removeEventListener("pageshow", this);
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

      if (StartUI.isStartPageVisible) {
        let windowUtils = Browser.windowUtils;
        Browser.selectedBrowser.contentWindow.scrollTo(scrollTo.x, scrollTo.y);
        windowUtils.setResolution(resolution, resolution);
        windowUtils.setDisplayPortForElement(displayPort.x * resolution,
                                             displayPort.y * resolution,
                                             displayPort.width * resolution,
                                             displayPort.height * resolution,
                                             Elements.startUI);
      } else {
        let windowUtils = Browser.selectedBrowser.contentWindow.
                                  QueryInterface(Ci.nsIInterfaceRequestor).
                                  getInterface(Ci.nsIDOMWindowUtils);
        windowUtils.setScrollPositionClampingScrollPortSize(compositedRect.width,
                                                            compositedRect.height);
        Browser.selectedBrowser.messageManager.sendAsyncMessage("Content:SetCacheViewport", {
          scrollX: scrollTo.x,
          scrollY: scrollTo.y,
          x: displayPort.x + scrollTo.x,
          y: displayPort.y + scrollTo.y,
          w: displayPort.width,
          h: displayPort.height,
          scale: resolution,
          id: scrollId
        });
      }

      Util.dumpLn("APZC scrollId: " + scrollId);
      Util.dumpLn("APZC scrollTo.x: " + scrollTo.x + ", scrollTo.y: " + scrollTo.y);
      Util.dumpLn("APZC setResolution: " + resolution);
      Util.dumpLn("APZC setDisplayPortForElement: displayPort.x: " +
                  displayPort.x + ", displayPort.y: " + displayPort.y +
                  ", displayPort.width: " + displayPort.width +
                  ", displayort.height: " + displayPort.height);
    } else if (aTopic == "apzc-handle-pan-begin") {
      
      
      
      
      Util.dumpLn("APZC pan-begin");
      if (InputSourceHelper.isPrecise) {
        InputSourceHelper._imprecise();
      }

    } else if (aTopic == "apzc-handle-pan-end") {
      Util.dumpLn("APZC pan-end");
    }
  }
};
