












"use strict";

this.EXPORTED_SYMBOLS = [
  "TestUtils",
];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

this.TestUtils = {
  executeSoon(callbackFn) {
    Services.tm.mainThread.dispatch(callbackFn, Ci.nsIThread.DISPATCH_NORMAL);
  },

  

















  topicObserved(topic, checkFn) {
    return new Promise((resolve, reject) => {
      Services.obs.addObserver(function observer(subject, topic, data) {
        try {
          try {
            if (checkFn && !checkFn(subject, data)) {
              return;
            }
          } finally {
            Services.obs.removeObserver(observer, topic);
          }
          resolve([subject, data]);
        } catch (ex) {
          reject(ex);
        }
      }, topic, false);
    });
  },
};
