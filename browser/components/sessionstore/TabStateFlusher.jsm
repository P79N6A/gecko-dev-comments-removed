



"use strict";

this.EXPORTED_SYMBOLS = ["TabStateFlusher"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Promise.jsm", this);








this.TabStateFlusher = Object.freeze({
  




  flush(browser) {
    return TabStateFlusherInternal.flush(browser);
  },

  


  resolve(browser, flushID) {
    TabStateFlusherInternal.resolve(browser, flushID);
  },

  





  resolveAll(browser) {
    TabStateFlusherInternal.resolveAll(browser);
  }
});

let TabStateFlusherInternal = {
  
  _lastRequestID: 0,

  
  _requests: new WeakMap(),

  




  flush(browser) {
    let id = ++this._lastRequestID;
    let mm = browser.messageManager;
    mm.sendAsyncMessage("SessionStore:flush", {id});

    
    let permanentKey = browser.permanentKey;
    let perBrowserRequests = this._requests.get(permanentKey) || new Map();

    return new Promise(resolve => {
      
      perBrowserRequests.set(id, resolve);

      
      this._requests.set(permanentKey, perBrowserRequests);
    });
  },

  


  resolve(browser, flushID) {
    
    if (!this._requests.has(browser.permanentKey)) {
      return;
    }

    
    let perBrowserRequests = this._requests.get(browser.permanentKey);
    if (!perBrowserRequests.has(flushID)) {
      return;
    }

    
    let resolve = perBrowserRequests.get(flushID);
    perBrowserRequests.delete(flushID);
    resolve();
  },

  





  resolveAll(browser) {
    
    if (!this._requests.has(browser.permanentKey)) {
      return;
    }

    
    let perBrowserRequests = this._requests.get(browser.permanentKey);

    
    for (let resolve of perBrowserRequests.values()) {
      resolve();
    }

    
    perBrowserRequests.clear();
  }
};
