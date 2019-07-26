



"use strict";

this.EXPORTED_SYMBOLS = ["Messenger"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Timer.jsm", this);




this.Messenger = Object.freeze({
  send: function (tab, type, options = {}) {
    return MessengerInternal.send(tab, type, options);
  }
});




let MessengerInternal = {
  
  
  _latestMessageID: 0,

  











  send: function (tab, type, options = {}) {
    let browser = tab.linkedBrowser;
    let mm = browser.messageManager;
    let deferred = Promise.defer();
    let id = ++this._latestMessageID;
    let timeout;

    function onMessage({data: {id: mid, data}}) {
      if (mid == id) {
        mm.removeMessageListener(type, onMessage);
        clearTimeout(timeout);
        deferred.resolve(data);
      }
    }

    mm.addMessageListener(type, onMessage);
    mm.sendAsyncMessage(type, {id: id});

    function onTimeout() {
      mm.removeMessageListener(type, onMessage);
      deferred.reject(new Error("Timed out while waiting for a " + type + " " +
                                "response message."));
    }

    let delay = (options && options.timeout) || 5000;
    timeout = setTimeout(onTimeout, delay);
    return deferred.promise;
  }
};
