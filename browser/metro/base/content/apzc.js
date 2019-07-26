




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
    os.addObserver(this, "apzc-transform-begin", false);

    
    window.addEventListener("SizeChanged", this, true);

    Elements.tabList.addEventListener("TabSelect", this, true);
    Elements.tabList.addEventListener("TabOpen", this, true);
    Elements.tabList.addEventListener("TabClose", this, true);
  },

  shutdown: function shutdown() {
    if (!this._enabled) {
      return;
    }

    let os = Services.obs;
    os.removeObserver(this, "apzc-transform-begin");

    window.removeEventListener("SizeChanged", this, true);

    Elements.tabList.removeEventListener("TabSelect", this, true);
    Elements.tabList.removeEventListener("TabOpen", this, true);
    Elements.tabList.removeEventListener("TabClose", this, true);
  },

  handleEvent: function APZC_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "SizeChanged":
      case 'TabSelect':
        this._resetDisplayPort();
        break;

      case 'pageshow':
        if (aEvent.target != Browser.selectedBrowser.contentDocument) {
          break;
        }
        this._resetDisplayPort();
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

  observe: function ao_observe(aSubject, aTopic, aData) {
    if (aTopic == "apzc-transform-begin") {
      
      
      
      
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
        Services.obs.notifyObservers(null, "apzc-scroll-offset-changed", data);
        break;
      }
    }
  },

  _resetDisplayPort: function () {
    
    
    let doc = Browser.selectedBrowser.contentDocument.documentElement;
    
    
    if (!doc) {
      return;
    }
    let win = Browser.selectedBrowser.contentWindow;
    let factor = 0.2;
    let portX = 0;
    let portY = 0;
    let portWidth = ContentAreaObserver.width;
    let portHeight = ContentAreaObserver.height;

    if (portWidth < doc.scrollWidth) {
      portWidth += ContentAreaObserver.width * factor;
      if (portWidth > doc.scrollWidth) {
        portWidth = doc.scrollWidth;
      }
    }
    if (portHeight < doc.scrollHeight) {
      portHeight += ContentAreaObserver.height * factor;
      if (portHeight > doc.scrollHeight) {
        portHeight = doc.scrollHeight;
      }
    }
    if (win.scrollX > 0) {
      portX -= ContentAreaObserver.width * factor;
    }
    if (win.scrollY > 0) {
      portY -= ContentAreaObserver.height * factor;
    }
    let cwu = Browser.selectedBrowser.contentWindow
                      .QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIDOMWindowUtils);
    cwu.setDisplayPortForElement(portX, portY,
                                 portWidth, portHeight,
                                 Browser.selectedBrowser.contentDocument.documentElement);
  }
};
