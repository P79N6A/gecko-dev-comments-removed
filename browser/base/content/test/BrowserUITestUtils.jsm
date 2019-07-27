"use strict";

this.EXPORTED_SYMBOLS = [
  "BrowserUITestUtils",
];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Timer.jsm");






const DEFAULT_WAIT = 2000;





this.BrowserUITestUtils = {

  





















  waitForEvent(subject, eventName, timeoutMs, target) {
    return new Promise((resolve, reject) => {
      function listener(event) {
        if (target && target !== event.target) {
          return;
        }

        subject.removeEventListener(eventName, listener);
        clearTimeout(timerID);
        resolve(event);
      }

      timeoutMs = timeoutMs || DEFAULT_WAIT;
      let stack = new Error().stack;

      let timerID = setTimeout(() => {
        subject.removeEventListener(eventName, listener);
        reject(new Error(`${eventName} event timeout at ${stack}`));
      }, timeoutMs);


      subject.addEventListener(eventName, listener);
    });
  },
};
