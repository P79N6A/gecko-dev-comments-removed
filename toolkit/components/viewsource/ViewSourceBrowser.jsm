





const { utils: Cu, interfaces: Ci, classes: Cc } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
  "resource://gre/modules/Deprecated.jsm");

this.EXPORTED_SYMBOLS = ["ViewSourceBrowser"];















this.ViewSourceBrowser = function ViewSourceBrowser(aBrowser) {
  this._browser = aBrowser;
  this.init();
}

ViewSourceBrowser.prototype = {
  


  get browser() {
    return this._browser;
  },

  





  
  messages: [
  ],

  



  init() {
    this.messages.forEach((msgName) => {
      this.mm.addMessageListener(msgName, this);
    });
  },

  



  uninit() {
    this.messages.forEach((msgName) => {
      this.mm.removeMessageListener(msgName, this);
    });
  },

  



  receiveMessage(message) {
    let data = message.data;

    
    switch(message.name) {
    }
  },

  


  get mm() {
    return this.browser.messageManager;
  },

  


  sendAsyncMessage(...args) {
    this.browser.messageManager.sendAsyncMessage(...args);
  },

  





















  loadViewSource({ URL, browser, outerWindowID, lineNumber }) {
    if (!URL) {
      throw new Error("Must supply a URL when opening view source.");
    }

    if (browser) {
      
      
      this.updateBrowserRemoteness(browser.isRemoteBrowser);
    } else {
      if (outerWindowID) {
        throw new Error("Must supply the browser if passing the outerWindowID");
      }
    }

    this.sendAsyncMessage("ViewSource:LoadSource",
                          { URL, outerWindowID, lineNumber });
  },

  









  updateBrowserRemoteness(shouldBeRemote) {
    if (this.browser.isRemoteBrowser != shouldBeRemote) {
      
      
      
      
      throw new Error("View source browser's remoteness mismatch");
    }
  },
};
