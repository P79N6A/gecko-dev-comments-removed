





"use strict";

this.EXPORTED_SYMBOLS = [
  "ContentTask"
];

const Cu = Components.utils;
Cu.import("resource://gre/modules/Promise.jsm");




let gScriptLoadedSet = new WeakSet();




let gPromises = new Map();




let gMessageID = 1;




this.ContentTask = {
  
















  spawn: function ContentTask_spawn(browser, arg, task) {
    if(!gScriptLoadedSet.has(browser)) {
      let mm = browser.messageManager;
      mm.addMessageListener("content-task:complete", ContentMessageListener);
      mm.loadFrameScript(
        "chrome://mochikit/content/tests/BrowserTestUtils/content-task.js", true);

      gScriptLoadedSet.add(browser);
    }

    let deferred = {};
    deferred.promise = new Promise((resolve, reject) => {
      deferred.resolve = resolve;
      deferred.reject = reject;
    });

    let id = gMessageID++;
    gPromises.set(id, deferred);

    browser.messageManager.sendAsyncMessage(
      "content-task:spawn",
      {
        id: id,
        runnable: task.toString(),
        arg: arg,
      });

    return deferred.promise;
  },
};

let ContentMessageListener = {
  receiveMessage(aMessage) {
    let id = aMessage.data.id
    let deferred = gPromises.get(id);
    gPromises.delete(id);

    if (aMessage.data.error) {
      deferred.reject(aMessage.data.error);
    } else {
      deferred.resolve(aMessage.data.result);
    }
  },
};
