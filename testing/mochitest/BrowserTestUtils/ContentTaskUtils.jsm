











"use strict";

this.EXPORTED_SYMBOLS = [
  "ContentTaskUtils",
];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Timer.jsm");

this.ContentTaskUtils = {
  
















  waitForCondition(condition, msg, interval=100, maxTries=50) {
    return new Promise((resolve, reject) => {
      let tries = 0;
      let intervalID = setInterval(() => {
        if (tries >= maxTries) {
          clearInterval(intervalID);
          msg += ` - timed out after ${maxTries} tries.`;
          reject(msg);
          return;
        }

        let conditionPassed = false;
        try {
          conditionPassed = condition();
        } catch(e) {
          msg += ` - threw exception: ${e}`;
          clearInterval(intervalID);
          reject(msg);
          return;
        }

        if (conditionPassed) {
          clearInterval(intervalID);
          resolve();
        }
        tries++;
      }, interval);
    });
  },

  




























  waitForEvent(subject, eventName, capture, checkFn) {
    return new Promise((resolve, reject) => {
      subject.addEventListener(eventName, function listener(event) {
        try {
          if (checkFn && !checkFn(event)) {
            return;
          }
          subject.removeEventListener(eventName, listener, capture);
          resolve(event);
        } catch (ex) {
          try {
            subject.removeEventListener(eventName, listener, capture);
          } catch (ex2) {
            
          }
          reject(ex);
        }
      }, capture);
    });
  },
};
