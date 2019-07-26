













const CONTENT_URL = "chrome://mochikit/content/mochitest-e10s-utils-content.js";




let locationStub = function(browser) {
  this.browser = browser;
};
locationStub.prototype = {
  get href() {
    return this.browser.webNavigation.currentURI.spec;
  },
  set href(val) {
    this.browser.loadURI(val);
  },
  assign: function(url) {
    this.href = url;
  }
};



let TemporaryWindowStub = function(browser) {
  this._locationStub = new locationStub(browser);
};

TemporaryWindowStub.prototype = {
  
  
  toString: function() {
    return "[Window Stub for e10s tests]";
  },
  get location() {
    return this._locationStub;
  },
  set location(val) {
    this._locationStub.href = val;
  },
  get document() {
    
    return this;
  }
};



function observeNewFrameloader(subject, topic, data) {
  let browser = subject.QueryInterface(Ci.nsIFrameLoader).ownerElement;
  browser._contentWindow = new TemporaryWindowStub(browser);
}

function e10s_init() {
  
  let globalMM = Cc["@mozilla.org/globalmessagemanager;1"]
                   .getService(Ci.nsIMessageListenerManager);
  globalMM.loadFrameScript(CONTENT_URL, true);
  globalMM.addMessageListener("Test:Event", function(message) {
    let event = document.createEvent('HTMLEvents');
    event.initEvent(message.data.name, true, true, {});
    message.target.dispatchEvent(event);
  });

  
  Services.obs.addObserver(observeNewFrameloader, "remote-browser-shown", false);

  
  
  window.addEventListener("oop-browser-crashed", (event) => {
    let uri = event.target.currentURI;
    Cu.reportError("remote browser crashed while on " +
                   (uri ? uri.spec : "<unknown>") + "\n");
  }, true);
}
