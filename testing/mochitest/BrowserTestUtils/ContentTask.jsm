





"use strict";

this.EXPORTED_SYMBOLS = [
  "ContentTask"
];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/Promise.jsm");

const FRAME_SCRIPT = "chrome://mochikit/content/tests/BrowserTestUtils/content-task.js";




let gPromises = new Map();




let gMessageID = 1;




this.ContentTask = {
  
















  spawn: function ContentTask_spawn(browser, arg, task) {
    let mm = browser.ownerDocument.defaultView.messageManager;
    let scripts = mm.getDelayedFrameScripts();

    
    if (!scripts.find(script => script[0] == FRAME_SCRIPT)) {
      mm.loadFrameScript(FRAME_SCRIPT, true);
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
Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager)
  .addMessageListener("content-task:complete", ContentMessageListener);
