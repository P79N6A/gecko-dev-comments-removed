












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

  











  topicObserved(topic, subject=null) {
    return new Promise(resolve => {
      Services.obs.addObserver(function observer(observedSubject, topic, data) {
        if (subject !== null && subject !== observedSubject) { return; }

        Services.obs.removeObserver(observer, topic);
        resolve(data);
      }, topic, false);
    });
  },
};
